// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#include <vector>
#include <sstream>

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

using namespace std;

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
