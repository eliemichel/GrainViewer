
#include "InstanceSandRenderer.h"
#include "TransformBehavior.h"
#include "MeshDataBehavior.h"
#include "SandBehavior.h"
#include "IPointCloudData.h"
#include "ShaderPool.h"
#include "ResourceManager.h"

// TODO find a way to use reflection or behavior registry to avoid enumerating all possible parent of IPointCloudData
#include "PointCloudSplitter.h"
#include "PointCloudDataBehavior.h"
#include "Sand6Data.h"

#include "utils/jsonutils.h"
#include "utils/behaviorutils.h"

bool InstanceSandRenderer::deserialize(const rapidjson::Value & json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);
	std::string colormap;
	if (jrOption(json, "colormap", colormap)) {
		m_colormapTexture = ResourceManager::loadTexture(colormap);
	}
	autoDeserialize(json, m_properties);
	return true;
}

void InstanceSandRenderer::start()
{
	m_transform = getComponent<TransformBehavior>();
	m_sand = getComponent<SandBehavior>();
	m_mesh = getComponent<MeshDataBehavior>();

	// The following block is duplicated in other sand/point rendering components
	if (auto splitter = getComponent<PointCloudSplitter>().lock()) {
		m_pointData = splitter->subPointCloud(PointCloudSplitter::RenderModel::Instance);
	}
	if (m_pointData.expired()) m_pointData = getComponent<PointCloudDataBehavior>();
	if (m_pointData.expired()) m_pointData = getComponent<Sand6Data>();
	if (m_pointData.expired()) {
		WARN_LOG
			<< "InstanceSandRenderer could not find point data "
			<< "(ensure that there is one of PointCloudSplitter, "
			<< "PointCloudDataBehavior or Sand6Data attached to the same"
			<< "object)";
	}

	m_shader = ShaderPool::GetShader(m_shaderName);
}

void InstanceSandRenderer::update(float time, int frame)
{
	m_time = time;
}

void InstanceSandRenderer::render(const Camera& camera, const World& world, RenderType target) const
{
	auto mesh = m_mesh.lock();
	auto pointData = m_pointData.lock();
	if (!mesh || !pointData || pointData->pointCount() == 0) return;

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	const ShaderProgram & shader = *m_shader; // maybe switch shader depending on option/render target

	glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
	shader.bindUniformBlock("Camera", camera.ubo());
	shader.setUniform("modelMatrix", modelMatrix());
	shader.setUniform("viewModelMatrix", viewModelMatrix);

	autoSetUniforms(shader, properties());
	if (auto sand = m_sand.lock()) {
		autoSetUniforms(shader, sand->properties());
	}

	shader.setUniform("uPointCount", static_cast<GLuint>(pointData->pointCount()));
	shader.setUniform("uFrameCount", static_cast<GLuint>(pointData->frameCount()));
	shader.setUniform("uTime", static_cast<GLfloat>(m_time));

	GLint o = 0;
	if (m_colormapTexture) {
		m_colormapTexture->bind(o);
		shader.setUniform("uColormapTexture", o++);
	}

	// TODO: Get from mesh data component
	/*
	GLuint matId = 0;
	for (const auto& mat : m_instanceMaterials) {
		o = mat.setUniforms(shader, matId, o);
		++matId;
	}
	*/

	shader.use();

	glBindVertexArray(mesh->vao());
	pointData->vbo().bindSsbo(0);
	if (auto pointElements = pointData->ebo()) {
		pointElements->bindSsbo(1);
		shader.setUniform("uUsePointElements", true);
	}
	else {
		shader.setUniform("uUsePointElements", false);
	}
	glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, mesh->pointCount(), pointData->pointCount(), pointData->pointOffset());

	glBindVertexArray(0);
}


//-----------------------------------------------------------------------------

glm::mat4 InstanceSandRenderer::modelMatrix() const
{
	if (auto transform = m_transform.lock()) {
		return transform->modelMatrix();
	} else {
		return glm::mat4(1.0f);
	}
}

