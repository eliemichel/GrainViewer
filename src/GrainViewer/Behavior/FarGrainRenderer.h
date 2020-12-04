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

#pragma once

#include "Behavior.h"
#include "utils/ReflectionAttributes.h"

#include <refl.hpp>

#include <memory>
#include <string>

class TransformBehavior;
class GrainBehavior;
class ShaderProgram;
class IPointCloudData;
class GlTexture;

/**
 * A grain renderer focused on furthest grains, that are subpixelic.
 * It does epsilon-depth-testing.
 */
class FarGrainRenderer : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void update(float time, int frame) override;
	void render(const Camera & camera, const World & world, RenderType target) const override;

public:
	// Public properties
	enum DebugShape {
		DebugShapeNone = -1,
		DebugShapeRaytracedSphere = 0,
		DebugShapeDisc = 1,
		DebugShapeSquare = 2,
	};
	enum WeightMode {
		WeightModeNone = -1,
		WeightModeLinear = 0,
	};
	struct Properties {
		float radius = 0.007f;
		float epsilonFactor = 10.0f; // multiplied by radius
		bool useShellCulling = true;
		DebugShape debugShape = DebugShapeRaytracedSphere;
		WeightMode weightMode = WeightModeLinear;
		bool shellDepthFalloff = true;
		bool useEarlyDepthTest = true;
		bool noDiscard = false; // in eps (some artefacts on edges or with low shell depth, but faster)
		bool pseudoLean = false;

		bool useBbox = false; // if true, remove all points out of the supplied bounding box
		glm::vec3 bboxMin;
		glm::vec3 bboxMax;

		// For debug
		bool checkboardSprites = false;
		bool showSampleCount = false;
	};
	Properties & properties() { return m_properties; }
	const Properties & properties() const { return m_properties; }

private:
	// When adding flag, don't forget to add the corresponding define in the
	// definition of s_shaderVariantDefines in cpp file.
	// TODO: distinguish stages from options
	enum ShaderVariantFlags {
		ShaderOptionShellCulling = 1 << 0,
		ShaderPassDepth = 1 << 1,
		ShaderPassEpsilonDepth = 1 << 2,
		ShaderPassBlitToMainFbo = 1 << 3,
		ShaderOptionNoDiscard = 1 << 4,
		ShaderOptionPseudoLean = 1 << 5,
		_ShaderVariantFlagsCount = 1 << 6,
	};
	typedef int ShaderVariantFlagSet;
	static const std::vector<std::string> s_shaderVariantDefines;

private:
	void draw(const IPointCloudData& pointData) const;
	void renderToGBuffer(const IPointCloudData& pointData, const Camera& camera, const World& world) const;
	void renderToShadowMap(const IPointCloudData& pointData, const Camera& camera, const World& world) const;
	glm::mat4 modelMatrix() const;
	GLint setCommonUniforms(const ShaderProgram & shader, const Camera & camera, GLint nextTextureUnit = 0) const;
	void bindDepthTexture(ShaderProgram & shader, GLuint textureUnit = 7) const;
	std::shared_ptr<ShaderProgram> getShader(ShaderVariantFlagSet flags = 0) const;

public:
	std::string m_shaderName = "FarGrain";
	std::string m_colormapTextureName = "";
	Properties m_properties;

	// One shader by combination of flags
	mutable std::vector<std::shared_ptr<ShaderProgram>> m_shaders; // mutable for lazy loading, do NOT use this directly, rather use getShader()
	std::weak_ptr<TransformBehavior> m_transform;
	std::weak_ptr<GrainBehavior> m_grain;
	std::weak_ptr<IPointCloudData> m_pointData;
	std::unique_ptr<GlTexture> m_colormapTexture;

	std::shared_ptr<Framebuffer> m_depthFbo;

	float m_time;
};

using namespace ReflectionAttributes;
REFL_TYPE(FarGrainRenderer::Properties)
REFL_FIELD(radius, Range(0.0f, 0.1f))
REFL_FIELD(epsilonFactor, Range(0.01f, 20.0f))
REFL_FIELD(useShellCulling)
REFL_FIELD(debugShape)
REFL_FIELD(weightMode)
REFL_FIELD(shellDepthFalloff)
REFL_FIELD(useEarlyDepthTest)
REFL_FIELD(noDiscard)
REFL_FIELD(pseudoLean)
REFL_FIELD(useBbox)
REFL_FIELD(bboxMin, Range(-1.0f, 1.0f))
REFL_FIELD(bboxMax, Range(-1.0f, 1.0f))
REFL_FIELD(checkboardSprites)
REFL_FIELD(showSampleCount)
REFL_END

registerBehaviorType(FarGrainRenderer)
