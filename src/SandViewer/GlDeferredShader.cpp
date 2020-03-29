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

GlDeferredShader::GlDeferredShader()
	: m_shader("deferred-shader")
{
	glCreateVertexArrays(1, &m_vao);
}

void GlDeferredShader::lazyInitFramebuffer() const
{
	m_framebuffer = std::make_unique<Framebuffer>(m_width, m_height, m_attachments);
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
			addShaderDefine(def);
		}
	}

	std::string colormapFilename;
	if (jrOption(json, "colormap", colormapFilename)) {
		m_colormap = ResourceManager::loadTexture(colormapFilename);
		m_colormap->setWrapMode(GL_CLAMP_TO_EDGE);
	}

	autoDeserialize(json, m_properties);

	jrOption(json, "attachments", m_attachments, m_attachments);
	GLenum i = 0;
	for (auto & att : m_attachments) {
		att.attachement = GL_COLOR_ATTACHMENT0 + i;
		++i;
	}

	return true;
}

void GlDeferredShader::reloadShaders()
{
	m_shader.load();
}

void GlDeferredShader::update(float time)
{
	m_shader.use();
	m_shader.setUniform("uTime", static_cast<GLfloat>(time));
}

void GlDeferredShader::render(const Camera & camera, const World & world, RenderType target) const
{
	if (!m_framebuffer) lazyInitFramebuffer();

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	m_shader.use();
	m_shader.bindUniformBlock("Camera", camera.ubo());

	GLint o = 0;
	for (int i = 0; i < m_framebuffer->colorTextureCount(); ++i) {
		m_shader.setUniform(MAKE_STR("gbuffer" << i), o);
		glBindTextureUnit(static_cast<GLuint>(o), m_framebuffer->colorTexture(i));
		++o;
	}

	m_shader.setUniform("in_depth", o);
	glBindTextureUnit(static_cast<GLuint>(o), m_framebuffer->depthTexture());
	++o;

	// TODO: Use UBO, move to World
	auto lights = world.lights();
	for (size_t k = 0; k < lights.size(); ++k) {
		std::string prefix = MAKE_STR("light[" << k << "].");
		m_shader.setUniform(prefix + "position_ws", lights[k]->position());
		m_shader.setUniform(prefix + "color", lights[k]->color());
		m_shader.setUniform(prefix + "matrix", lights[k]->shadowMap().camera().projectionMatrix() * lights[k]->shadowMap().camera().viewMatrix());
		m_shader.setUniform(prefix + "isRich", lights[k]->isRich() ? 1 : 0);
		m_shader.setUniform(prefix + "hasShadowMap", lights[k]->hasShadowMap() ? 1 : 0);
		m_shader.setUniform(prefix + "shadowMap", o);
		glBindTextureUnit(static_cast<GLuint>(o), lights[k]->shadowMap().depthTexture());
		++o;
		if (lights[k]->isRich()) {
			ostringstream oss6;
			oss6 << "light[" << k << "].richShadowMap";
			m_shader.setUniform(oss6.str(), o);
			glBindTextureUnit(static_cast<GLuint>(o), lights[k]->shadowMap().colorTexture(0));
			++o;
		}
	}

	m_shader.setUniform("uIsShadowMapEnabled", world.isShadowMapEnabled());

	autoSetUniforms(m_shader, m_properties);

	m_shader.setUniform("uHasColormap", static_cast<bool>(m_colormap));
	if (m_colormap) {
		m_shader.setUniform("uColormap", static_cast<GLint>(o));
		m_colormap->bind(static_cast<GLuint>(o));
		++o;
	}

	glBindVertexArray(m_vao);
	glDrawArrays(GL_POINTS, 0, 1);
	glBindVertexArray(0);
}

void GlDeferredShader::setResolution(int width, int height)
{
	m_width = width;
	m_height = height;
	if (!m_framebuffer) {
		lazyInitFramebuffer();
	} else {
		m_framebuffer->setResolution(width, height);
	}
}
