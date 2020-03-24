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
		DepthPass,
		RawGBuffer0,
		RawGBuffer1,
		RawGBuffer2,
	};
	struct Properties {
		ShadingMode shadingMode = BeautyPass;
		bool transparentFilm = false;
		bool showSampleCount = false;
		float maxSampleCount = 10;
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

	Properties & properties() { return m_properties; }
	const Properties & properties() const { return m_properties; }

	bool hasColorMap() const { return m_colormap != nullptr; }
	// Use only if hasColorMap is truc
	const GlTexture & colormap() const { return *m_colormap; }

private:
	ShaderProgram m_shader;
	Framebuffer m_framebuffer;
	GLuint m_vao;
	std::unique_ptr<GlTexture> m_colormap; // colormap used as ramp for outputting debug images

	Properties m_properties;
};

