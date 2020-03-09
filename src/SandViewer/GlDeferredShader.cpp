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

using namespace std;

GlDeferredShader::GlDeferredShader()
	: m_shader("deferred-shader")
	, m_framebuffer(MAX_DISPLAY_WIDTH, MAX_DISPLAY_HEIGHT, {
		{ GL_RGBA32F,  GL_COLOR_ATTACHMENT0 },
		{ GL_RGBA32UI, GL_COLOR_ATTACHMENT1 },
		{ GL_RGBA32UI, GL_COLOR_ATTACHMENT2 },
	})
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

	if (json.HasMember("defines")) {
		auto& defines = json["defines"];
		if (!defines.IsArray()) {
			ERR_LOG << "'defines' must be an array";
		}
		else {
			for (rapidjson::SizeType i = 0; i < defines.Size(); i++) {
				addShaderDefine(defines[i].GetString());
			}
		}
	}

	if (json.HasMember("colormap")) {
		auto& colormap = json["colormap"];
		if (!colormap.IsString()) {
			ERR_LOG << "'colormap' must be a file path";
		} else {
			m_colormap = ResourceManager::loadTexture(colormap.GetString());
			m_colormap->setWrapMode(GL_CLAMP_TO_EDGE);
		}
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
	m_shader.setUniform("time", static_cast<GLfloat>(time));
}

void GlDeferredShader::render(const Camera & camera, const World & world, RenderType target) const
{
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_shader.use();
	m_shader.setUniform("viewMatrix", camera.viewMatrix());
	m_shader.setUniform("inverseViewMatrix", inverse(camera.viewMatrix()));

	size_t o = 0;
	glBindTextureUnit(0, m_framebuffer.colorTexture(0));
	glBindTextureUnit(1, m_framebuffer.colorTexture(1));
	glBindTextureUnit(2, m_framebuffer.colorTexture(2));
	glBindTextureUnit(3, m_framebuffer.depthTexture());
	o += 4;

	// TODO: Use UBO, move to World
	auto lights = world.lights();
	for (size_t k = 0; k < lights.size(); ++k) {
		ostringstream oss1;
		oss1 << "light[" << k << "].position_ws";
		m_shader.setUniform(oss1.str(), lights[k]->position());
		ostringstream oss2;
		oss2 << "light[" << k << "].color";
		m_shader.setUniform(oss2.str(), lights[k]->color());
		ostringstream oss3;
		oss3 << "light[" << k << "].matrix";
		m_shader.setUniform(oss3.str(), lights[k]->shadowMap().camera().projectionMatrix() * lights[k]->shadowMap().camera().viewMatrix());
		ostringstream oss4;
		oss4 << "light[" << k << "].isRich";
		m_shader.setUniform(oss4.str(), lights[k]->isRich() ? 1 : 0);
		ostringstream oss45;
		oss45 << "light[" << k << "].hasShadowMap";
		m_shader.setUniform(oss45.str(), lights[k]->hasShadowMap() ? 1 : 0);
		ostringstream oss5;
		oss5 << "light[" << k << "].shadowMap";
		m_shader.setUniform(oss5.str(), static_cast<GLint>(o));
		glBindTextureUnit(static_cast<GLuint>(o), lights[k]->shadowMap().depthTexture());
		++o;
		if (lights[k]->isRich()) {
			ostringstream oss6;
			oss6 << "light[" << k << "].richShadowMap";
			m_shader.setUniform(oss6.str(), static_cast<GLint>(o));
			glBindTextureUnit(static_cast<GLuint>(o), lights[k]->shadowMap().colorTexture(0));
			++o;
		}
	}

	m_shader.setUniform("isShadowMapEnabled", world.isShadowMapEnabled());

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
	m_framebuffer.setResolution(width, height);
}
