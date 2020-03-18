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

class GlTexture;

class GlDeferredShader {
public:
	enum ShadingMode {
		BeautyPass,
		NormalPass,
		BaseColorPass,
		MetallicPass,
		RoughnessPass,
		WorldPositionPass,
		RawGBuffer0,
		RawGBuffer1,
		RawGBuffer2,
	};
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

	ShadingMode shadingMode() const { return m_shadingMode; }
	void setShadingMode(ShadingMode shadingMode) { m_shadingMode = shadingMode; }

	bool transparentFilm() const { return m_transparentFilm; }
	void setTransparentFilm(bool transparent) { m_transparentFilm = transparent; }

private:
	ShaderProgram m_shader;
	Framebuffer m_framebuffer;
	GLuint m_vao;
	std::unique_ptr<GlTexture> m_colormap; // colormap used as ramp for outputting debug images

	ShadingMode m_shadingMode = BeautyPass;
	bool m_transparentFilm = false;
};

