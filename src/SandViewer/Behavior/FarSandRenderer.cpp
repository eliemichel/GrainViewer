#include <glad/glad.h>

#include "utils/jsonutils.h"
#include "FarSandRenderer.h"
#include "TransformBehavior.h"
#include "ShaderPool.h"
#include "ShaderProgram.h"
#include "GlBuffer.h"
#include "PointCloudDataBehavior.h"
#include "ResourceManager.h"
#include "GlTexture.h"

///////////////////////////////////////////////////////////////////////////////
// Behavior implementation
///////////////////////////////////////////////////////////////////////////////

bool FarSandRenderer::deserialize(const rapidjson::Value & json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);
	jrOption(json, "colormap", m_colormapTextureName, m_colormapTextureName);
	jrOption(json, "radius", m_properties.radius, m_properties.radius);
	return true;
}

void FarSandRenderer::start()
{
	m_shader = ShaderPool::GetShader(m_shaderName);
	m_transform = getComponent<TransformBehavior>();
	m_pointData = getComponent<PointCloudDataBehavior>();

	if (!m_colormapTextureName.empty()) {
		m_colormapTexture = ResourceManager::loadTexture(m_colormapTextureName);
	}
}

void FarSandRenderer::render(const Camera & camera, const World & world, RenderType target) const
{
	if (!m_shader->isValid()) return;
	if (auto pointData = m_pointData.lock()) {
		glEnable(GL_PROGRAM_POINT_SIZE);
		m_shader->use();

		glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
		m_shader->bindUniformBlock("Camera", camera.ubo());
		m_shader->setUniform("modelMatrix", modelMatrix());
		m_shader->setUniform("viewModelMatrix", viewModelMatrix);

		m_shader->setUniform("uRadius", m_properties.radius);
		
		size_t o = 0;
		if (m_colormapTexture) {
			glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o));
			m_colormapTexture->bind();
			m_shader->setUniform("uColormapTexture", static_cast<GLint>(o));
			++o;
		}

		glBindVertexArray(pointData->vao());
		glDrawArrays(GL_POINTS, 0, pointData->pointCount() / pointData->frameCount());
		glBindVertexArray(0);
	}
}

///////////////////////////////////////////////////////////////////////////////
// private members
///////////////////////////////////////////////////////////////////////////////

glm::mat4 FarSandRenderer::modelMatrix() const {
	if (auto transform = m_transform.lock()) {
		return transform->modelMatrix();
	} else {
		return glm::mat4(1.0f);
	}
}
