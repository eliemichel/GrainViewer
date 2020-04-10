// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once


#include "ShaderProgram.h"
#include "Framebuffer.h"
#include "Camera.h"
#include "World.h"
#include "RenderType.h"
#include "utils/ReflectionAttributes.h"

#include <glm/glm.hpp>
#include <rapidjson/document.h>
#include <refl.hpp>

#include <vector>
#include <memory>

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
		float shadowMapBiasBase = 0.1f;
		int shadowMapBiasExponent = -5;

		float ShadowMapBias() const;
	};
public:
	GlDeferredShader();
	~GlDeferredShader();

	void addShaderDefine(const std::string & def) { m_shader.define(def); }

	void bindFramebuffer(const Camera& camera) const;

	bool deserialize(const rapidjson::Value & json);
	void reloadShaders();
	void update(float time);

	void render(const Camera & camera, const World & world, RenderType target) const;

	Properties & properties() { return m_properties; }
	const Properties & properties() const { return m_properties; }

	bool hasColorMap() const { return m_colormap != nullptr; }
	// Use only if hasColorMap is truc
	const GlTexture & colormap() const { return *m_colormap; }

	void setBlitOffset(GLint x, GLint y) { m_blitOffset = glm::vec2(static_cast<float>(x), static_cast<float>(y)); }

private:
	Properties m_properties;
	ShaderProgram m_shader;
	GLuint m_vao;
	std::unique_ptr<GlTexture> m_colormap; // colormap used as ramp for outputting debug images
	glm::vec2 m_blitOffset = glm::vec2(0.0f); // offset when writing to output framebuffer
};

#define _ ReflectionAttributes::
REFL_TYPE(GlDeferredShader::Properties)
REFL_FIELD(shadingMode)
REFL_FIELD(transparentFilm)
REFL_FIELD(showSampleCount)
REFL_FIELD(maxSampleCount, _ HideInDialog())
REFL_FIELD(shadowMapBiasBase)
REFL_FIELD(shadowMapBiasExponent, _ Range(-8, 2))
REFL_END
#undef _
