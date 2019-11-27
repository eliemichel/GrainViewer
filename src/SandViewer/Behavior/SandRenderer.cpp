#include <sstream>

#include "utils/jsonutils.h"
#include "utils/strutils.h"
#include "PointCloud.h"
#include "ResourceManager.h"
#include "SandRenderer.h"
#include "GlTexture.h"
#include "ShaderProgram.h"
#include "ShaderPool.h"
#include "bufferFillers.h"
#include "MeshDataBehavior.h"
#include "TransformBehavior.h"

void SandRenderer::initShader(ShaderProgram & shader) {
	size_t o = 0;
	shader.use();
	
	shader.setUniform("cubemap", static_cast<GLint>(o++));

	shader.setUniform("uFrameCount", static_cast<GLuint>(m_frameCount));
}

void SandRenderer::updateShader(ShaderProgram & shader, float time) {
	shader.use();
	shader.setUniform("time", static_cast<GLfloat>(time));
}


///////////////////////////////////////////////////////////////////////////////
// Behavior implementation
///////////////////////////////////////////////////////////////////////////////

// private classes for serialization only
class SandRendererModel {
public:
	bool readJson(const rapidjson::Value & json) { JREAD(baseColor); JREAD(normalAlpha); JREAD(metallicRoughness); return true; }
	void writeJson(JsonWriter & writer) const { writer.StartObject(); JWRITE(baseColor); JWRITE(normalAlpha); JWRITE(metallicRoughness); writer.EndObject(); }

	const std::string & baseColor() const { return m_baseColor; }
	const std::string & normalAlpha() const { return m_normalAlpha; }
	const std::string & metallicRoughness() const { return m_metallicRoughness; }
private:
	std::string m_baseColor;
	std::string m_normalAlpha;
	std::string m_metallicRoughness;
};

bool SandRenderer::deserialize(const rapidjson::Value & json)
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

	std::vector<SandRendererModel> models;
	jrArray<SandRendererModel>(json, "models", models);
	for (auto m : models) {
		if (!m.normalAlpha().empty()) {
			loadImpostorTexture(m_normalAlphaTextures, ResourceManager::resolveResourcePath(m.normalAlpha()));
		}
		if (!m.baseColor().empty()) {
			loadImpostorTexture(m_baseColorTextures, ResourceManager::resolveResourcePath(m.baseColor()));
		}
		if (!m.metallicRoughness().empty()) {
			loadImpostorTexture(m_metallicRoughnessTextures, ResourceManager::resolveResourcePath(m.metallicRoughness()));
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

	jrOption(json, "instanceCloudShader", m_instanceCloudShaderName, m_instanceCloudShaderName);

#define jrPropperty(prop) jrOption(json, #prop, m_properties.prop, m_properties.prop)
	jrPropperty(grainRadius);
	jrPropperty(grainMeshScale);
	jrPropperty(instanceLimit);
	jrPropperty(disableImpostors);
	jrPropperty(disableInstances);
#undef jrProperty

	return true;
}

void SandRenderer::start()
{
	m_shader = ShaderPool::GetShader(m_shaderName);
	m_shadowMapShader = ShaderPool::GetShader(m_shadowMapShaderName);
	m_cullingShader = ShaderPool::GetShader(m_cullingShaderName);
	m_instanceCloudShader = ShaderPool::GetShader(m_instanceCloudShaderName);

	if (!m_shader || !m_shadowMapShader) {
		WARN_LOG << "Using direct shader name in MeshRenderer is depreciated, use 'shaders' section to define shaders (shader = " << m_shaderName << ").";
		// Legacy behavior
		m_shader = std::make_shared<ShaderProgram>(m_shaderName);
		m_shadowMapShader = std::make_unique<ShaderProgram>(m_shaderName);
		m_shadowMapShader->define("SHADOW_MAP");
	}

	m_commandBuffer = std::make_unique<GlBuffer>(GL_DRAW_INDIRECT_BUFFER);
	m_commandBuffer->addBlock<DrawElementsIndirectCommand>();
	m_commandBuffer->addBlock<DrawArraysIndirectCommand>();
	m_commandBuffer->alloc();

	m_cullingPointersSsbo = std::make_unique<GlBuffer>(GL_SHADER_STORAGE_BUFFER);
	m_cullingPointersSsbo->addBlock<PointersSsbo>();
	m_cullingPointersSsbo->alloc();

	m_elementBuffer = std::make_unique<GlBuffer>(GL_ELEMENT_ARRAY_BUFFER);
	m_elementBuffer->addBlock<GLuint>(m_nbPoints);
	m_elementBuffer->alloc();
	m_elementBuffer->finalize(); // This buffer will never be mapped on CPU

	m_grainMeshData = getComponent<MeshDataBehavior>();
	m_transform = getComponent<TransformBehavior>();

	m_isDeferredRendered = true;
}

void SandRenderer::onDestroy()
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

void SandRenderer::update(float time)
{
	updateShader(*m_shader, time);
	updateShader(*m_shadowMapShader, time);
}

void SandRenderer::render(const Camera & camera, const World & world, RenderType target) const
{
	switch (target) {
	case DefaultRendering:
		renderDefault(camera, world);
		break;
	case ShadowMapRendering:
	default:
		// TODO
		break;
	}
}

void SandRenderer::reloadShaders()
{
	initShader(*m_shader);
	initShader(*m_shadowMapShader);
}

///////////////////////////////////////////////////////////////////////////////
// private members
///////////////////////////////////////////////////////////////////////////////

glm::mat4 SandRenderer::modelMatrix() const {
	if (auto transform = m_transform.lock()) {
		return transform->modelMatrix();
	}
	else {
		return glm::mat4(1.0f);
	}
}

bool SandRenderer::load(const PointCloud & pointCloud) {
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
	glNamedBufferStorage(m_vbo, static_cast<GLsizeiptr>(attributes.size() * sizeof(GLfloat)), attributes.data(), 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	return true;
}

bool SandRenderer::loadImpostorTexture(std::vector<std::unique_ptr<GlTexture>> & textures, const std::string & textureDirectory) {
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


bool SandRenderer::loadColormapTexture(const std::string & filename)
{
	m_colormapTexture = ResourceManager::loadTexture(filename);
	return m_colormapTexture != nullptr;
}

///////////////////////////////////////////////////////////////////////////////
// Rendering
///////////////////////////////////////////////////////////////////////////////

void SandRenderer::renderDefault(const Camera & camera, const World & world) const
{
	renderCulling(camera, world);

	if (!m_properties.disableImpostors) {
		renderImpostorsDefault(camera, world);
	}

	if (!m_properties.disableInstances) {
		renderInstancesDefault(camera, world);
	}
}

///////////////////////////////////////////////////////////////////////////
// Culling
void SandRenderer::renderCulling(const Camera & camera, const World & world) const
{
	GLuint effectivePointCount = static_cast<GLuint>(m_nbPoints / m_frameCount);

	m_cullingPointersSsbo->fillBlock<PointersSsbo>(0, [effectivePointCount, this](PointersSsbo *pointers, size_t _) {
		pointers->nextInstanceElement = 0;
		pointers->nextImpostorElement = static_cast<GLuint>(effectivePointCount - 1);
	});

	m_cullingShader->use();
	m_cullingShader->bindUniformBlock("Camera", camera.ubo());
	m_cullingShader->setUniform("modelMatrix", modelMatrix());
	m_cullingShader->setUniform("viewModelMatrix", camera.viewMatrix() * modelMatrix());
	m_cullingShader->setUniform("nbPoints", effectivePointCount);
	m_cullingShader->setUniform("instanceLimit", static_cast<GLfloat>(m_properties.instanceLimit));
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_vbo);
	m_cullingPointersSsbo->bindSsbo(2);
	m_elementBuffer->bindSsbo(3);
	glDispatchCompute(static_cast<GLuint>((effectivePointCount + 127) / 128), 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	GLuint instanceCount = 0;
	GLuint impostorFirst = 0;
	GLuint impostorCount = effectivePointCount;
	m_cullingPointersSsbo->readBlock<PointersSsbo>(0, [&instanceCount, &impostorCount, &impostorFirst, effectivePointCount, this](PointersSsbo *pointers, size_t _) {
		if (pointers->nextInstanceElement != 0) {
			instanceCount = pointers->nextInstanceElement - 1;
			impostorFirst = pointers->nextImpostorElement + 1;
			impostorCount = effectivePointCount - impostorFirst;
		}
	});

	// TODO: Write from compute shader
	// a. impostors
	m_commandBuffer->fillBlock<DrawElementsIndirectCommand>(0, [impostorCount, impostorFirst, this](DrawElementsIndirectCommand *cmd, size_t _) {
		cmd->count = impostorCount;
		cmd->instanceCount = 1;
		cmd->firstIndex = impostorFirst;
		cmd->baseVertex = 0;
		cmd->baseInstance = 0;
	});
	// b. instances
	GLuint grainMeshPointCount = 0;
	if (auto mesh = m_grainMeshData.lock()) {
		grainMeshPointCount = mesh->pointCount();
	}
	m_commandBuffer->fillBlock<DrawArraysIndirectCommand>(1, [grainMeshPointCount, instanceCount, this](DrawArraysIndirectCommand *cmd, size_t _) {
		cmd->count = grainMeshPointCount;
		cmd->instanceCount = instanceCount;
		cmd->first = 0;
		cmd->baseInstance = 0;
	});
}

///////////////////////////////////////////////////////////////////////////
// Impostors drawing
void SandRenderer::renderImpostorsDefault(const Camera & camera, const World & world) const
{
	glEnable(GL_PROGRAM_POINT_SIZE);

	m_shader->use();

	// Set uniforms
	m_shader->bindUniformBlock("Camera", camera.ubo());
	m_shader->setUniform("modelMatrix", modelMatrix());
	m_shader->setUniform("viewModelMatrix", camera.viewMatrix() * modelMatrix());
	m_shader->setUniform("invViewMatrix", inverse(camera.viewMatrix()));
	m_shader->setUniform("grainRadius", static_cast<GLfloat>(m_properties.grainRadius));

	size_t o = 0;

	m_shader->setUniform("colormapTexture", static_cast<GLint>(o));
	glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o));
	m_colormapTexture->bind();
	++o;

	// TODO: Use UBO
	for (size_t k = 0; k < m_normalAlphaTextures.size(); ++k) {
		GLuint n = m_normalAlphaTextures[k]->depth();
		GLuint viewCount = static_cast<GLuint>(sqrt(n / 2));

		std::ostringstream oss1;
		oss1 << "impostor[" << k << "].viewCount";
		m_shader->setUniform(oss1.str(), viewCount);

		std::ostringstream oss2;
		oss2 << "impostor[" << k << "].normalAlphaTexture";
		m_shader->setUniform(oss2.str(), static_cast<GLint>(o));
		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o));
		m_normalAlphaTextures[k]->bind();
		++o;

		if (m_baseColorTextures.size() > k) {
			std::ostringstream oss;
			oss << "impostor[" << k << "].baseColorTexture";
			m_shader->setUniform(oss.str(), static_cast<GLint>(o));
			glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o));
			m_baseColorTextures[k]->bind();
			++o;
		}

		if (m_metallicRoughnessTextures.size() > k) {
			std::ostringstream oss;
			oss << "impostor[" << k << "].metallicRoughnesTexture";
			m_shader->setUniform(oss.str(), static_cast<GLint>(o));
			glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o));
			m_metallicRoughnessTextures[k]->bind();
			++o;
		}
	}

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_vbo);

	if (m_colormapTexture) {
		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o++));
		m_colormapTexture->bind();
	}

	//glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o++));
	//skybox.bindEnvTexture();

	glBindVertexArray(m_vao);
	m_commandBuffer->bind();
	m_elementBuffer->bind();
	glDrawElementsIndirect(GL_POINTS, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}

///////////////////////////////////////////////////////////////////////////
// Instances drawing
void SandRenderer::renderInstancesDefault(const Camera & camera, const World & world) const
{
	m_instanceCloudShader->use();

	// Set uniforms
	m_instanceCloudShader->bindUniformBlock("Camera", camera.ubo());
	m_instanceCloudShader->setUniform("modelMatrix", modelMatrix());
	m_instanceCloudShader->setUniform("viewModelMatrix", camera.viewMatrix() * modelMatrix());
	m_instanceCloudShader->setUniform("invViewMatrix", inverse(camera.viewMatrix()));

	m_instanceCloudShader->setUniform("grainRadius", static_cast<GLfloat>(m_properties.grainRadius));
	m_instanceCloudShader->setUniform("grainMeshScale", static_cast<GLfloat>(m_properties.grainMeshScale));

	GLuint o = 0;
	m_shader->setUniform("colormapTexture", static_cast<GLint>(o));
	glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o));
	m_colormapTexture->bind();
	++o;

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_vbo);

	// Render
	if (auto mesh = m_grainMeshData.lock()) {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_vbo);
		m_elementBuffer->bindSsbo(2);
		glBindVertexArray(mesh->vao());
		m_commandBuffer->bind();
		glDrawArraysIndirect(GL_TRIANGLES, static_cast<void*>(static_cast<DrawElementsIndirectCommand*>(nullptr) + 1));
		glBindVertexArray(0);
	}
}
