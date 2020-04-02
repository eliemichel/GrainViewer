#include "PointCloudSplitter.h"

// TODO find a way to use reflection or behavior registry to avoid enumerating all possible parent of IPointCloudData
#include "PointCloudDataBehavior.h"
#include "Sand6Data.h"

#include "ShaderPool.h"
#include "utils/jsonutils.h"
#include "utils/behaviorutils.h"

#include <magic_enum.hpp>

///////////////////////////////////////////////////////////////////////////////

bool PointCloudSplitter::deserialize(const rapidjson::Value& json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);
	autoDeserialize(json, m_properties);
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

	m_countersSsbo->bindSsbo(0);
	// m_renderTypeSsbo.bindSsbo(1);
	m_elementBuffer->bindSsbo(2);

	StepShaderVariant firstStep = StepShaderVariant::STEP_RESET;
	if (properties().renderTypeCaching == RenderTypeCaching::Precompute) {
		firstStep = StepShaderVariant::STEP_PRECOMPUTE;
	}

	constexpr StepShaderVariant lastStep = lastValue<StepShaderVariant>();
	for (int i = static_cast<int>(firstStep); i <= static_cast<int>(lastStep); ++i) {
		const ShaderProgram& shader = *getShader(properties().renderTypeCaching, i);
		autoSetUniforms(shader, properties());
		shader.setUniform("uPointCount", m_elementCount);
		shader.setUniform("uRenderModelCount", static_cast<GLuint>(magic_enum::enum_count<RenderModel>()));
		shader.use();
		glDispatchCompute(i == 0 || i == 2 ? 1 : static_cast<GLuint>(m_xWorkGroups), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	// Get counters back
	m_countersSsbo->exportBlock(0, m_counters);
}

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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
