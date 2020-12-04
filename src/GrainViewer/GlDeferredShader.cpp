/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
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

#include "Logger.h"
#include "GlDeferredShader.h"
#include "Light.h"
#include "ShadowMap.h"
#include "ResourceManager.h"
#include "GlTexture.h"
#include "utils/strutils.h"
#include "utils/jsonutils.h"
#include "utils/behaviorutils.h"
#include "Framebuffer.h"

#include <vector>
#include <sstream>

float GlDeferredShader::Properties::ShadowMapBias() const
{
	return shadowMapBiasBase * static_cast<float>(pow(10, shadowMapBiasExponent));
}

GlDeferredShader::GlDeferredShader()
	: m_shader("deferred-shader")
	, m_debugShader("deferred-shader")
{
	glCreateVertexArrays(1, &m_vao);
}

GlDeferredShader::~GlDeferredShader()
{
	glDeleteVertexArrays(1, &m_vao);
}

bool GlDeferredShader::deserialize(const rapidjson::Value & json)
{
	if (!json.IsObject()) return false;

	std::vector<std::string> defines;
	if (jrOption(json, "defines", defines)) {
		for (const auto& def : defines) {
			m_shader.define(def);
			m_debugShader.define(def);
		}
	}
	m_debugShader.define("DEBUG_VECTORS");

	std::string colormapFilename;
	if (jrOption(json, "colormap", colormapFilename)) {
		m_colormap = ResourceManager::loadTexture(colormapFilename);
		m_colormap->setWrapMode(GL_CLAMP_TO_EDGE);
	}

	autoDeserialize(json, m_properties);

	return true;
}

void GlDeferredShader::bindFramebuffer(const Camera& camera) const
{
	auto fbo = camera.getExtraFramebuffer(Camera::ExtraFramebufferOption::GBufferDepth);
	fbo->bind();
}

void GlDeferredShader::reloadShaders()
{
	m_shader.load();
	m_debugShader.load();
}

void GlDeferredShader::update(float time)
{
	m_shader.setUniform("uTime", static_cast<GLfloat>(time));
	m_debugShader.setUniform("uTime", static_cast<GLfloat>(time));
}

void GlDeferredShader::render(const Camera & camera, const World & world, RenderType target) const
{
	auto fbo = camera.getExtraFramebuffer(Camera::ExtraFramebufferOption::GBufferDepth);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	const ShaderProgram& shader = properties().debugVectors ? m_debugShader : m_shader;

	shader.bindUniformBlock("Camera", camera.ubo());

	shader.setUniform("uBlitOffset", m_blitOffset);

	GLint o = 0;
	for (int i = 0; i < fbo->colorTextureCount(); ++i) {
		shader.setUniform(MAKE_STR("gbuffer" << i), o);
		glBindTextureUnit(static_cast<GLuint>(o), fbo->colorTexture(i));
		++o;
	}

	shader.setUniform("in_depth", o);
	glBindTextureUnit(static_cast<GLuint>(o), fbo->depthTexture());
	++o;

	// TODO: Use UBO, move to World
	auto lights = world.lights();
	for (size_t k = 0; k < lights.size(); ++k) {
		std::string prefix = MAKE_STR("light[" << k << "].");
		shader.setUniform(prefix + "position_ws", lights[k]->position());
		shader.setUniform(prefix + "color", lights[k]->color());
		shader.setUniform(prefix + "matrix", lights[k]->shadowMap().camera().projectionMatrix() * lights[k]->shadowMap().camera().viewMatrix());
		shader.setUniform(prefix + "isRich", lights[k]->isRich() ? 1 : 0);
		shader.setUniform(prefix + "hasShadowMap", lights[k]->hasShadowMap() ? 1 : 0);
		shader.setUniform(prefix + "shadowMap", o);
		glBindTextureUnit(static_cast<GLuint>(o), lights[k]->shadowMap().depthTexture());
		++o;
		if (lights[k]->isRich()) {
			shader.setUniform(prefix + "richShadowMap", o);
			glBindTextureUnit(static_cast<GLuint>(o), lights[k]->shadowMap().colorTexture(0));
			++o;
		}
	}

	shader.setUniform("uIsShadowMapEnabled", world.isShadowMapEnabled());

	autoSetUniforms(shader, m_properties);
	shader.setUniform("uShadowMapBias", m_properties.ShadowMapBias());

	shader.setUniform("uHasColormap", static_cast<bool>(m_colormap));
	if (m_colormap) {
		shader.setUniform("uColormap", static_cast<GLint>(o));
		m_colormap->bind(static_cast<GLuint>(o));
		++o;
	}

	shader.use();
	glBindVertexArray(m_vao);
	glDrawArrays(GL_POINTS, 0, 1);
	glBindVertexArray(0);
}
