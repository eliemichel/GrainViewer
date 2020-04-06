#include "Logger.h"

#include "ResourceManager.h"
#include "ShaderProgram.h"
#include "MeshDataBehavior.h"
#include "MeshRenderer.h"
#include "ShaderPool.h"
#include "TransformBehavior.h"
#include "GlTexture.h"
#include "utils/jsonutils.h"
#include "utils/strutils.h"

///////////////////////////////////////////////////////////////////////////////
// Behavior implementation
///////////////////////////////////////////////////////////////////////////////

bool MeshRenderer::deserialize(const rapidjson::Value& json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);
	jrArray(json, "materials", m_materials);
	return true;
}

void MeshRenderer::start()
{
	m_shader = ShaderPool::GetShader(m_shaderName);
	m_meshData = getComponent<MeshDataBehavior>();
	m_transform = getComponent<TransformBehavior>();
}

void MeshRenderer::render(const Camera& camera, const World& world, RenderType target) const
{
	if (!m_shader->isValid()) return;

	if (auto mesh = m_meshData.lock()) {
		m_shader->use();

		glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
		m_shader->bindUniformBlock("Camera", camera.ubo());
		m_shader->setUniform("modelMatrix", modelMatrix());
		m_shader->setUniform("viewModelMatrix", viewModelMatrix);

		GLuint o = 0;
		int n = static_cast<int>(std::max(mesh->materials().size(), m_materials.size()));
		for (int i = 0 ; i < n ; ++i) {
			const StandardMaterial& mat = i < m_materials.size() ? m_materials[i] : mesh->materials()[i];
			o = mat.setUniforms(*m_shader, MAKE_STR("uMaterial[" << i << "]."), o);
		}

		glBindVertexArray(mesh->vao());
		glDrawArrays(GL_TRIANGLES, 0, mesh->pointCount());
		glBindVertexArray(0);
	}
}

///////////////////////////////////////////////////////////////////////////////
// private members
///////////////////////////////////////////////////////////////////////////////

glm::mat4 MeshRenderer::modelMatrix() const {
	if (auto transform = m_transform.lock()) {
		return transform->modelMatrix();
	} else {
		return glm::mat4(1.0f);
	}
}