/**
 * This file is part of GrainViewer
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#include <OpenGL>

#include "utils/jsonutils.h"
#include "utils/mathutils.h"
#include "utils/strutils.h"
#include "utils/behaviorutils.h"
#include "utils/ScopedFramebufferOverride.h"
#include "FarGrainRenderer.h"
#include "TransformBehavior.h"
#include "GrainBehavior.h"
#include "ShaderPool.h"
#include "ShaderProgram.h"
#include "GlBuffer.h"
#include "ResourceManager.h"
#include "GlTexture.h"
#include "Framebuffer.h"
#include "PostEffect.h"
#include "BehaviorRegistry.h"
#include "GlobalTimer.h"

#include <magic_enum.hpp>

const std::vector<std::string> FarGrainRenderer::s_shaderVariantDefines = {
	"SHELL_CULLING",
	"PASS_DEPTH",
	"PASS_EPSILON_DEPTH",
	"PASS_BLIT_TO_MAIN_FBO",
	"NO_DISCARD_IN_PASS_EPSILON_DEPTH",
	"PSEUDO_LEAN",
};

//-----------------------------------------------------------------------------
// Behavior implementation

bool FarGrainRenderer::deserialize(const rapidjson::Value & json)
{

	jrOption(json, "shader", m_shaderName, m_shaderName);
	jrOption(json, "colormap", m_colormapTextureName, m_colormapTextureName);

	autoDeserialize(json, m_properties);

	return true;
}

void FarGrainRenderer::start()
{
	m_transform = getComponent<TransformBehavior>();
	m_grain = getComponent<GrainBehavior>();
	m_pointData = BehaviorRegistry::getPointCloudDataComponent(*this, PointCloudSplitter::RenderModel::Point);

	if (!m_colormapTextureName.empty()) {
		m_colormapTexture = ResourceManager::loadTexture(m_colormapTextureName);
	}
}

void FarGrainRenderer::update(float time, int frame)
{
	m_time = time;
}

void FarGrainRenderer::render(const Camera& camera, const World& world, RenderType target) const
{
	ScopedTimer timer((target == RenderType::ShadowMap ? "FarGrainRenderer_shadowmap" : "FarGrainRenderer"));

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

//-----------------------------------------------------------------------------
// private members

void FarGrainRenderer::draw(const IPointCloudData& pointData) const
{
	glBindVertexArray(pointData.vao());
	if (auto ebo = pointData.ebo()) {
		//glVertexArrayElementBuffer(pointData.vao(), ebo->name());
		//glDrawElements(GL_POINTS, pointData.pointCount(), GL_UNSIGNED_INT, 0);
		// could not find a way to offset in element buffer, so fall back to ssbo for indexed vertex arrays
		pointData.vbo().bindSsbo(0);
		ebo->bindSsbo(1);
		glDrawArrays(GL_POINTS, pointData.pointOffset(), pointData.pointCount());
	} else {
		glDrawArrays(GL_POINTS, pointData.pointOffset(), pointData.pointCount());
	}
	glBindVertexArray(0);
}

void FarGrainRenderer::renderToGBuffer(const IPointCloudData& pointData, const Camera& camera, const World& world) const
{
	ScopedFramebufferOverride scoppedFramebufferOverride; // to automatically restore fbo binding at the end of scope

	glEnable(GL_PROGRAM_POINT_SIZE);

	const auto& props = properties();
	
	std::shared_ptr<Framebuffer> fbo;
	if (props.useShellCulling) {
		if (props.pseudoLean) {
			fbo = camera.getExtraFramebuffer(Camera::ExtraFramebufferOption::LeanLinearGBufferDepth);
		}
		else {
			fbo = camera.getExtraFramebuffer(Camera::ExtraFramebufferOption::LinearGBufferDepth);
		}
	}

	// 0. Clear depth
	if (props.useShellCulling) {
		fbo->deactivateColorAttachments();
		fbo->bind();
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	// 1. Render depth buffer with an offset of epsilon
	if (props.useShellCulling) {
		ShaderVariantFlagSet flags = ShaderPassEpsilonDepth | ShaderOptionShellCulling;
		if (props.noDiscard) flags |= ShaderOptionNoDiscard;
		if (props.pseudoLean) flags |= ShaderOptionPseudoLean;
		ShaderProgram& shader = *getShader(flags);

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
		ShaderVariantFlagSet flags = 0;
		if (props.noDiscard) flags |= ShaderOptionNoDiscard;
		if (props.pseudoLean) flags |= ShaderOptionPseudoLean;
		if (props.useShellCulling) flags |= ShaderOptionShellCulling;
		ShaderProgram& shader = *getShader(flags);

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

		GLint o = setCommonUniforms(shader, camera);

		if (auto sand = m_grain.lock()) {
			for (size_t k = 0; k < sand->atlases().size(); ++k) {
				o = sand->atlases()[k].setUniforms(shader, MAKE_STR("uImpostor[" << k << "]."), o);
			}
		}

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
		ShaderVariantFlagSet flags = ShaderPassBlitToMainFbo | ShaderOptionShellCulling;
		if (props.noDiscard) flags |= ShaderOptionNoDiscard;
		if (props.pseudoLean) flags |= ShaderOptionPseudoLean;
		ShaderProgram& shader = *getShader(flags);

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
			shader.setUniform(MAKE_STR("lgbuffer" << i), o);
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

void FarGrainRenderer::renderToShadowMap(const IPointCloudData& pointData, const Camera& camera, const World& world) const
{
	const Properties& props = properties();
	ShaderVariantFlagSet flags = ShaderPassDepth;
	if (props.noDiscard) flags |= ShaderOptionNoDiscard;
	if (props.pseudoLean) flags |= ShaderOptionPseudoLean;
	if (props.useShellCulling) flags |= ShaderOptionShellCulling;
	ShaderProgram& shader = *getShader(flags);

	glEnable(GL_PROGRAM_POINT_SIZE);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	setCommonUniforms(shader, camera);

	shader.use();
	draw(pointData);
}

glm::mat4 FarGrainRenderer::modelMatrix() const {
	if (auto transform = m_transform.lock()) {
		return transform->modelMatrix();
	} else {
		return glm::mat4(1.0f);
	}
}

GLint FarGrainRenderer::setCommonUniforms(const ShaderProgram & shader, const Camera & camera, GLint nextTextureUnit) const {
	GLint o = nextTextureUnit;

	glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
	shader.bindUniformBlock("Camera", camera.ubo());
	shader.setUniform("modelMatrix", modelMatrix());
	shader.setUniform("viewModelMatrix", viewModelMatrix);

	autoSetUniforms(shader, m_properties);
	if (auto sand = m_grain.lock()) {
		autoSetUniforms(shader, sand->properties());
		shader.setUniform("uEpsilon", m_properties.epsilonFactor * sand->properties().grainRadius);
	} else {
		shader.setUniform("uEpsilon", m_properties.epsilonFactor * m_properties.radius);
	}
	
	shader.setUniform("uTime", m_time);
	
	if (m_colormapTexture) {
		m_colormapTexture->bind(o);
		shader.setUniform("uColormapTexture", o);
		++o;
	}

	return o;
}

void FarGrainRenderer::bindDepthTexture(ShaderProgram & shader, GLuint textureUnit) const
{
	GLint depthTexture;
	GLint drawFboId = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
	glGetNamedFramebufferAttachmentParameteriv(drawFboId, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depthTexture);
	glBindTextureUnit(textureUnit, static_cast<GLuint>(depthTexture));
	shader.setUniform("uDepthTexture", static_cast<GLint>(textureUnit));
}

std::shared_ptr<ShaderProgram> FarGrainRenderer::getShader(ShaderVariantFlagSet flags) const
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
