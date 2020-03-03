#include <sstream>
#include <regex>

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
#include "PointCloudDataBehavior.h"
#include "AnimationManager.h"
#include "EnvironmentVariables.h"

#define MAKE_STR(contents) (std::ostringstream() << contents).str()

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

bool SandRenderer::deserialize(const rapidjson::Value & json, const EnvironmentVariables & env, std::shared_ptr<AnimationManager> animations)
{
	std::vector<SandRendererModel> models;
	jrArray<SandRendererModel>(json, "models", models);
	for (auto m : models) {
		if (!m.normalAlpha().empty()) {
			bool success = loadImpostorTexture(m_normalAlphaTextures, ResourceManager::resolveResourcePath(m.normalAlpha()));
			if (success) {
				auto tex = Filtering::CreateLeanTexture(*m_normalAlphaTextures.back());
				m_leanTextures.push_back(std::move(tex));
			}
		}
		if (!m.baseColor().empty()) {
			loadImpostorTexture(m_baseColorTextures, ResourceManager::resolveResourcePath(m.baseColor()));
		}
		if (!m.metallicRoughness().empty()) {
			loadImpostorTexture(m_metallicRoughnessTextures, ResourceManager::resolveResourcePath(m.metallicRoughness()));
			m_properties.hasMetallicRoughnessMap = true;
		}
	}

	// Colormap
	std::string colormap;
	jrOption(json, "colormap", colormap);
	if (!colormap.empty()) {
		loadColormapTexture(ResourceManager::resolveResourcePath(colormap));
	}

	// Instance materials
	MeshRenderer::Material::deserializeArray(json, "instanceMaterials", m_instanceMaterials);

	// Shader
	jrOption(json, "shader", m_impostorShaderName, m_impostorShaderName);
	m_shadowMapImpostorShaderName = m_impostorShaderName + "_SHADOW_MAP";
	ShaderPool::AddShaderVariant(m_shadowMapImpostorShaderName, m_impostorShaderName, "SHADOW_MAP");

	std::string baseCullingShaderName;
	jrOption(json, "cullingShader", baseCullingShaderName);
	m_cullingShaderNames.push_back(baseCullingShaderName);

	jrOption(json, "instanceCloudShader", m_instanceCloudShaderName, m_instanceCloudShaderName);

	jrOption(json, "occlusionCullingShader", m_occlusionCullingShaderName, m_occlusionCullingShaderName);

#define jrPropperty(prop) jrOption(json, #prop, m_properties.prop, m_properties.prop)
	jrPropperty(grainRadius);
	//jrPropperty(grainInnerRadiusRatio);
	jrPropperty(grainMeshScale);
	jrPropperty(instanceLimit);
	jrPropperty(disableImpostors);
	jrPropperty(disableInstances);
	jrPropperty(renderAdditive);
#undef jrProperty

	// grainInnerRadiusRatio can be an array of keyframes
	// TODO: do this for all other properties
	if (json.HasMember("grainInnerRadiusRatio")) {
		const auto& ratio = json["grainInnerRadiusRatio"];
		if (ratio.IsFloat()) {
			m_properties.grainInnerRadiusRatio = ratio.GetFloat();
		}
		else if (ratio.IsArray() && animations) {
			std::vector<float> keyframes(ratio.Size());
			for (int i = 0; i < static_cast<int>(ratio.Size()); ++i) {
				keyframes[i] = ratio[i].GetFloat();
			}
			animations->addAnimation([keyframes, this](float time, int frame) {
				int i = std::min(std::max(0, frame), static_cast<int>(keyframes.size()));
				m_properties.grainInnerRadiusRatio = keyframes[i];
			});
		}
	}

	if (jrOption(json, "outputStats", m_outputStats, m_outputStats)) {
		m_outputStats = std::regex_replace(m_outputStats, std::regex("\\$BASEFILE"), env.baseFile);
		m_outputStats = ResourceManager::resolveResourcePath(m_outputStats);
	}
	
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
#define step(Step, STEP_DEFINE) \
	m_cullingShaderNames[Step] = baseCullingShaderName + "_" + #STEP_DEFINE; \
	ShaderPool::AddShaderVariant(m_cullingShaderNames[Step], baseCullingShaderName, #STEP_DEFINE);
		step(MarkCulling, STEP_MARK_CULLING);
		step(Group, STEP_GROUP);
		step(BuildCommandBuffer, STEP_BUILD_COMMAND_BUFFER);
#undef step
	}

	return true;
}

void SandRenderer::start()
{
	m_impostorShader = ShaderPool::GetShader(m_impostorShaderName);
	m_shadowMapImpostorShader = ShaderPool::GetShader(m_shadowMapImpostorShaderName);
	for (const auto& name : m_cullingShaderNames) {
		m_cullingShaders.push_back(ShaderPool::GetShader(name));
	}
	m_instanceCloudShader = ShaderPool::GetShader(m_instanceCloudShaderName);
	m_prefixSumShader = ShaderPool::GetShader(m_prefixSumShaderName);
	m_occlusionCullingShader = ShaderPool::GetShader(m_occlusionCullingShaderName);

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

	m_grainMeshData = getComponent<MeshDataBehavior>();
	m_transform = getComponent<TransformBehavior>();
	m_pointData = getComponent<PointCloudDataBehavior>();

	// Allocate element buffers
	GLuint effectivePointCount = 0;
	if (auto pointData = m_pointData.lock()) {
		effectivePointCount = static_cast<GLuint>(pointData->pointCount() / pointData->frameCount());
	}
	else {
		ERR_LOG << "Could not find PointCloudDataBehavior component (required by SandRenderer)";
	}

	size_t elementBufferCount = m_cullingMechanism == AtomicSum ? 1 : (m_cullingMechanism == PrefixSum ? 3 : 2);
	for (size_t i = 0; i < elementBufferCount; ++i) {
		auto buff = std::make_unique<GlBuffer>(GL_ELEMENT_ARRAY_BUFFER);
		buff->addBlock<GLuint>(effectivePointCount);
		buff->alloc();
		buff->finalize(); // This buffer will never be mapped on CPU
		m_elementBuffers.push_back(std::move(buff));
	}

	m_isDeferredRendered = true;

	if (!m_outputStats.empty()) {
		m_outputStatsFile.open(m_outputStats);
		m_outputStatsFile << "frame;impostorCount;instanceCount;grainInnerRadiusRatio\n";
	}
}

void SandRenderer::update(float time, int frame)
{
	m_time = time;
}

void SandRenderer::onPostRender(float time, int frame)
{
	if (m_outputStatsFile.is_open()) {
		m_outputStatsFile
			<< frame << ";"
			<< m_renderInfo.impostorCount << ";"
			<< m_renderInfo.instanceCount << ";"
			<< m_properties.grainInnerRadiusRatio << "\n";
	}
}

void SandRenderer::render(const Camera & camera, const World & world, RenderType target) const
{
	m_occlusionCullingMap = camera.getExtraFramebuffer();

	if (m_cullingMechanism == PrefixSum) {
		renderCullingPrefixSum(camera, world);
	} else {
		WARN_LOG << "Non-prefix-sum based culling is depreciated";
		renderCulling(camera, world);
	}

	if (!m_properties.disableImpostors) {
		renderImpostors(camera, world, target);
	}

	if (!m_properties.disableInstances) {
		renderInstances(camera, world, target);
	}

	camera.releaseExtraFramebuffer(m_occlusionCullingMap);
	m_occlusionCullingMap.reset();
}

///////////////////////////////////////////////////////////////////////////////
// private members
///////////////////////////////////////////////////////////////////////////////

glm::mat4 SandRenderer::modelMatrix() const {
	if (auto transform = m_transform.lock()) {
		return transform->modelMatrix();
	} else {
		return glm::mat4(1.0f);
	}
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

///////////////////////////////////////////////////////////////////////////
// Culling
void SandRenderer::renderCulling(const Camera & camera, const World & world) const
{
	if (auto pointData = m_pointData.lock()) {
		GLuint effectivePointCount = static_cast<GLuint>(pointData->pointCount() / pointData->frameCount());

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
		pointData->data().bindSsbo(ssboIndex++);
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
			}
			else if (pointers->nextImpostorElement == 0) {
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
}

void SandRenderer::renderCullingPrefixSum(const Camera & camera, const World & world) const
{
	int resultIndex;
	if (!m_cullingShaders[MarkCulling] || !m_cullingShaders[MarkCulling]->isValid()
		|| !m_cullingShaders[BuildCommandBuffer] || !m_cullingShaders[BuildCommandBuffer]
		|| !m_cullingShaders[Group]->isValid() || !m_occlusionCullingShader
		|| !m_occlusionCullingShader->isValid()) {
		return;
	}

	if (auto pointData = m_pointData.lock()) {
		GLuint effectivePointCount = static_cast<GLuint>(pointData->pointCount() / pointData->frameCount());
		
		// 0. Occlusion culling map (kind of shadow map)
		if (m_properties.grainInnerRadiusRatio > 0) {
			GLint drawFbo = 0, readFbo = 0;
			glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo);
			glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFbo);
			m_occlusionCullingMap->bind();
			glClearColor(0, 0, 0, 1);
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

			const ShaderProgram & shader = *m_occlusionCullingShader;
			shader.use();
			shader.bindUniformBlock("Camera", camera.ubo());
			shader.setUniform("modelMatrix", modelMatrix());
			shader.setUniform("viewModelMatrix", camera.viewMatrix() * modelMatrix());

			shader.setUniform("uOuterOverInnerRadius", 1.0f / m_properties.grainInnerRadiusRatio);
			shader.setUniform("uInnerRadius", m_properties.grainInnerRadiusRatio * m_properties.grainRadius);
			shader.setUniform("uPointCount", static_cast<GLuint>(pointData->pointCount()));
			shader.setUniform("uFrameCount", static_cast<GLuint>(pointData->frameCount()));
			shader.setUniform("uTime", static_cast<GLfloat>(m_time));

			glEnable(GL_PROGRAM_POINT_SIZE);

			pointData->data().bindSsbo(1);

			glBindVertexArray(pointData->vao());
			glDrawArrays(GL_POINTS, 0, effectivePointCount);
			glBindVertexArray(0);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFbo);
		}
		
		// 1. Mark instances
		{
			const ShaderProgram & shader = *m_cullingShaders[MarkCulling];
			shader.use();
			shader.setUniform("uType", static_cast<GLuint>(0));
			shader.bindUniformBlock("Camera", camera.ubo());
			shader.setUniform("modelMatrix", modelMatrix());
			shader.setUniform("viewModelMatrix", camera.viewMatrix() * modelMatrix());
			shader.setUniform("uPointCountPerFrame", effectivePointCount);
			shader.setUniform("uInstanceLimit", static_cast<GLfloat>(m_properties.instanceLimit));
			shader.setUniform("uOuterOverInnerRadius", 1.f / m_properties.grainInnerRadiusRatio);
			shader.setUniform("uInnerRadius", m_properties.grainInnerRadiusRatio * m_properties.grainRadius);
			shader.setUniform("uOuterRadius", m_properties.grainRadius);
			shader.setUniform("uPointCount", static_cast<GLuint>(pointData->pointCount()));
			shader.setUniform("uFrameCount", static_cast<GLuint>(pointData->frameCount()));
			shader.setUniform("uTime", static_cast<GLfloat>(m_time));
			shader.setUniform("uEnableOcclusionCulling", m_properties.enableOcclusionCulling);
			shader.setUniform("uEnableFrustumCulling", m_properties.enableFrustumCulling);
			shader.setUniform("uEnableDistanceCulling", m_properties.enableDistanceCulling);
			glBindTextureUnit(0, m_occlusionCullingMap->colorTexture(0));
			shader.setUniform("occlusionMap", 0);
			m_prefixSumInfoSsbo->bindSsbo(0);
			pointData->data().bindSsbo(1);
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
			shader.setUniform("uPointCountPerFrame", effectivePointCount);
			shader.setUniform("uType", static_cast<GLuint>(0));
			m_prefixSumInfoSsbo->bindSsbo(0);
			m_elementBuffers[1 + resultIndex]->bindSsbo(1);
			m_elementBuffers[0]->bindSsbo(2);
			glDispatchCompute(static_cast<GLuint>((effectivePointCount + 127) / 128), 1, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}

		// 4. Mark impostors
		{
			const ShaderProgram & shader = *m_cullingShaders[MarkCulling];
			shader.use();
			shader.setUniform("uType", static_cast<GLuint>(1));
			shader.bindUniformBlock("Camera", camera.ubo());
			shader.setUniform("modelMatrix", modelMatrix());
			shader.setUniform("viewModelMatrix", camera.viewMatrix() * modelMatrix());
			shader.setUniform("uPointCountPerFrame", effectivePointCount);
			shader.setUniform("uInstanceLimit", static_cast<GLfloat>(m_properties.instanceLimit));
			shader.setUniform("uOuterOverInnerRadius", 1.f / m_properties.grainInnerRadiusRatio);
			shader.setUniform("uInnerRadius", m_properties.grainInnerRadiusRatio * m_properties.grainRadius);
			shader.setUniform("uOuterRadius", m_properties.grainRadius);
			shader.setUniform("uPointCount", static_cast<GLuint>(pointData->pointCount()));
			shader.setUniform("uFrameCount", static_cast<GLuint>(pointData->frameCount()));
			shader.setUniform("uTime", static_cast<GLfloat>(m_time));
			shader.setUniform("uEnableOcclusionCulling", m_properties.enableOcclusionCulling);
			shader.setUniform("uEnableFrustumCulling", m_properties.enableFrustumCulling);
			shader.setUniform("uEnableDistanceCulling", m_properties.enableDistanceCulling);
			glBindTextureUnit(0, m_occlusionCullingMap->colorTexture(0));
			shader.setUniform("occlusionMap", 0);
			m_prefixSumInfoSsbo->bindSsbo(0);
			pointData->data().bindSsbo(1);
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
			shader.setUniform("uPointCountPerFrame", effectivePointCount);
			shader.setUniform("uType", static_cast<GLuint>(1));
			m_prefixSumInfoSsbo->bindSsbo(0);
			m_elementBuffers[1 + resultIndex]->bindSsbo(1);
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
			shader.setUniform("uPointCountPerFrame", effectivePointCount);
			shader.setUniform("uGrainMeshPointCount", grainMeshPointCount);
			m_prefixSumInfoSsbo->bindSsbo(0);
			m_commandBuffer->bindSsbo(1);
			glDispatchCompute(1, 1, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}

		// 8. Collect info
		if (m_prefixSumInfoSsbo) {
			m_prefixSumInfoSsbo->readBlock<PrefixSumInfoSsbo>(0, [this](PrefixSumInfoSsbo *ssbo, size_t _) {
				m_renderInfo.instanceCount = ssbo->instanceCount;
				m_renderInfo.impostorCount = ssbo->impostorCount;
			});
		}
	}
}

///////////////////////////////////////////////////////////////////////////
// Impostors drawing
void SandRenderer::renderImpostors(const Camera & camera, const World & world, RenderType target) const
{
	if (auto pointData = m_pointData.lock()) {
		glEnable(GL_PROGRAM_POINT_SIZE);
		if (m_cullingMechanism == RestartPrimitive) {
			glEnable(GL_PRIMITIVE_RESTART);
			glPrimitiveRestartIndex(0xFFFFFFF0);
		}

		if (m_properties.renderAdditive) {
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
		} else {
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
		}

		const ShaderProgram & shader = target == ShadowMapRendering ? *m_shadowMapImpostorShader : *m_impostorShader;
		shader.use();

		// Set uniforms
		shader.bindUniformBlock("Camera", camera.ubo());
		shader.setUniform("modelMatrix", modelMatrix());
		shader.setUniform("viewModelMatrix", camera.viewMatrix() * modelMatrix());
		shader.setUniform("uOuterRadius", static_cast<GLfloat>(m_properties.grainRadius));
		shader.setUniform("uInnerRadius", static_cast<GLfloat>(m_properties.grainRadius * m_properties.grainInnerRadiusRatio));
		shader.setUniform("uPointCount", static_cast<GLuint>(pointData->pointCount()));
		shader.setUniform("uFrameCount", static_cast<GLuint>(pointData->frameCount()));
		shader.setUniform("uTime", static_cast<GLfloat>(m_time));
		shader.setUniform("uHasMetallicRoughnessMap", m_properties.hasMetallicRoughnessMap);

		GLint o = 0;

		glBindTextureUnit(static_cast<GLuint>(o), m_occlusionCullingMap->colorTexture(0));
		shader.setUniform("occlusionMap", o++);

		if (m_colormapTexture) {
			m_colormapTexture->bind(o);
			shader.setUniform("colormapTexture", o++);
		}

		// TODO: Use UBO
		for (size_t k = 0; k < m_normalAlphaTextures.size(); ++k) {
			GLuint n = m_normalAlphaTextures[k]->depth();
			GLuint viewCount = static_cast<GLuint>(sqrt(n / 2));

			shader.setUniform(MAKE_STR("impostor[" << k << "].viewCount"), viewCount);

			m_normalAlphaTextures[k]->bind(o);
			shader.setUniform(MAKE_STR("impostor[" << k << "].normalAlphaTexture"), o++);

			m_leanTextures[k]->lean1.bind(o);
			shader.setUniform(MAKE_STR("impostor[" << k << "].lean1Texture"), o++);

			m_leanTextures[k]->lean2.bind(o);
			shader.setUniform(MAKE_STR("impostor[" << k << "].lean2Texture"), o++);

			if (m_baseColorTextures.size() > k) {
				m_baseColorTextures[k]->bind(o);
				shader.setUniform(MAKE_STR("impostor[" << k << "].baseColorTexture"), o++);
			}

			if (m_metallicRoughnessTextures.size() > k) {
				m_metallicRoughnessTextures[k]->bind(o);
				shader.setUniform(MAKE_STR("impostor[" << k << "].metallicRoughnesTexture"), o++);
			}
		}

		glBindVertexArray(pointData->vao());
		pointData->data().bindSsbo(1);
		m_commandBuffer->bind();
		m_elementBuffers[0]->bind();
		glDrawElementsIndirect(GL_POINTS, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
	}
}

///////////////////////////////////////////////////////////////////////////
// Instances drawing
void SandRenderer::renderInstances(const Camera & camera, const World & world, RenderType target) const
{
	if (auto mesh = m_grainMeshData.lock()) {
		if (auto pointData = m_pointData.lock()) {

			if (m_cullingMechanism == RestartPrimitive) {
				glEnable(GL_PRIMITIVE_RESTART);
				glPrimitiveRestartIndex(0xFFFFFFF0);
			}

			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);

			m_instanceCloudShader->use();

			// Set uniforms
			m_instanceCloudShader->bindUniformBlock("Camera", camera.ubo());
			m_instanceCloudShader->setUniform("modelMatrix", modelMatrix());
			m_instanceCloudShader->setUniform("viewModelMatrix", camera.viewMatrix() * modelMatrix());
			m_instanceCloudShader->setUniform("invViewMatrix", inverse(camera.viewMatrix()));

			m_instanceCloudShader->setUniform("grainRadius", static_cast<GLfloat>(m_properties.grainRadius));
			m_instanceCloudShader->setUniform("grainMeshScale", static_cast<GLfloat>(m_properties.grainMeshScale));

			m_instanceCloudShader->setUniform("uPointCount", static_cast<GLuint>(pointData->pointCount()));
			m_instanceCloudShader->setUniform("uFrameCount", static_cast<GLuint>(pointData->frameCount()));
			m_instanceCloudShader->setUniform("uTime", static_cast<GLfloat>(m_time));

			GLint o = 0;
			if (m_colormapTexture) {
				m_colormapTexture->bind(o);
				m_instanceCloudShader->setUniform("colormapTexture", o++);
			}

			GLuint matId = 0;
			for (const auto& mat : m_instanceMaterials) {
				o = mat.setUniforms(*m_instanceCloudShader, matId, o);
				++matId;
			}

			// Render
			glBindVertexArray(mesh->vao());
			pointData->data().bindSsbo(1);
			m_elementBuffers[m_cullingMechanism == RestartPrimitive ? 1 : 0]->bindSsbo(2);
			m_commandBuffer->bind();
			glDrawArraysIndirect(GL_TRIANGLES, static_cast<void*>(static_cast<DrawElementsIndirectCommand*>(nullptr) + 1));
			glBindVertexArray(0);
		}
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

