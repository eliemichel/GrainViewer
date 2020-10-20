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
		bool debugVectors = false;
		float debugVectorsScale = 0.1f;
		float debugVectorsGrid = 20.0f;
		float shadowMapBiasBase = 0.1f;
		int shadowMapBiasExponent = -5;

		float ShadowMapBias() const;
	};
public:
	GlDeferredShader();
	~GlDeferredShader();

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
	ShaderProgram m_shader, m_debugShader;
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
REFL_FIELD(debugVectors)
REFL_FIELD(debugVectorsScale, _ Range(0, 2))
REFL_FIELD(debugVectorsGrid, _ Range(0, 40))
REFL_FIELD(shadowMapBiasBase)
REFL_FIELD(shadowMapBiasExponent, _ Range(-8, 2))
REFL_END
#undef _
