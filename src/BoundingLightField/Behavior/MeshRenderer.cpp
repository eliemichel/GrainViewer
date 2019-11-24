#include "Logger.h"

#include "ResourceManager.h"
#include "ShaderProgram.h"
#include "MeshDataBehavior.h"
#include "MeshRenderer.h"
#include "ShaderPool.h"

bool MeshRenderer::deserialize(const rapidjson::Value& json)
{
	if (json.HasMember("shader")) {
		if (json["shader"].IsString()) {
			m_shaderName = json["shader"].GetString();
		}
		else {
			ERR_LOG << "Field 'shader' of MeshRenderer must be a string";
			return false;
		}
	}

	return true;
}

void MeshRenderer::start()
{
	m_shader = ShaderPool::GetShader(m_shaderName);
	if (!m_shader) {
		WARN_LOG << "Using direct shader name in MeshRenderer is depreciated, use 'shaders' section to define shaders.";
		m_shader = std::make_shared<ShaderProgram>(m_shaderName);
	}

	m_meshData = getComponent<MeshDataBehavior>();
	m_modelMatrix = glm::mat4(1.0);
}

void MeshRenderer::render(const Camera& camera, const World& world, RenderType target) const
{
	if (!m_shader->isValid()) return;

	if (auto mesh = m_meshData.lock()) {
		m_shader->use();

		glm::mat4 viewModelMatrix = camera.viewMatrix() * m_modelMatrix;
		m_shader->bindUniformBlock("Camera", camera.ubo());
		m_shader->setUniform("modelMatrix", m_modelMatrix);
		m_shader->setUniform("viewModelMatrix", viewModelMatrix);

		glBindVertexArray(mesh->vao());
		glDrawArrays(GL_TRIANGLES, 0, mesh->pointCount());
		glBindVertexArray(0);
	}
}

void MeshRenderer::reloadShaders()
{
	m_shader->load();
}
