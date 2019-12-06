// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include <vector>
#include <memory>

#include <rapidjson/document.h>

#include "ShaderProgram.h"
#include "Framebuffer.h"
#include "Camera.h"
#include "World.h"
#include "RenderType.h"

class GlDeferredShader {
public:
	GlDeferredShader();
	~GlDeferredShader();

	void addShaderDefine(const std::string & def) { m_shader.define(def); }

	void bindFramebuffer() const { m_framebuffer.bind(); };

	bool deserialize(const rapidjson::Value & json);
	void reloadShaders();
	void update(float time);

	void render(const Camera & camera, const World & world, RenderType target) const;

	void setResolution(int width, int height);

private:
	ShaderProgram m_shader;
	Framebuffer m_framebuffer;
	GLuint m_vao;
};

