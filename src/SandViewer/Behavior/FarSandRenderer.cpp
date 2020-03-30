#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/modernglad.h>

#include "utils/jsonutils.h"
#include "utils/mathutils.h"
#include "utils/strutils.h"
#include "utils/behaviorutils.h"
#include "utils/ScopedFramebufferOverride.h"
#include "FarSandRenderer.h"
#include "TransformBehavior.h"
#include "ShaderPool.h"
#include "ShaderProgram.h"
#include "GlBuffer.h"
#include "PointCloudDataBehavior.h"
#include "ResourceManager.h"
#include "GlTexture.h"
#include "Framebuffer.h"
#include "PostEffect.h"

const std::vector<std::string> FarSandRenderer::s_shaderVariantDefines = {
	"STAGE_EPSILON_ZBUFFER",
	"NO_DISCARD_IN_EPSILON_ZBUFFER",
	"NO_COLOR_OUTPUT",
	"USING_ShellCullingFragDepth",
	"STAGE_EXTRA_INIT",
	"STAGE_BLIT_TO_MAIN_FBO",
	"USING_ExtraFbo",
};

///////////////////////////////////////////////////////////////////////////////
// Behavior implementation
///////////////////////////////////////////////////////////////////////////////

bool FarSandRenderer::deserialize(const rapidjson::Value & json)
{

	jrOption(json, "shader", m_shaderName, m_shaderName);
	jrOption(json, "colormap", m_colormapTextureName, m_colormapTextureName);

	autoDeserialize(json, m_properties);

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
	return true;
}

void FarSandRenderer::start()
{
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
	if (target == RenderType::ShadowMap) {
		renderToShadowMap(camera, world, target);
		return;
	}

	// Sanity checks
	auto pointData = m_pointData.lock();
	if (!pointData) return;

	glEnable(GL_PROGRAM_POINT_SIZE);

	const auto & props = properties();

	ScopedFramebufferOverride scoppedFramebufferOverride; // to restore fbo binding
	std::shared_ptr<Framebuffer> fbo;
	if (props.extraFbo) {
		fbo = camera.getExtraFramebuffer(Camera::ExtraFramebufferOption::TwoRgba32fDepth);
	}

	// 1. Render epsilon depth buffer
	if (props.useShellCulling) {
		if (props.extraFbo) {
			fbo->deactivateColorAttachments();
			fbo->bind();
			glClear(GL_DEPTH_BUFFER_BIT);
		}

		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		if (props.shellCullingStrategy == ShellCullingDepthRange) {
			glDepthRangef(depthRangeBias(camera), 1.0f);
		}

		ShaderVariantFlagSet flags = ShaderVariantEpsilonZPass;
		if (props.shellCullingStrategy == ShellCullingFragDepth) flags |= ShaderVariantFragDepth;
		if (props.noDiscardInEpsilonZPass) flags |= ShaderVariantNoDiscard;
		if (props.extraInitPass) flags |= ShaderVariantNoColor;
		if (props.extraFbo) flags |= ShaderVariantUsingExtraFbo;
		ShaderProgram & shader = *getShader(flags);
		setCommonUniforms(shader, camera);

		shader.use();
		glBindVertexArray(pointData->vao());
		glDrawArrays(GL_POINTS, 0, pointData->pointCount() / pointData->frameCount());
		glBindVertexArray(0);

		if (props.extraFbo) {
			// Replaces optional extra init pass
			fbo->activateColorAttachments();
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
		}
	}

	// 2. Optional extra init pass
	if (!props.extraFbo && props.useShellCulling && props.extraInitPass) {
		glDepthMask(GL_FALSE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		if (props.shellCullingStrategy == ShellCullingDepthRange) {
			glDepthRangef(0, 1.0f - depthRangeBias(camera));
		}

		ShaderVariantFlagSet flags = ShaderVariantExtraInitPass;
		if (props.shellCullingStrategy == ShellCullingFragDepth) flags |= ShaderVariantFragDepth;
		if (props.noDiscardInEpsilonZPass) flags |= ShaderVariantNoDiscard;
		if (props.extraFbo) flags |= ShaderVariantUsingExtraFbo;
		ShaderProgram& shader = *getShader(flags);
		setCommonUniforms(shader, camera);

		if (props.useShellCulling && props.useEarlyDepthTest) {
			glTextureBarrier();
			setDepthUniform(shader);
		}

		shader.use();
		glBindVertexArray(pointData->vao());
		glDrawArrays(GL_POINTS, 0, pointData->pointCount() / pointData->frameCount());
		glBindVertexArray(0);
	}

	// 3. Render points cumulatively
	{
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

		if (props.shellCullingStrategy == ShellCullingDepthRange) {
			glDepthRangef(0, 1.0f - depthRangeBias(camera));
		}

		ShaderVariantFlagSet flags = 0;
		if (props.shellCullingStrategy == ShellCullingFragDepth) flags |= ShaderVariantFragDepth;
		if (props.noDiscardInEpsilonZPass) flags |= ShaderVariantNoDiscard;
		if (props.extraFbo) flags |= ShaderVariantUsingExtraFbo;
		ShaderProgram & shader = *getShader(flags);
		setCommonUniforms(shader, camera);

		if (props.useShellCulling && (props.shellDepthFalloff || props.useEarlyDepthTest)) {
			glTextureBarrier();
			setDepthUniform(shader);
		}

		shader.use();
		glBindVertexArray(pointData->vao());
		glDrawArrays(GL_POINTS, 0, pointData->pointCount() / pointData->frameCount());
		glBindVertexArray(0);
	}

	// 4. Blit extra fbo to gbuffer
	if (props.useShellCulling && props.extraFbo) {
		scoppedFramebufferOverride.restore();
		glTextureBarrier();
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		ShaderVariantFlagSet flags = ShaderVariantBlitToMainFbo;
		if (props.extraFbo) flags |= ShaderVariantUsingExtraFbo;
		ShaderProgram& shader = *getShader(flags);

		setCommonUniforms(shader, camera);

		// Bind depth buffer for reading
		setDepthUniform(shader);

		// Bind secondary FBO textures
		GLint o = 0;
		for (int i = 0; i < fbo->colorTextureCount(); ++i) {
			glBindTextureUnit(static_cast<GLuint>(o), fbo->colorTexture(i));
			shader.setUniform(MAKE_STR("uFboColor" << i << "Texture"), o);
			++o;
		}
		glBindTextureUnit(static_cast<GLuint>(o), fbo->depthTexture());
		shader.setUniform("uFboDepthTexture", o);
		++o;

		shader.use();
		PostEffect::DrawWithDepthTest();
	}

	// Reset depth range to default
	glDepthRangef(0.0f, 1.0f);
	glDepthMask(GL_TRUE);
}

///////////////////////////////////////////////////////////////////////////////
// private members
///////////////////////////////////////////////////////////////////////////////

void FarSandRenderer::renderToShadowMap(const Camera& camera, const World& world, RenderType target) const
{
	// Sanity checks
	auto pointData = m_pointData.lock();
	if (!pointData) return;

	glEnable(GL_PROGRAM_POINT_SIZE);

	const auto& props = properties();

	// 1. Render epsilon depth buffer
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	ShaderVariantFlagSet flags = ShaderVariantEpsilonZPass;
	if (props.noDiscardInEpsilonZPass) flags |= ShaderVariantNoDiscard;
	flags |= ShaderVariantNoColor;
	ShaderProgram& shader = *getShader(flags);
	setCommonUniforms(shader, camera);

	shader.setUniform("uEpsilon", 0);

	shader.use();
	glBindVertexArray(pointData->vao());
	glDrawArrays(GL_POINTS, 0, pointData->pointCount() / pointData->frameCount());
	glBindVertexArray(0);
}

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

	autoSetUniforms(shader, m_properties);

	shader.setUniform("uEpsilon", m_properties.epsilonFactor * m_properties.radius);
	shader.setUniform("uTime", m_time);
	
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

void FarSandRenderer::setDepthUniform(ShaderProgram & shader) const
{
	GLint depthTexture;
	GLint drawFboId = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
	glGetNamedFramebufferAttachmentParameteriv(drawFboId, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depthTexture);
	glBindTextureUnit(7, static_cast<GLuint>(depthTexture));
	shader.setUniform("uDepthTexture", 7);
}

float FarSandRenderer::depthRangeBias(const Camera & camera) const
{
	if (m_properties.shellCullingStrategy == ShellCullingDepthRange) {
		float eps = m_properties.epsilonFactor * m_properties.radius;
		return eps / (camera.farDistance() - camera.nearDistance() + eps);
	}
	else {
		return 0.0f;
	}
}

std::shared_ptr<ShaderProgram> FarSandRenderer::getShader(ShaderVariantFlagSet flags) const
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
