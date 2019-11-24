#include <sstream>

#include "utils/jsonutils.h"
#include "utils/strutils.h"
#include "PointCloud.h"
#include "ResourceManager.h"
#include "ImpostorCloudRenderer.h"
#include "GlTexture.h"
#include "ShaderProgram.h"

void ImpostorCloudRenderer::renderWithShader(const Camera & camera, const World & world, const ShaderProgram & shader) const
{
	glEnable(GL_PROGRAM_POINT_SIZE);

	shader.use();

	// Set uniforms
	shader.setUniform("modelMatrix", m_modelMatrix);
	shader.setUniform("invModelMatrix", glm::inverse(m_modelMatrix));
	shader.setUniform("viewMatrix", camera.viewMatrix());
	shader.setUniform("invViewMatrix", glm::inverse(camera.viewMatrix()));
	shader.setUniform("projectionMatrix", camera.projectionMatrix());

	shader.setUniform("iResolution", camera.resolution());

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_vbo);

	// Bind textures
	size_t o = 0;
	for (auto & tex : m_normalTextures) {
		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o++));
		tex->bind();
	}
	for (auto & tex : m_depthTextures) {
		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o++));
		tex->bind();
	}
	for (auto & tex : m_albedoTextures) {
		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o++));
		tex->bind();
	}

	if (m_colormapTexture) {
		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o++));
		m_colormapTexture->bind();
	}

	//glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o++));
	//skybox.bindEnvTexture();

	// Render
	glBindVertexArray(m_vao);
	glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_nbPoints / m_frameCount));
	glBindVertexArray(0);
}

void ImpostorCloudRenderer::initShader(ShaderProgram & shader) {
	size_t o = 0;
	shader.use();
	for (size_t i = 0; i < m_normalTextures.size(); ++i) {
		std::ostringstream ss;
		ss << "impostorTexture" << i;
		shader.setUniform(ss.str(), static_cast<GLint>(o++));
	}
	for (size_t i = 0; i < m_depthTextures.size(); ++i) {
		std::ostringstream ss;
		ss << "impostorDepthTexture" << i;
		shader.setUniform(ss.str(), static_cast<GLint>(o++));
	}
	for (size_t i = 0; i < m_albedoTextures.size(); ++i) {
		std::ostringstream ss;
		ss << "impostorAlbedoTexture" << i;
		shader.setUniform(ss.str(), static_cast<GLint>(o++));
	}
	if (m_colormapTexture) {
		shader.setUniform("colormapTexture", static_cast<GLint>(o++));
	}
	shader.setUniform("cubemap", static_cast<GLint>(o++));

	shader.setUniform("uFrameCount", static_cast<GLuint>(m_frameCount));
}

void ImpostorCloudRenderer::updateShader(ShaderProgram & shader, float time) {
	shader.use();
	shader.setUniform("time", static_cast<GLfloat>(time));
}


///////////////////////////////////////////////////////////////////////////////
// Behavior implementation
///////////////////////////////////////////////////////////////////////////////

// private classes for serialization only
class MultiViewImpostorCloudModel {
public:
	bool readJson(const rapidjson::Value & json) { JREAD(albedo); JREAD(normal); JREAD(depth); return true; }
	void writeJson(JsonWriter & writer) const { writer.StartObject(); JWRITE(albedo); JWRITE(normal); JWRITE(depth); writer.EndObject(); }

	const std::string & albedo() const { return m_albedo; }
	const std::string & normal() const { return m_normal; }
	const std::string & depth() const { return m_depth; }
private:
	std::string m_albedo;
	std::string m_normal;
	std::string m_depth;
};
class ObjectPhysics {
public:
	bool readJson(const rapidjson::Value & json) { JREAD_DEFAULT(enabled, false); JREAD(shader); return true; }
	void writeJson(JsonWriter & writer) const { writer.StartObject(); JWRITE(enabled); JWRITE(shader); writer.EndObject(); }

	bool enabled() const { return m_enabled; }
	const std::string & shader() const { return m_shader; }
private:
	bool m_enabled;
	std::string m_shader;
};

bool ImpostorCloudRenderer::deserialize(const rapidjson::Value & json)
{
	if (!json.HasMember("pointcloud") || !json["pointcloud"].IsString()) {
		ERR_LOG << "multiViewImpostor-cloud object requires a string field 'pointcloud'";
		return false;
	}

	// Position
	// TODO: this should be in another Component/DataObject
	PointCloud cloud;
	std::string & filename = ResourceManager::resolveResourcePath(json["pointcloud"].GetString());
	if (endsWith(filename, ".bin")) {
		cloud.loadBin(filename);
	}
	else {
		cloud.loadXYZ(filename);
	}

	if (!load(cloud)) {
		return false;
	}

	std::vector<MultiViewImpostorCloudModel> models;
	jrArray<MultiViewImpostorCloudModel>(json, "models", models);
	for (auto m : models) {
		if (!m.normal().empty()) {
			loadImpostorTexture(m_normalTextures, ResourceManager::resolveResourcePath(m.normal()));
		}
		if (!m.depth().empty()) {
			loadImpostorTexture(m_depthTextures, ResourceManager::resolveResourcePath(m.depth()));
		}
		if (!m.albedo().empty()) {
			loadImpostorTexture(m_albedoTextures, ResourceManager::resolveResourcePath(m.albedo()));
		}
	}

	// Colormap
	std::string colormap;
	jrOption(json, "colormap", colormap);
	if (!colormap.empty()) {
		loadColormapTexture(ResourceManager::resolveResourcePath(colormap));
	}
	
	// Shader
	jrOption(json, "shader", m_shaderName, m_shaderName);

	return true;
}

void ImpostorCloudRenderer::start()
{
	m_shader = std::make_unique<ShaderProgram>(m_shaderName);
	m_shadowMapShader = std::make_unique<ShaderProgram>(m_shaderName);
	m_shadowMapShader->define("SHADOW_MAP");
	m_isDeferredRendered = true;
	//m_nbPoints = 0;
}

void ImpostorCloudRenderer::onDestroy()
{
	if (m_nbPoints == 0) {
		// cloud has never been loaded, nothing to free (assert that..)
		return;
	}

	glBindVertexArray(m_vao);
	glDeleteBuffers(1, &m_vbo);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &m_vao);
}

void ImpostorCloudRenderer::update(float time)
{
	updateShader(*m_shader, time);
	updateShader(*m_shadowMapShader, time);
}

void ImpostorCloudRenderer::render(const Camera & camera, const World & world, RenderType target) const
{
	const ShaderProgram & shader = (target == ShadowMapRendering) ? *m_shadowMapShader : *m_shader;
	renderWithShader(camera, world, shader);
}

void ImpostorCloudRenderer::reloadShaders()
{
	reloadOneShader(*m_shader);
	reloadOneShader(*m_shadowMapShader);
}

///////////////////////////////////////////////////////////////////////////////
// private members
///////////////////////////////////////////////////////////////////////////////

bool ImpostorCloudRenderer::load(const PointCloud & pointCloud) {
	// A. Load point cloud
	m_nbPoints = pointCloud.data().size();
	std::vector<GLfloat> attributes(m_nbPoints * 8);
	GLfloat *v = attributes.data();

	m_frameCount = pointCloud.frameCount();
	for (auto p : pointCloud.data()) {
		v[0] = p.x; v[1] = p.y; v[2] = p.z; v += 4;
	}

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glCreateBuffers(1, &m_vbo);
	glNamedBufferStorage(m_vbo, static_cast<GLsizeiptr>(attributes.size() * sizeof(GLfloat)), attributes.data(), NULL);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	return true;
}

bool ImpostorCloudRenderer::loadImpostorTexture(std::vector<std::unique_ptr<GlTexture>> & textures, const std::string & textureDirectory) {
	auto tex = ResourceManager::loadTextureStack(textureDirectory);
	
	if (!tex) {
		return false;
	}
	float n = sqrt(static_cast<float>(tex->depth()) / 2.f);
	if (n != round(n) || n <= 0.f) {
		ERR_LOG << "Number of textures not in the form 2*n*n. Did you generate them using share/scripts/make_octaedron.py?";
		return false;
	}

	textures.push_back(std::move(tex));
	return true;
}


bool ImpostorCloudRenderer::loadColormapTexture(const std::string & filename)
{
	m_colormapTexture = ResourceManager::loadTexture(filename, 10 /* levels */);
	return m_colormapTexture != nullptr;
}

void ImpostorCloudRenderer::reloadOneShader(ShaderProgram & shader) {
	shader.load();
	initShader(shader);
}

