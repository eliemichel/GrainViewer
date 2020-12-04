/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#include "PointCloudSplitter.h"
#include "TransformBehavior.h"
#include "GrainBehavior.h"
#include "BehaviorRegistry.h"
#include "utils/ScopedFramebufferOverride.h"
#include "ShaderPool.h"
#include "utils/jsonutils.h"
#include "utils/behaviorutils.h"
#include "Framebuffer.h"
#include "GlobalTimer.h"
#include "ResourceManager.h"
#include "PointCloudView.h"

#include <magic_enum.hpp>

#include <filesystem>
namespace fs = std::filesystem;

//-----------------------------------------------------------------------------

bool PointCloudSplitter::deserialize(const rapidjson::Value& json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);
	jrOption(json, "occlusionCullingShader", m_occlusionCullingShaderName, m_occlusionCullingShaderName);
	autoDeserialize(json, m_properties);

	if (jrOption(json, "outputStats", m_outputStats)) {
		initStats();
	}

	return true;
}

void PointCloudSplitter::start()
{
	m_transform = getComponent<TransformBehavior>();
	m_grain = getComponent<GrainBehavior>();
	m_pointData = BehaviorRegistry::getPointCloudDataComponent(*this);

	// Initialize element buffer
	auto pointData = m_pointData.lock();
	m_elementCount = static_cast<GLuint>(pointData->pointCount() / pointData->frameCount());
	
	m_elementBuffer = std::make_unique<GlBuffer>(GL_ELEMENT_ARRAY_BUFFER);
	m_elementBuffer->addBlock<GLuint>(m_elementCount);
	m_elementBuffer->alloc();
	m_elementBuffer->finalize();

	// Small extra buffer to count the number of elements for each model
	m_counters.resize(magic_enum::enum_count<RenderModel>());
	m_countersSsbo = std::make_unique<GlBuffer>(GL_ELEMENT_ARRAY_BUFFER);
	m_countersSsbo->importBlock(m_counters);

	m_xWorkGroups = (m_elementCount + (m_local_size_x - 1)) / m_local_size_x;

	// Create proxies to sub parts of the output point clouds
	m_subClouds.resize(magic_enum::enum_count<RenderModel>());
	for (int i = 0; i < m_subClouds.size(); ++i) {
		m_subClouds[i] = std::make_shared<PointCloudView>(*this, static_cast<RenderModel>(i));
	}

	// Shader (other shaders are lazy loaded by getShader())
	m_occlusionCullingShader = ShaderPool::GetShader(m_occlusionCullingShaderName);
}

void PointCloudSplitter::update(float time, int frame)
{
	m_time = time;
}

void PointCloudSplitter::onPreRender(const Camera& camera, const World& world, RenderType target)
{
	ScopedTimer timer((target == RenderType::ShadowMap ? "PointCloudSplitter_shadowmap" : "PointCloudSplitter"));

	auto pointData = m_pointData.lock();
	if (!pointData) return;

	const auto& props = properties();

	std::shared_ptr<Framebuffer> occlusionCullingFbo;
	
	// 1. Occlusion culling map (kind of shadow map)
	if (props.enableOcclusionCulling) {
		ScopedFramebufferOverride scoppedFramebufferOverride;

		glEnable(GL_PROGRAM_POINT_SIZE);

		occlusionCullingFbo = camera.getExtraFramebuffer(Camera::ExtraFramebufferOption::Rgba32fDepth);
		occlusionCullingFbo->bind();
		glClearColor(0, 0, 0, 1);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		// 1.1. z-prepass (optional)
		if (props.zPrepass) {
			occlusionCullingFbo->deactivateColorAttachments();
			const ShaderProgram& shader = *m_occlusionCullingShader;

			setCommonUniforms(shader, camera);

			shader.use();
			glBindVertexArray(pointData->vao());
			pointData->vbo().bindSsbo(1);
			glDrawArrays(GL_POINTS, 0, m_elementCount);
			glBindVertexArray(0);

			glTextureBarrier();
			occlusionCullingFbo->activateColorAttachments();
		}
		// 1.2. core pass
		{
			ScopedFramebufferOverride scoppedFramebufferOverride;

			glEnable(GL_PROGRAM_POINT_SIZE);
			if (props.zPrepass) {
				glDepthMask(GL_FALSE);
				glDepthFunc(GL_LEQUAL);
			}

			occlusionCullingFbo = camera.getExtraFramebuffer(Camera::ExtraFramebufferOption::Rgba32fDepth);
			occlusionCullingFbo->bind();
			glClearColor(0, 0, 0, 1);
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

			const ShaderProgram& shader = *m_occlusionCullingShader;

			setCommonUniforms(shader, camera);

			shader.use();
			glBindVertexArray(pointData->vao());
			pointData->vbo().bindSsbo(1);
			glDrawArrays(GL_POINTS, 0, m_elementCount);
			glBindVertexArray(0);

			glTextureBarrier();
			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LESS);
		}
	}

	// 2. Splitting
	{
		m_countersSsbo->bindSsbo(0);
		m_elementBuffer->bindSsbo(2);
		pointData->vbo().bindSsbo(3);

		if (props.renderTypeCaching != RenderTypeCaching::Forget) {
			// Cache for render types
			if (!m_renderTypeCache) {
				m_renderTypeCache = std::make_unique<GlBuffer>(GL_ELEMENT_ARRAY_BUFFER);
				m_renderTypeCache->addBlock<GLuint>(m_elementCount);
				m_renderTypeCache->alloc();
				m_renderTypeCache->finalize();
			}
			m_renderTypeCache->bindSsbo(1);
		}

		StepShaderVariant firstStep = StepShaderVariant::STEP_RESET;
		if (props.renderTypeCaching == RenderTypeCaching::Precompute) {
			firstStep = StepShaderVariant::STEP_PRECOMPUTE;
		}

		constexpr StepShaderVariant lastStep = lastValue<StepShaderVariant>();
		constexpr int STEP_RESET = static_cast<int>(StepShaderVariant::STEP_RESET);
		constexpr int STEP_OFFSET = static_cast<int>(StepShaderVariant::STEP_OFFSET);
		for (int i = static_cast<int>(firstStep); i <= static_cast<int>(lastStep); ++i) {
			const ShaderProgram& shader = *getShader(props.renderTypeCaching, i);
			setCommonUniforms(shader, camera);
			if (props.enableOcclusionCulling) {
				glBindTextureUnit(0, occlusionCullingFbo->colorTexture(0));
				shader.setUniform("uOcclusionMap", 0);
			}
			shader.use();
			glDispatchCompute(i == STEP_RESET || i == STEP_OFFSET ? 1 : static_cast<GLuint>(m_xWorkGroups), 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}

		// Get counters back
		m_countersSsbo->exportBlock(0, m_counters);
	}

	writeStats();
}

//-----------------------------------------------------------------------------

std::shared_ptr<PointCloudView> PointCloudSplitter::subPointCloud(RenderModel model) const
{
	return m_subClouds[static_cast<int>(model)];
}

GLsizei PointCloudSplitter::pointCount(RenderModel model) const
{
	return static_cast<GLsizei>(m_counters[static_cast<int>(model)].count);
}

GLsizei PointCloudSplitter::frameCount(RenderModel model) const
{
	return 1; // renderers must not animate it again, this was already taken care of in PointCloudSplitter
}

GLuint PointCloudSplitter::vao(RenderModel model) const
{
	auto pointData = m_pointData.lock();
	assert(pointData);
	return pointData->vao();
}

const GlBuffer& PointCloudSplitter::vbo(RenderModel model) const
{
	auto pointData = m_pointData.lock();
	assert(pointData);
	return pointData->vbo();
}

std::shared_ptr<GlBuffer> PointCloudSplitter::ebo(RenderModel model) const
{
	return m_elementBuffer;
}

GLint PointCloudSplitter::pointOffset(RenderModel model) const
{
	return static_cast<GLint>(m_counters[static_cast<int>(model)].offset);
}

//-----------------------------------------------------------------------------

glm::mat4 PointCloudSplitter::modelMatrix() const {
	if (auto transform = m_transform.lock()) {
		return transform->modelMatrix();
	}
	else {
		return glm::mat4(1.0f);
	}
}

void PointCloudSplitter::setCommonUniforms(const ShaderProgram& shader, const Camera& camera) const
{
	glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
	shader.bindUniformBlock("Camera", camera.ubo());
	shader.setUniform("modelMatrix", modelMatrix());
	shader.setUniform("viewModelMatrix", viewModelMatrix);


	autoSetUniforms(shader, properties());
	if (auto grain = m_grain.lock()) {
		autoSetUniforms(shader, grain->properties());
		shader.setUniform("uOuterOverInnerRadius", 1.0f / grain->properties().grainInnerRadiusRatio);
	}

	shader.setUniform("uPointCount", m_elementCount);
	shader.setUniform("uRenderModelCount", static_cast<GLuint>(magic_enum::enum_count<RenderModel>()));
	shader.setUniform("uFrameCount", static_cast<GLuint>(m_pointData.lock()->frameCount()));
	shader.setUniform("uTime", m_time);
}

std::shared_ptr<ShaderProgram> PointCloudSplitter::getShader(RenderTypeCaching renderType, int step) const
{
	return getShader(static_cast<RenderTypeShaderVariant>(renderType), static_cast<StepShaderVariant>(step));
}

std::shared_ptr<ShaderProgram> PointCloudSplitter::getShader(RenderTypeShaderVariant renderType, StepShaderVariant step) const
{
	constexpr size_t n1 = magic_enum::enum_count<RenderTypeShaderVariant>();
	constexpr size_t n2 = magic_enum::enum_count<StepShaderVariant>();
	if (m_shaders.empty()) {
		m_shaders.resize(n1 * n2);
	}

	int i1 = static_cast<int>(renderType);
	int i2 = static_cast<int>(step);
	int index = i1 + n1 * i2;

	if (!m_shaders[index]) {
		// Lazy loading of shader variants
		std::string variantName = m_shaderName + "_RenderType" + std::to_string(i1) + "_Step" + std::to_string(i2);

		std::vector<std::string> defines;
		defines.push_back(std::string(magic_enum::enum_name(renderType)));
		defines.push_back(std::string(magic_enum::enum_name(step)));

		std::map<std::string, std::string> snippets;
		snippets["settings"] = "#define LOCAL_SIZE_X " + std::to_string(m_local_size_x);

		DEBUG_LOG << "loading variant " << variantName;
		ShaderPool::AddShaderVariant(variantName, m_shaderName, defines);
		m_shaders[index] = ShaderPool::GetShader(variantName);
	}
	return m_shaders[index];
}

void PointCloudSplitter::initStats()
{
	m_outputStats = ResourceManager::resolveResourcePath(m_outputStats);
	fs::create_directories(fs::path(m_outputStats).parent_path());
	m_outputStatsFile.open(m_outputStats);
	m_outputStatsFile << "frame;instanceCount;impostorCount;pointCount;noneCount\n";
	m_statFrame = 0;
}

void PointCloudSplitter::writeStats()
{
	if (!m_outputStatsFile.is_open()) return;
	m_outputStatsFile << m_statFrame << ";";

	m_outputStatsFile << m_counters[static_cast<int>(RenderModel::Instance)].count << ";";
	m_outputStatsFile << m_counters[static_cast<int>(RenderModel::Impostor)].count << ";";
	m_outputStatsFile << m_counters[static_cast<int>(RenderModel::Point)].count << ";";
	m_outputStatsFile << m_counters[static_cast<int>(RenderModel::None)].count << "\n";

	++m_statFrame;
}
