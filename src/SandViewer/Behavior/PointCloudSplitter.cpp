#include "PointCloudSplitter.h"
#include "TransformBehavior.h"

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
	m_transform = getComponent<TransformBehavior>();
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

void PointCloudSplitter::update(float time, int frame)
{
	m_time = time;
}

void PointCloudSplitter::onPreRender(const Camera& camera, const World& world, RenderType target)
{
	auto pointData = m_pointData.lock();
	if (!pointData) return;

	m_countersSsbo->bindSsbo(0);
	m_elementBuffer->bindSsbo(2);
	pointData->vbo().bindSsbo(3);

	if (properties().renderTypeCaching != RenderTypeCaching::Forget) {
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
	if (properties().renderTypeCaching == RenderTypeCaching::Precompute) {
		firstStep = StepShaderVariant::STEP_PRECOMPUTE;
	}

	constexpr StepShaderVariant lastStep = lastValue<StepShaderVariant>();
	constexpr int STEP_RESET = static_cast<int>(StepShaderVariant::STEP_RESET);
	constexpr int STEP_OFFSET = static_cast<int>(StepShaderVariant::STEP_OFFSET);
	for (int i = static_cast<int>(firstStep); i <= static_cast<int>(lastStep); ++i) {
		const ShaderProgram& shader = *getShader(properties().renderTypeCaching, i);
		setCommonUniforms(shader, camera);
		shader.use();
		glDispatchCompute(i == STEP_RESET || i == STEP_OFFSET ? 1 : static_cast<GLuint>(m_xWorkGroups), 1, 1);
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
	auto pointData = m_pointData.lock();
	assert(pointData);
	return pointData->frameCount();
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
	shader.setUniform("uOuterOverInnerRadius", properties().outerRadius / properties().innerRadius);

	// shader.setUniform("uOcclusionMap", m_elementCount);

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
