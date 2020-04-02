#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/modernglad.h>

#include <magic_enum.hpp>

#include "utils/jsonutils.h"
#include "utils/mathutils.h"
#include "utils/strutils.h"
#include "utils/behaviorutils.h"
#include "utils/ScopedFramebufferOverride.h"
#include "UberSandRenderer.h"
#include "TransformBehavior.h"
#include "ShaderPool.h"
#include "ShaderProgram.h"
#include "GlBuffer.h"
#include "ResourceManager.h"
#include "GlTexture.h"
#include "Framebuffer.h"
#include "PostEffect.h"

// TODO find a way to use reflection or behavior registry to avoid enumerating all possible parent of IPointCloudData
#include "PointCloudDataBehavior.h"
#include "Sand6Data.h"

const std::vector<std::string> UberSandRenderer::s_shaderVariantDefines = {
	"SHELL_CULLING",
	"PASS_DEPTH",
	"PASS_EPSILON_DEPTH",
	"PASS_BLIT_TO_MAIN_FBO",
};

///////////////////////////////////////////////////////////////////////////////
// Behavior implementation
///////////////////////////////////////////////////////////////////////////////

bool UberSandRenderer::deserialize(const rapidjson::Value & json)
{

	jrOption(json, "shader", m_shaderName, m_shaderName);
	jrOption(json, "colormap", m_colormapTextureName, m_colormapTextureName);

	autoDeserialize(json, m_properties);

	return true;
}

void UberSandRenderer::start()
{
	m_transform = getComponent<TransformBehavior>();

	m_pointData = getComponent<PointCloudDataBehavior>();
	if (m_pointData.expired()) m_pointData = getComponent<Sand6Data>();
	if (m_pointData.expired()) {
		WARN_LOG << "UberSandRenderer could not find point data (ensure that there is a PointCloudDataBehavior or Sand6Data attached to the same object)";
	}

	if (!m_colormapTextureName.empty()) {
		m_colormapTexture = ResourceManager::loadTexture(m_colormapTextureName);
	}
}

void UberSandRenderer::update(float time, int frame)
{
	m_time = time;
}

void UberSandRenderer::render(const Camera& camera, const World& world, RenderType target) const
{
	// Sanity checks
	auto pointData = m_pointData.lock();
	if (!pointData) return;

	switch (target) {
	case RenderType::ShadowMap:
		renderToShadowMap(*pointData, camera, world);
		break;
	case RenderType::Default:
		renderToGBuffer(*pointData, camera, world);
		break;
	default:
		ERR_LOG << "Unsupported render target: " << magic_enum::enum_name(target);
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////
// private members
///////////////////////////////////////////////////////////////////////////////

void UberSandRenderer::draw(const IPointCloudData& pointData) const
{
	glBindVertexArray(pointData.vao());
	glDrawArrays(GL_POINTS, pointData.pointOffset(), pointData.pointCount() / pointData.frameCount());
	glBindVertexArray(0);
}

void UberSandRenderer::renderToGBuffer(const IPointCloudData& pointData, const Camera& camera, const World& world) const
{
	ScopedFramebufferOverride scoppedFramebufferOverride; // to automatically restore fbo binding at the end of scope

	glEnable(GL_PROGRAM_POINT_SIZE);

	const auto& props = properties();
	
	std::shared_ptr<Framebuffer> fbo;
	if (props.useShellCulling) {
		fbo = camera.getExtraFramebuffer(Camera::ExtraFramebufferOption::TwoRgba32fDepth);
	}

	// 0. Clear depth
	if (props.useShellCulling) {
		fbo->deactivateColorAttachments();
		fbo->bind();
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	// 1. Render depth buffer with an offset of epsilon
	if (props.useShellCulling) {
		ShaderProgram& shader = *getShader(ShaderPassEpsilonDepth | ShaderOptionShellCulling);

		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		setCommonUniforms(shader, camera);

		shader.use();
		draw(pointData);
	}

	// 2. Clear color buffers
	if (props.useShellCulling) {
		fbo->activateColorAttachments();
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	// 3. Render points cumulatively
	{
		ShaderProgram& shader = *getShader(props.useShellCulling ? ShaderOptionShellCulling : 0);

		if (props.useShellCulling) {
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

		setCommonUniforms(shader, camera);

		if (props.useShellCulling && (props.shellDepthFalloff || props.useEarlyDepthTest)) {
			// If we need to read from the current depth buffer
			glTextureBarrier();
			bindDepthTexture(shader);
		}

		shader.use();
		draw(pointData);
	}

	// 4. Blit extra fbo to gbuffer
	if (props.useShellCulling) {
		ShaderProgram& shader = *getShader(ShaderPassBlitToMainFbo | ShaderOptionShellCulling);

		scoppedFramebufferOverride.restore();
		
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		setCommonUniforms(shader, camera);

		// Bind secondary FBO textures
		glTextureBarrier();
		GLint o = 0;
		for (int i = 0; i < fbo->colorTextureCount(); ++i) {
			glBindTextureUnit(static_cast<GLuint>(o), fbo->colorTexture(i));
			shader.setUniform(MAKE_STR("uFboColor" << i << "Texture"), o);
			++o;
		}
		glBindTextureUnit(static_cast<GLuint>(o), fbo->depthTexture());
		shader.setUniform("uFboDepthTexture", o);
		++o;

		// Bind depth buffer for reading
		bindDepthTexture(shader, o++);

		shader.use();
		PostEffect::DrawWithDepthTest();
	}
}

void UberSandRenderer::renderToShadowMap(const IPointCloudData& pointData, const Camera& camera, const World& world) const
{
	ShaderProgram& shader = *getShader(ShaderPassDepth | (properties().useShellCulling ? ShaderOptionShellCulling : 0));

	glEnable(GL_PROGRAM_POINT_SIZE);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	setCommonUniforms(shader, camera);

	shader.use();
	draw(pointData);
}

glm::mat4 UberSandRenderer::modelMatrix() const {
	if (auto transform = m_transform.lock()) {
		return transform->modelMatrix();
	} else {
		return glm::mat4(1.0f);
	}
}

void UberSandRenderer::setCommonUniforms(ShaderProgram & shader, const Camera & camera) const {
	glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
	shader.bindUniformBlock("Camera", camera.ubo());
	shader.setUniform("modelMatrix", modelMatrix());
	shader.setUniform("viewModelMatrix", viewModelMatrix);

	autoSetUniforms(shader, m_properties);

	shader.setUniform("uEpsilon", m_properties.epsilonFactor * m_properties.radius);
	shader.setUniform("uTime", m_time);
	
	GLint o = 0;
	if (m_colormapTexture) {
		m_colormapTexture->bind(o);
		shader.setUniform("uColormapTexture", o);
		++o;
	}
}

void UberSandRenderer::bindDepthTexture(ShaderProgram & shader, GLuint textureUnit) const
{
	GLint depthTexture;
	GLint drawFboId = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
	glGetNamedFramebufferAttachmentParameteriv(drawFboId, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depthTexture);
	glBindTextureUnit(textureUnit, static_cast<GLuint>(depthTexture));
	shader.setUniform("uDepthTexture", static_cast<GLint>(textureUnit));
}

std::shared_ptr<ShaderProgram> UberSandRenderer::getShader(ShaderVariantFlagSet flags) const
{
	if (m_shaders.empty()) {
		m_shaders.resize(_ShaderVariantFlagsCount);
	}

	if (!m_shaders[flags]) {
		// Lazy loading of shader variants
		int nFlags = ilog2(_ShaderVariantFlagsCount);
		std::string variantName = m_shaderName + "_ShaderVariantFlags_" + bitname(flags, nFlags);
		std::vector<std::string> defines;
		for (int f = 0; f < nFlags; ++f) {
			if ((flags & (1 << f)) != 0) {
				defines.push_back(s_shaderVariantDefines[f]);
			}
		}
		DEBUG_LOG << "loading variant " << variantName;
		ShaderPool::AddShaderVariant(variantName, m_shaderName, defines);
		m_shaders[flags] = ShaderPool::GetShader(variantName);
	}
	return m_shaders[flags];
}
