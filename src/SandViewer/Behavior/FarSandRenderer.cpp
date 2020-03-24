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

#define jrProperty(prop) jrOption(json, #prop, m_properties.prop, m_properties.prop)
	jrProperty(radius);
	jrProperty(epsilonFactor);
	jrProperty(useShellCulling);
	jrProperty(shellDepthFalloff);
	jrProperty(disableBlend);
	jrProperty(useEarlyDepthTest);
#undef jrProperty

	int debugShape = m_properties.debugShape;
	jrOption(json, "debugShape", debugShape, debugShape);
	m_properties.debugShape = static_cast<DebugShape>(debugShape);

	int weightMode = m_properties.weightMode;
	jrOption(json, "debugShape", weightMode, weightMode);
	m_properties.weightMode = static_cast<WeightMode>(weightMode);

	int shellCullingStrategy = m_properties.shellCullingStrategy;
	jrOption(json, "debugShape", shellCullingStrategy, shellCullingStrategy);
	m_properties.shellCullingStrategy = static_cast<ShellCullingStrategy>(shellCullingStrategy);

	if (json.HasMember("bbox")) {
		if (json["bbox"].IsObject()) {
			m_properties.useBbox = true;
			auto & bbox = json["bbox"];
			if (bbox.HasMember("xmin") && bbox["xmin"].IsNumber()) m_properties.bboxMin.x = bbox["xmin"].GetFloat();
			if (bbox.HasMember("ymin") && bbox["ymin"].IsNumber()) m_properties.bboxMin.y = bbox["ymin"].GetFloat();
			if (bbox.HasMember("zmin") && bbox["zmin"].IsNumber()) m_properties.bboxMin.z = bbox["zmin"].GetFloat();
			if (bbox.HasMember("xmax") && bbox["xmax"].IsNumber()) m_properties.bboxMax.x = bbox["xmax"].GetFloat();
			if (bbox.HasMember("ymax") && bbox["ymax"].IsNumber()) m_properties.bboxMax.y = bbox["ymax"].GetFloat();
			if (bbox.HasMember("zmax") && bbox["zmax"].IsNumber()) m_properties.bboxMax.z = bbox["zmax"].GetFloat();
		}
		else {
			ERR_LOG << "Field 'filename' of PointCloudDataBehavior must be a string";
			return false;
		}
	}

	m_shaderName_Variant = m_shaderName + "_USING_ShellCullingFragDepth";
	ShaderPool::AddShaderVariant(m_shaderName_Variant, m_shaderName, { "USING_ShellCullingFragDepth" });
	m_epsilonZBufferShaderName_Variant = m_epsilonZBufferShaderName + "_USING_ShellCullingFragDepth";
	ShaderPool::AddShaderVariant(m_epsilonZBufferShaderName_Variant, m_epsilonZBufferShaderName, { "USING_ShellCullingFragDepth" });

	return true;
}

void FarSandRenderer::start()
{
	m_shader = ShaderPool::GetShader(m_shaderName);
	m_epsilonZBufferShader = ShaderPool::GetShader(m_epsilonZBufferShaderName);
	m_shader_Variant = ShaderPool::GetShader(m_shaderName_Variant);
	m_epsilonZBufferShader_Variant = ShaderPool::GetShader(m_epsilonZBufferShaderName_Variant);
	m_transform = getComponent<TransformBehavior>();
	m_pointData = getComponent<PointCloudDataBehavior>();

	if (!m_colormapTextureName.empty()) {
		m_colormapTexture = ResourceManager::loadTexture(m_colormapTextureName);
	}
}

void FarSandRenderer::update(float time, int frame)
{
	m_time = time;
}

void FarSandRenderer::render(const Camera & camera, const World & world, RenderType target) const
{
	// Sanity checks
	auto pointData = m_pointData.lock();
	if (!pointData) return;
	if (!m_shader->isValid()) return;
	if (!m_epsilonZBufferShader->isValid()) return;

	glEnable(GL_PROGRAM_POINT_SIZE);

	GLint drawFboId = 0, readFboId = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFboId);
	//auto depthFbo = camera.getExtraFramebuffer(true /* depthOnly */);

	// 1. Render epsilon depth buffer
	if (properties().useShellCulling) {
		//depthFbo->bind();
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		if (m_properties.shellCullingStrategy == ShellCullingDepthRange) {
			glDepthRangef(depthRangeBias(camera), 1.0f);
		}

		ShaderProgram & shader = m_properties.shellCullingStrategy == ShellCullingFragDepth ? *m_epsilonZBufferShader_Variant : *m_epsilonZBufferShader;
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
			if (properties().disableBlend) {
				glDisable(GL_BLEND);
			} else {
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
			}
		}
		else {
			glDepthMask(GL_TRUE);
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
		}

		if (m_properties.shellCullingStrategy == ShellCullingDepthRange) {
			glDepthRangef(0, 1.0f - depthRangeBias(camera));
		}

		ShaderProgram & shader = m_properties.shellCullingStrategy == ShellCullingFragDepth ? *m_shader_Variant : *m_shader;
		setCommonUniforms(shader, camera);

		if (properties().useShellCulling && properties().shellDepthFalloff) {
			GLint depthTexture;
			glGetNamedFramebufferAttachmentParameteriv(drawFboId, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depthTexture);
			glBindTextureUnit(7, static_cast<GLuint>(depthTexture));
			shader.setUniform("uDepthTexture", 7);
		}

		shader.use();
		glBindVertexArray(pointData->vao());
		glDrawArrays(GL_POINTS, 0, pointData->pointCount() / pointData->frameCount());
		glBindVertexArray(0);
	}

	// Reset depth range to default
	glDepthRangef(0.0f, 1.0f);
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

	shader.setUniform("uTime", m_time);
	shader.setUniform("uRadius", m_properties.radius);
	shader.setUniform("uEpsilon", m_properties.epsilonFactor * m_properties.radius);
	shader.setUniform("uDebugShape", m_properties.debugShape);
	shader.setUniform("uWeightMode", m_properties.weightMode);
	shader.setUniform("uShellDepthFalloff", m_properties.shellDepthFalloff);
	shader.setUniform("uUseShellCulling", m_properties.useShellCulling);
	shader.setUniform("uShellCullingStrategy", m_properties.shellCullingStrategy);
	shader.setUniform("uUseEarlyDepthTest", m_properties.useEarlyDepthTest);

	shader.setUniform("uUseBbox", m_properties.useBbox);
	shader.setUniform("uBboxMin", m_properties.bboxMin);
	shader.setUniform("uBboxMax", m_properties.bboxMax);

	if (m_properties.shellCullingStrategy == ShellCullingDepthRange) {
		shader.setUniform("uDepthRangeMin", 0.0f);
		shader.setUniform("uDepthRangeMax", 1.0f - depthRangeBias(camera));
	}

	GLint o = 0;
	if (m_colormapTexture) {
		m_colormapTexture->bind(o);
		shader.setUniform("uColormapTexture", o);
		++o;
	}
}

float FarSandRenderer::depthRangeBias(const Camera & camera) const
{
	if (m_properties.shellCullingStrategy == ShellCullingDepthRange) {
		float eps = m_properties.epsilonFactor * m_properties.radius;
		return eps / (camera.farDistance() - camera.nearDistance() + eps) * m_properties.metaBias;
	}
	else {
		return 0.0f;
	}
}
