#include <sstream>

#include "utils/jsonutils.h"
#include "utils/strutils.h"
#include "PointCloud.h"
#include "ResourceManager.h"
#include "ImpostorCloudRenderer.h"
#include "GlTexture.h"
#include "ShaderProgram.h"
#include "ShaderPool.h"
#include "bufferFillers.h"

void ImpostorCloudRenderer::renderWithShader(const Camera & camera, const World & world, const ShaderProgram & shader) const
{
	glm::mat4 viewModelMatrix = camera.viewMatrix() * m_modelMatrix;

	///////////////////////////////////////////////////////////////////////////
	// Culling

	GLuint effectivePointCount = static_cast<GLuint>(m_nbPoints / m_frameCount);

	m_cullingPointersSsbo->fillBlock<PointersSsbo>(0, [effectivePointCount, this](PointersSsbo *pointers, size_t _) {
		pointers->nextInstanceElement = 0;
		pointers->nextImpostorElement = static_cast<GLuint>(effectivePointCount - 1);
	});

	m_cullingShader->use();
	m_cullingShader->bindUniformBlock("Camera", camera.ubo());
	m_cullingShader->setUniform("modelMatrix", m_modelMatrix);
	m_cullingShader->setUniform("viewModelMatrix", viewModelMatrix);
	m_cullingShader->setUniform("nbPoints", effectivePointCount);
	m_cullingShader->setUniform("uInstanceLimit", static_cast<GLfloat>(1.05));
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_vbo);
	m_cullingPointersSsbo->bindSsbo(2);
	m_elementBuffer->bindSsbo(3);
	glDispatchCompute(static_cast<GLuint>((effectivePointCount + 127) / 128), 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	GLuint instanceCount = 0;
	GLuint impostorFirst = 0;
	GLuint impostorCount = effectivePointCount;
	m_cullingPointersSsbo->readBlock<PointersSsbo>(0, [&instanceCount, &impostorCount, &impostorFirst, effectivePointCount, this](PointersSsbo *pointers, size_t _) {
		DEBUG_LOG << "nextInstanceElement: " << pointers->nextInstanceElement << ", nextImpostorElement: " << pointers->nextImpostorElement;
		if (pointers->nextInstanceElement != 0) {
			instanceCount = pointers->nextInstanceElement - 1;
			impostorFirst = pointers->nextImpostorElement + 1;
			impostorCount = effectivePointCount - impostorFirst;
		}
	});

	// TODO: Write from compute shader
	m_drawIndirectBuffer->fillBlock<DrawElementsIndirectCommand>(0, [impostorCount, impostorFirst, this](DrawElementsIndirectCommand *cmd, size_t _) {
		cmd->count = impostorCount;
		cmd->instanceCount = 1;
		cmd->firstIndex = impostorFirst;
		cmd->baseVertex = 0;
		cmd->baseInstance = 0;
	});

	///////////////////////////////////////////////////////////////////////////
	// Drawing

	glEnable(GL_PROGRAM_POINT_SIZE);

	shader.use();

	// Set uniforms
	m_shader->bindUniformBlock("Camera", camera.ubo());
	m_shader->setUniform("modelMatrix", m_modelMatrix);
	m_shader->setUniform("viewModelMatrix", viewModelMatrix);
	m_shader->setUniform("invViewMatrix", inverse(camera.viewMatrix()));

	shader.setUniform("iResolution", camera.resolution());

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_vbo);

	// Bind textures
	size_t o = 0;
	for (auto & tex : m_normalTextures) {
		tex->bind(static_cast<GLuint>(o++));
	}
	for (auto & tex : m_depthTextures) {
		tex->bind(static_cast<GLuint>(o++));
	}
	for (auto & tex : m_albedoTextures) {
		tex->bind(static_cast<GLuint>(o++));
	}

	if (m_colormapTexture) {
		m_colormapTexture->bind(static_cast<GLuint>(o++));
	}

	//glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o++));
	//skybox.bindEnvTexture();

	// Render
	glBindVertexArray(m_vao);
	m_drawIndirectBuffer->bind();
	m_elementBuffer->bind();
	glDrawElementsIndirect(GL_POINTS, GL_UNSIGNED_INT, nullptr);
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
	std::string filename = ResourceManager::resolveResourcePath(json["pointcloud"].GetString());
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
	m_shadowMapShaderName = m_shaderName + "_SHADOW_MAP";
	ShaderPool::AddShaderVariant(m_shadowMapShaderName, m_shaderName, "SHADOW_MAP");

	jrOption(json, "cullingShader", m_cullingShaderName, m_cullingShaderName);

	return true;
}

void ImpostorCloudRenderer::start()
{
	m_shader = ShaderPool::GetShader(m_shaderName);
	m_shadowMapShader = ShaderPool::GetShader(m_shadowMapShaderName);
	m_cullingShader = ShaderPool::GetShader(m_cullingShaderName);

	if (!m_shader || !m_shadowMapShader) {
		WARN_LOG << "Using direct shader name in MeshRenderer is depreciated, use 'shaders' section to define shaders (shader = " << m_shaderName << ").";
		// Legacy behavior
		m_shader = std::make_shared<ShaderProgram>(m_shaderName);
		m_shadowMapShader = std::make_unique<ShaderProgram>(m_shaderName);
		m_shadowMapShader->define("SHADOW_MAP");
	}

	m_drawIndirectBuffer = std::make_unique<GlBuffer>(GL_DRAW_INDIRECT_BUFFER);
	m_drawIndirectBuffer->addBlock<DrawElementsIndirectCommand>();
	m_drawIndirectBuffer->alloc();

	m_cullingPointersSsbo = std::make_unique<GlBuffer>(GL_SHADER_STORAGE_BUFFER);
	m_cullingPointersSsbo->addBlock<PointersSsbo>();
	m_cullingPointersSsbo->alloc();

	m_elementBuffer = std::make_unique<GlBuffer>(GL_ELEMENT_ARRAY_BUFFER);
	m_elementBuffer->addBlock<GLuint>(m_nbPoints);
	m_elementBuffer->alloc();
	m_elementBuffer->finalize(); // This buffer will never be mapped on CPU

	m_isDeferredRendered = true;
	m_modelMatrix = glm::mat4(1.0);
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
	initShader(*m_shader);
	initShader(*m_shadowMapShader);
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

	glCreateVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glCreateBuffers(1, &m_vbo);
	glNamedBufferStorage(m_vbo, static_cast<GLsizeiptr>(attributes.size() * sizeof(GLfloat)), attributes.data(), 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
	//glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, 1); // To be tested
	glEnableVertexArrayAttrib(m_vao, 0);
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
	m_colormapTexture = ResourceManager::loadTexture(filename);
	return m_colormapTexture != nullptr;
}
