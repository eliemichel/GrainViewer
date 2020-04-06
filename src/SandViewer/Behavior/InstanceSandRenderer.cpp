
#include "InstanceSandRenderer.h"
#include "TransformBehavior.h"
#include "MeshDataBehavior.h"
#include "SandBehavior.h"
#include "IPointCloudData.h"
#include "ShaderPool.h"
#include "ResourceManager.h"
#include "BehaviorRegistry.h"

#include "utils/jsonutils.h"
#include "utils/behaviorutils.h"

bool InstanceSandRenderer::deserialize(const rapidjson::Value & json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);
	std::string colormap;
	if (jrOption(json, "colormap", colormap)) {
		m_colormapTexture = ResourceManager::loadTexture(colormap);
	}
	jrArray(json, "materials", m_materials);
	autoDeserialize(json, m_properties);
	return true;
}

void InstanceSandRenderer::start()
{
	m_transform = getComponent<TransformBehavior>();
	m_sand = getComponent<SandBehavior>();
	m_mesh = getComponent<MeshDataBehavior>();
	m_pointData = BehaviorRegistry::getPointCloudDataComponent(*this, PointCloudSplitter::RenderModel::Instance);

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

	int n = static_cast<int>(std::max(mesh->materials().size(), m_materials.size()));
	for (int i = 0; i < n; ++i) {
		const StandardMaterial& mat = i < m_materials.size() ? m_materials[i] : mesh->materials()[i];
		o = mat.setUniforms(*m_shader, MAKE_STR("uMaterial[" << i << "]."), o);
	}

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

