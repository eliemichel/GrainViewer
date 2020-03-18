#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/modernglad.h>

#include "utils/jsonutils.h"
#include "FarSandRenderer.h"
#include "TransformBehavior.h"
#include "ShaderPool.h"
#include "ShaderProgram.h"
#include "GlBuffer.h"
#include "PointCloudDataBehavior.h"
#include "ResourceManager.h"
#include "GlTexture.h"
#include "Framebuffer.h"

///////////////////////////////////////////////////////////////////////////////
// Behavior implementation
///////////////////////////////////////////////////////////////////////////////

bool FarSandRenderer::deserialize(const rapidjson::Value & json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);
	jrOption(json, "epsilonZBufferShader", m_epsilonZBufferShaderName, m_epsilonZBufferShaderName);
	jrOption(json, "colormap", m_colormapTextureName, m_colormapTextureName);
	jrOption(json, "radius", m_properties.radius, m_properties.radius);
	jrOption(json, "epsilonFactor", m_properties.epsilonFactor, m_properties.epsilonFactor);
	return true;
}

void FarSandRenderer::start()
{
	m_shader = ShaderPool::GetShader(m_shaderName);
	m_epsilonZBufferShader = ShaderPool::GetShader(m_epsilonZBufferShaderName);
	m_transform = getComponent<TransformBehavior>();
	m_pointData = getComponent<PointCloudDataBehavior>();

	if (!m_colormapTextureName.empty()) {
		m_colormapTexture = ResourceManager::loadTexture(m_colormapTextureName);
	}
}

void FarSandRenderer::render(const Camera & camera, const World & world, RenderType target) const
{
	// Sanity checks
	auto pointData = m_pointData.lock();
	if (!pointData) return;
	if (!m_shader->isValid()) return;
	if (!m_epsilonZBufferShader->isValid()) return;

	glEnable(GL_PROGRAM_POINT_SIZE);

	//GLint drawFboId = 0, readFboId = 0;
	//glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
	//glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFboId);
	//auto depthFbo = camera.getExtraFramebuffer(true /* depthOnly */);

	// 1. Render epsilon depth buffer
	if (properties().useShellCulling) {
		//depthFbo->bind();
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		ShaderProgram & shader = *m_epsilonZBufferShader;
		setCommonUniforms(shader, camera);

		shader.use();
		glBindVertexArray(pointData->vao());
		glDrawArrays(GL_POINTS, 0, pointData->pointCount() / pointData->frameCount());
		glBindVertexArray(0);

		glTextureBarrier();
	}

	// 2. Render points cumulatively
	{
		//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFboId);
		//glBindFramebuffer(GL_READ_FRAMEBUFFER, readFboId);
		if (properties().useShellCulling) {
			glDepthMask(GL_FALSE);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
		}
		else {
			glDepthMask(GL_TRUE);
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
		}

		ShaderProgram & shader = *m_shader;
		setCommonUniforms(shader, camera);

		//glBindTextureUnit(0, depthFbo->depthTexture());
		//shader.setUniform("uDepthTexture", 0);

		shader.use();
		glBindVertexArray(pointData->vao());
		glDrawArrays(GL_POINTS, 0, pointData->pointCount() / pointData->frameCount());
		glBindVertexArray(0);
	}

	glDepthMask(GL_TRUE);
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

void FarSandRenderer::setCommonUniforms(ShaderProgram & shader, const Camera & camera) const {
	glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
	shader.bindUniformBlock("Camera", camera.ubo());
	shader.setUniform("modelMatrix", modelMatrix());
	shader.setUniform("viewModelMatrix", viewModelMatrix);

	shader.setUniform("uRadius", m_properties.radius);
	shader.setUniform("uEpsilon", m_properties.epsilonFactor * m_properties.radius);
	shader.setUniform("uDebugShape", m_properties.debugShape);
	shader.setUniform("uWeightMode", m_properties.weightMode);

	GLint o = 0;
	if (m_colormapTexture) {
		m_colormapTexture->bind(o);
		shader.setUniform("uColormapTexture", o);
		++o;
	}
}
