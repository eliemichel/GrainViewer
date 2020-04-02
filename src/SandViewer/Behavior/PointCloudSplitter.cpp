#include "PointCloudSplitter.h"

// TODO find a way to use reflection or behavior registry to avoid enumerating all possible parent of IPointCloudData
#include "PointCloudDataBehavior.h"
#include "Sand6Data.h"

#include "ShaderPool.h"
#include "utils/jsonutils.h"

#include <magic_enum.hpp>

///////////////////////////////////////////////////////////////////////////////

bool PointCloudSplitter::deserialize(const rapidjson::Value& json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);
	return true;
}

void PointCloudSplitter::start()
{
	m_pointData = getComponent<PointCloudDataBehavior>();
	if (m_pointData.expired()) m_pointData = getComponent<Sand6Data>();
	if (m_pointData.expired()) {
		WARN_LOG << "PointCloudSplitter could not find point data (ensure that there is a PointCloudDataBehavior or Sand6Data attached to the same object)";
		return;
	}

	// Load shaders
	m_shader = ShaderPool::GetShader(m_shaderName);
	// This is a bit dirty, only ShaderPool is supposed to call load(), but it
	// has no mechanism (yet) to set snippets. We assume that all objects using
	// this shader will use the same value for LOCAL_SIZE_X.
	m_shader->setSnippet("settings", "#define LOCAL_SIZE_X " + std::to_string(m_local_size_x));
	m_shader->load();

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
}

void PointCloudSplitter::onPreRender(const Camera& camera, const World& world, RenderType target)
{
	auto pointData = m_pointData.lock();
	if (!pointData) return;

	// Split element buffer
	m_shader->use();
	m_shader->setUniform("uPointCount", m_elementCount);
	m_shader->setUniform("uRenderModelCount", static_cast<GLuint>(magic_enum::enum_count<RenderModel>()));

	m_countersSsbo->bindSsbo(0);
	// m_renderTypeSsbo.bindSsbo(1);
	m_elementBuffer->bindSsbo(2);

	for (GLuint i = 0; i < 4; ++i) {
		m_shader->setUniform("uStep", i);
		glDispatchCompute(i == 0 || i == 2 ? 1 : static_cast<GLuint>(m_xWorkGroups), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	// Get counters back
	m_countersSsbo->exportBlock(0, m_counters);
}

std::shared_ptr<PointCloudView> PointCloudSplitter::subPointCloud(RenderModel model) const
{
	return std::make_shared<PointCloudView>(*this, model);
}

GLint PointCloudSplitter::pointOffset(RenderModel model) const
{
	return static_cast<GLint>(m_counters[static_cast<int>(model)].offset);
}

GLsizei PointCloudSplitter::pointCount(RenderModel model) const
{
	return static_cast<GLsizei>(m_counters[static_cast<int>(model)].count);
}

GLsizei PointCloudSplitter::frameCount(RenderModel model) const
{
	if (auto pointData = m_pointData.lock()) {
		return pointData->frameCount();
	}
	else {
		return 0;
	}
}

GLuint PointCloudSplitter::vao(RenderModel model) const
{
	if (auto pointData = m_pointData.lock()) {
		return pointData->vao();
	}
	else {
		return 0;
	}
}
