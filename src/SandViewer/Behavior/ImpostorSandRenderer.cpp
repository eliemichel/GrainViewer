
#include "ImpostorSandRenderer.h"
#include "TransformBehavior.h"
#include "ShaderPool.h"

#include "utils/jsonutils.h"
#include "utils/behaviorutils.h"

bool ImpostorSandRenderer::deserialize(const rapidjson::Value & json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);
	autoDeserialize(json, m_properties);
	return true;
}

void ImpostorSandRenderer::start()
{
	m_transform = getComponent<TransformBehavior>();
	m_shader = ShaderPool::GetShader(m_shaderName);
}

void ImpostorSandRenderer::update(float time, int frame)
{
	
}

void ImpostorSandRenderer::render(const Camera& camera, const World& world, RenderType target) const
{
	const ShaderProgram & shader = *m_shader; // maybe switch shader depending on option/render target
	glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
	shader.bindUniformBlock("Camera", camera.ubo());
	shader.setUniform("modelMatrix", modelMatrix());
	shader.setUniform("viewModelMatrix", viewModelMatrix);

	autoSetUniforms(shader, properties());
	shader.use();

	// some draw call here!
}


//-----------------------------------------------------------------------------

glm::mat4 ImpostorSandRenderer::modelMatrix() const
{
	if (auto transform = m_transform.lock()) {
		return transform->modelMatrix();
	} else {
		return glm::mat4(1.0f);
	}
}

