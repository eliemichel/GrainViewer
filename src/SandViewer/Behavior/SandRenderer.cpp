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
#include "Framebuffer.h"

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

	std::string baseCullingShaderName;
	jrOption(json, "cullingShader", baseCullingShaderName);
	m_cullingShaderNames.push_back(baseCullingShaderName);

	jrOption(json, "instanceCloudShader", m_instanceCloudShaderName, m_instanceCloudShaderName);

	jrOption(json, "occlusionCullingShader", m_occlusionCullingShaderName, m_occlusionCullingShaderName);

#define jrPropperty(prop) jrOption(json, #prop, m_properties.prop, m_properties.prop)
	jrPropperty(grainRadius);
	jrPropperty(grainInnerRadiusRatio);
	jrPropperty(grainMeshScale);
	jrPropperty(instanceLimit);
	jrPropperty(disableImpostors);
	jrPropperty(disableInstances);
#undef jrProperty

	if (json.HasMember("cullingMechanism") && json["cullingMechanism"].IsString()) {
		std::string s = json["cullingMechanism"].GetString();
		if (s == "AtomicSum") {
			m_cullingMechanism = AtomicSum;
		} else if (s == "PrefixSum") {
			m_cullingMechanism = PrefixSum;
		} else if (s == "RestartPrimitive") {
			m_cullingMechanism = RestartPrimitive;
		} else {
			ERR_LOG << "Invalid culling mechanism: " << s << " (valid values are AtomicSum, PrefixSum and RestartPrimitive)";
		}
	}

	// Split compute shader into steps
	if (m_cullingMechanism == RestartPrimitive) {
		WARN_LOG << "RestartPrimitive culling mechanism is not working!";
		m_cullingShaderNames[0] = baseCullingShaderName + "_DOUBLE_ELEMENT_BUFFER";
		ShaderPool::AddShaderVariant(m_cullingShaderNames[0], baseCullingShaderName, "DOUBLE_ELEMENT_BUFFER");
	} else if (m_cullingMechanism == PrefixSum) {
		m_cullingShaderNames.resize(_PrefixSumCullingShadersCount);
		m_cullingShaderNames[MarkImpostors] = baseCullingShaderName + "_STEP_MARK_IMPOSTORS";
		ShaderPool::AddShaderVariant(m_cullingShaderNames[MarkImpostors], baseCullingShaderName, "STEP_MARK_IMPOSTORS");
		m_cullingShaderNames[MarkInstances] = baseCullingShaderName + "_STEP_MARK_INSTANCES";
		ShaderPool::AddShaderVariant(m_cullingShaderNames[MarkInstances], baseCullingShaderName, "STEP_MARK_INSTANCES");
		m_cullingShaderNames[Group] = baseCullingShaderName + "_STEP_GROUP";
		ShaderPool::AddShaderVariant(m_cullingShaderNames[Group], baseCullingShaderName, "STEP_GROUP");
		m_cullingShaderNames[BuildCommandBuffer] = baseCullingShaderName + "_STEP_BUILD_COMMAND_BUFFER";
		ShaderPool::AddShaderVariant(m_cullingShaderNames[BuildCommandBuffer], baseCullingShaderName, "STEP_BUILD_COMMAND_BUFFER");

#define step(Step, STEP_DEFINE) \
	m_cullingShaderNames[Step] = baseCullingShaderName + "_" + #STEP_DEFINE; \
	ShaderPool::AddShaderVariant(m_cullingShaderNames[Step], baseCullingShaderName, #STEP_DEFINE);
		step(MarkImpostors, STEP_MARK_IMPOSTORS);
		step(MarkInstances, STEP_MARK_INSTANCES);
		step(Group, STEP_GROUP);
		step(BuildCommandBuffer, STEP_BUILD_COMMAND_BUFFER);
#undef step
	}

	return true;
}

void SandRenderer::start()
{
	m_shader = ShaderPool::GetShader(m_shaderName);
	m_shadowMapShader = ShaderPool::GetShader(m_shadowMapShaderName);
	for (const auto& name : m_cullingShaderNames) {
		m_cullingShaders.push_back(ShaderPool::GetShader(name));
	}
	m_instanceCloudShader = ShaderPool::GetShader(m_instanceCloudShaderName);
	m_prefixSumShader = ShaderPool::GetShader(m_prefixSumShaderName);
	m_occlusionCullingShader = ShaderPool::GetShader(m_occlusionCullingShaderName);

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

	m_prefixSumInfoSsbo = std::make_unique<GlBuffer>(GL_SHADER_STORAGE_BUFFER);
	m_prefixSumInfoSsbo->addBlock<PrefixSumInfoSsbo>();
	m_prefixSumInfoSsbo->alloc();

	size_t elementBufferCount = m_cullingMechanism == AtomicSum ? 1 : (m_cullingMechanism == PrefixSum ? 3 : 2);
	GLuint effectivePointCount = static_cast<GLuint>(m_nbPoints / m_frameCount);
	for (size_t i = 0; i < elementBufferCount; ++i) {
		auto buff = std::make_unique<GlBuffer>(GL_ELEMENT_ARRAY_BUFFER);
		buff->addBlock<GLuint>(effectivePointCount);
		buff->alloc();
		buff->finalize(); // This buffer will never be mapped on CPU
		m_elementBuffers.push_back(std::move(buff));
	}

	m_grainMeshData = getComponent<MeshDataBehavior>();
	m_transform = getComponent<TransformBehavior>();

	// TODO: ensure this map always has the same resolution as the render
	m_occlusionCullingMap = std::make_unique<Framebuffer>(1000, 600);

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
	if (m_cullingMechanism == PrefixSum) {
		renderCullingPrefixSum(camera, world);
	} else {
		renderCulling(camera, world);
	}

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

	m_cullingShaders[0]->use();
	m_cullingShaders[0]->bindUniformBlock("Camera", camera.ubo());
	m_cullingShaders[0]->setUniform("modelMatrix", modelMatrix());
	m_cullingShaders[0]->setUniform("viewModelMatrix", camera.viewMatrix() * modelMatrix());
	m_cullingShaders[0]->setUniform("nbPoints", effectivePointCount);
	m_cullingShaders[0]->setUniform("instanceLimit", static_cast<GLfloat>(m_properties.instanceLimit));
	GLuint ssboIndex = 1;
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssboIndex++, m_vbo);
	m_cullingPointersSsbo->bindSsbo(ssboIndex++);
	for (auto& buff : m_elementBuffers) {
		buff->bindSsbo(ssboIndex++);
	}
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
		} else if (pointers->nextImpostorElement == 0) {
			impostorCount = 0;
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

void SandRenderer::renderCullingPrefixSum(const Camera & camera, const World & world) const
{
	int resultIndex;
	if (!m_cullingShaders[MarkInstances] || !m_cullingShaders[MarkInstances]->isValid()
		|| !m_cullingShaders[MarkImpostors] || !m_cullingShaders[MarkImpostors]->isValid()
		|| !m_cullingShaders[BuildCommandBuffer] || !m_cullingShaders[BuildCommandBuffer]
		|| !m_cullingShaders[Group]->isValid() || !m_occlusionCullingShader
		|| !m_occlusionCullingShader->isValid()) {
		return;
	}

	GLuint effectivePointCount = static_cast<GLuint>(m_nbPoints / m_frameCount);

	// 0. Occlusion culling map (kind of shadow map)
	if (m_properties.grainInnerRadiusRatio > 0) {
		GLint drawFbo = 0, readFbo = 0;
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo);
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFbo);
		m_occlusionCullingMap->bind();
		glClear(GL_DEPTH_BUFFER_BIT);

		const ShaderProgram & shader = *m_occlusionCullingShader;
		shader.use();
		shader.bindUniformBlock("Camera", camera.ubo());
		shader.setUniform("modelMatrix", modelMatrix());
		shader.setUniform("viewModelMatrix", camera.viewMatrix() * modelMatrix());

		shader.setUniform("uOuterOverInnerRadius", 1.0f / m_properties.grainInnerRadiusRatio);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_vbo);

		glBindVertexArray(m_vao);
		glDrawArrays(GL_POINTS, 0, effectivePointCount);
		glBindVertexArray(0);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFbo);
	}

	// 1. Mark instances
	{
		const ShaderProgram & shader = *m_cullingShaders[MarkInstances];
		shader.use();
		shader.bindUniformBlock("Camera", camera.ubo());
		shader.setUniform("modelMatrix", modelMatrix());
		shader.setUniform("viewModelMatrix", camera.viewMatrix() * modelMatrix());
		shader.setUniform("uPointCount", effectivePointCount);
		shader.setUniform("uInstanceLimit", static_cast<GLfloat>(m_properties.instanceLimit));
		m_prefixSumInfoSsbo->bindSsbo(0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_vbo);
		m_elementBuffers[1]->bindSsbo(2);
		glDispatchCompute(static_cast<GLuint>((effectivePointCount + 127) / 128), 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	// 2. Prefix sum instances
	resultIndex = prefixSum(*m_elementBuffers[1], *m_elementBuffers[2], effectivePointCount, *m_prefixSumShader);

	// 3. Group instances
	{
		const ShaderProgram & shader = *m_cullingShaders[Group];
		shader.use();
		shader.setUniform("uPointCount", effectivePointCount);
		shader.setUniform("uType", static_cast<GLuint>(0));
		m_prefixSumInfoSsbo->bindSsbo(0);
		m_elementBuffers[1+resultIndex]->bindSsbo(1);
		m_elementBuffers[0]->bindSsbo(2);
		glDispatchCompute(static_cast<GLuint>((effectivePointCount + 127) / 128), 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	// 4. Mark impostors
	{
		const ShaderProgram & shader = *m_cullingShaders[MarkImpostors];
		shader.use();
		shader.bindUniformBlock("Camera", camera.ubo());
		shader.setUniform("modelMatrix", modelMatrix());
		shader.setUniform("viewModelMatrix", camera.viewMatrix() * modelMatrix());
		shader.setUniform("uPointCount", effectivePointCount);
		shader.setUniform("uInstanceLimit", static_cast<GLfloat>(m_properties.instanceLimit));
		shader.setUniform("uInnerOverOuterRadius", m_properties.grainInnerRadiusRatio);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_occlusionCullingMap->depthTexture());
		shader.setUniform("depthMap", 0);
		m_prefixSumInfoSsbo->bindSsbo(0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_vbo);
		m_elementBuffers[1]->bindSsbo(2);
		glDispatchCompute(static_cast<GLuint>((effectivePointCount + 127) / 128), 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	// 5. Prefix sum impostors
	resultIndex = prefixSum(*m_elementBuffers[1], *m_elementBuffers[2], effectivePointCount, *m_prefixSumShader);

	// 6. Group impostors
	{
		const ShaderProgram & shader = *m_cullingShaders[Group];
		shader.use();
		shader.setUniform("uPointCount", effectivePointCount);
		shader.setUniform("uType", static_cast<GLuint>(1));
		m_prefixSumInfoSsbo->bindSsbo(0);
		m_elementBuffers[1+resultIndex]->bindSsbo(1);
		m_elementBuffers[0]->bindSsbo(2);
		glDispatchCompute(static_cast<GLuint>((effectivePointCount + 127) / 128), 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	// 7. Build command buffers
	{
		GLuint grainMeshPointCount = 0;
		if (auto mesh = m_grainMeshData.lock()) {
			grainMeshPointCount = mesh->pointCount();
		}

		const ShaderProgram & shader = *m_cullingShaders[BuildCommandBuffer];
		shader.use();
		shader.setUniform("uPointCount", effectivePointCount);
		shader.setUniform("uGrainMeshPointCount", grainMeshPointCount);
		m_prefixSumInfoSsbo->bindSsbo(0);
		m_commandBuffer->bindSsbo(1);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	// 8. Collect info
	m_prefixSumInfoSsbo->readBlock<PrefixSumInfoSsbo>(0, [this](PrefixSumInfoSsbo *ssbo, size_t _) {
		m_renderInfo.instanceCount = ssbo->instanceCount;
		m_renderInfo.impostorCount = ssbo->impostorCount;
	});
}

///////////////////////////////////////////////////////////////////////////
// Impostors drawing
void SandRenderer::renderImpostorsDefault(const Camera & camera, const World & world) const
{
	glEnable(GL_PROGRAM_POINT_SIZE);
	if (m_cullingMechanism == RestartPrimitive) {
		glEnable(GL_PRIMITIVE_RESTART);
		glPrimitiveRestartIndex(0xFFFFFFF0);
	}

	m_shader->use();

	// Set uniforms
	m_shader->bindUniformBlock("Camera", camera.ubo());
	m_shader->setUniform("modelMatrix", modelMatrix());
	m_shader->setUniform("viewModelMatrix", camera.viewMatrix() * modelMatrix());
	m_shader->setUniform("invViewMatrix", inverse(camera.viewMatrix()));
	m_shader->setUniform("grainRadius", static_cast<GLfloat>(m_properties.grainRadius));

	size_t o = 0;

	if (m_colormapTexture) {
		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o));
		m_colormapTexture->bind();
		m_shader->setUniform("colormapTexture", static_cast<GLint>(o));
		++o;
	}

	// TODO: Use UBO
	for (size_t k = 0; k < m_normalAlphaTextures.size(); ++k) {
		GLuint n = m_normalAlphaTextures[k]->depth();
		GLuint viewCount = static_cast<GLuint>(sqrt(n / 2));

		std::ostringstream oss1;
		oss1 << "impostor[" << k << "].viewCount";
		m_shader->setUniform(oss1.str(), viewCount);

		std::ostringstream oss2;
		oss2 << "impostor[" << k << "].normalAlphaTexture";
		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o));
		m_normalAlphaTextures[k]->bind();
		m_shader->setUniform(oss2.str(), static_cast<GLint>(o));
		++o;

		if (m_baseColorTextures.size() > k) {
			std::ostringstream oss;
			oss << "impostor[" << k << "].baseColorTexture";
			glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o));
			m_baseColorTextures[k]->bind();
			m_shader->setUniform(oss.str(), static_cast<GLint>(o));
			++o;
		}

		if (m_metallicRoughnessTextures.size() > k) {
			std::ostringstream oss;
			oss << "impostor[" << k << "].metallicRoughnesTexture";
			glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o));
			m_metallicRoughnessTextures[k]->bind();
			m_shader->setUniform(oss.str(), static_cast<GLint>(o));
			++o;
		}
	}

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_vbo);

	//glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o++));
	//skybox.bindEnvTexture();

	glBindVertexArray(m_vao);
	m_commandBuffer->bind();
	m_elementBuffers[0]->bind();
	glDrawElementsIndirect(GL_POINTS, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}

///////////////////////////////////////////////////////////////////////////
// Instances drawing
void SandRenderer::renderInstancesDefault(const Camera & camera, const World & world) const
{
	if (m_cullingMechanism == RestartPrimitive) {
		glEnable(GL_PRIMITIVE_RESTART);
		glPrimitiveRestartIndex(0xFFFFFFF0);
	}

	m_instanceCloudShader->use();

	// Set uniforms
	m_instanceCloudShader->bindUniformBlock("Camera", camera.ubo());
	m_instanceCloudShader->setUniform("modelMatrix", modelMatrix());
	m_instanceCloudShader->setUniform("viewModelMatrix", camera.viewMatrix() * modelMatrix());
	m_instanceCloudShader->setUniform("invViewMatrix", inverse(camera.viewMatrix()));

	m_instanceCloudShader->setUniform("grainRadius", static_cast<GLfloat>(m_properties.grainRadius));
	m_instanceCloudShader->setUniform("grainMeshScale", static_cast<GLfloat>(m_properties.grainMeshScale));

	GLuint o = 0;
	if (m_colormapTexture) {
		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o));
		m_colormapTexture->bind();
		m_instanceCloudShader->setUniform("colormapTexture", static_cast<GLint>(o));
		++o;
	}

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_vbo);

	// Render
	if (auto mesh = m_grainMeshData.lock()) {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_vbo);
		m_elementBuffers[m_cullingMechanism == RestartPrimitive ? 1 : 0]->bindSsbo(2);
		glBindVertexArray(mesh->vao());
		m_commandBuffer->bind();
		glDrawArraysIndirect(GL_TRIANGLES, static_cast<void*>(static_cast<DrawElementsIndirectCommand*>(nullptr) + 1));
		glBindVertexArray(0);
	}
}

/**
 * Input buffer is buffer0, output buffer is buffer0 or buffer1, as specified
 * by the returned value.
 */
int SandRenderer::prefixSum(const GlBuffer & buffer0, const GlBuffer & buffer1, int elementCount, const ShaderProgram & shader)
{
	if (!shader.isValid()) return -1;

	int iterationCount = static_cast<int>(2 * (floor(log(elementCount) / log(2)) + 1));

	shader.use();
	shader.setUniform("uElementCount", static_cast<GLuint>(elementCount));
	for (int i = 0; i < iterationCount; ++i) {
		// Element buffers 1 and 2 are alternatively used as previous and current buffer
		buffer0.bindSsbo(i % 2 == 0 ? 1 : 2);
		buffer1.bindSsbo(i % 2 == 0 ? 2 : 1);
		shader.setUniform("uIteration", static_cast<GLuint>(i));
		glDispatchCompute(static_cast<GLuint>((elementCount + 127) / 128), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	return iterationCount % 2;
}

