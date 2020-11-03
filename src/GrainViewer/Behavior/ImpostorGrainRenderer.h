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

#include <OpenGL>
#include "Behavior.h"
#include "GlTexture.h"
#include "GlBuffer.h"
#include "Filtering.h"
#include "ImpostorAtlasMaterial.h"

#include <refl.hpp>
#include <glm/glm.hpp>

#include <memory>

class ShaderProgram;
class TransformBehavior;
class GrainBehavior;
class IPointCloudData;
class PointCloudSplitter;

class ImpostorGrainRenderer : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void update(float time, int frame) override;
	void render(const Camera& camera, const World& world, RenderType target) const override;

public:
	// Properties (serialized and displayed in UI)
	enum class DebugShape {
		None = -1,
		Sphere,
		InnerSphere,
		Cube,
	};
	enum class InterpolationMode {
		None,
		Linear,
	};
	enum class SamplingMode {
		Plane,
		Sphere,
		Mixed
	};
	struct Properties {
		float grainScale = 1.0f;
		DebugShape debugShape = DebugShape::None;
		InterpolationMode interpolationMode = InterpolationMode::Linear;
		SamplingMode samplingMode = SamplingMode::Plane;
		bool noDiscard = false;
		bool precomputeViewMatrices = false; // requires that all impostors use the same number of views
		bool precomputeInVertex = true; // precompute more things in vertex shader
		bool prerenderSurface = true;
		bool firstPassOnly = false; // when using prerenderSurface, only draw the surface
		float hitSphereCorrectionFactor = 0.65f;
	};
	Properties & properties() { return m_properties; }
	const Properties& properties() const { return m_properties; }

private:
	// When adding flag, don't forget to add the corresponding define in the
	// definition of s_shaderVariantDefines in cpp file.
	// TODO: distinguish stages from options
	enum ShaderVariantFlag {
		ShaderOptionNoDiscard = 1 << 0,
		ShaderPassShadow = 1 << 1,
		ShaderPassBlitToMainFbo = 1 << 2,
		ShaderOptionNoInterpolation = 1 << 3,
		ShaderOptionPrecomputeViewMatrices = 1 << 4,
		ShaderOptionPrecomputeInVertex = 1 << 5,
	};
	typedef int ShaderVariantFlagSet;
	static const std::vector<std::string> s_shaderVariantDefines;

private:
	void draw(const IPointCloudData& pointData, const ShaderProgram& shader) const;
	void setCommonUniforms(const ShaderProgram& shader, const Camera& camera) const;
	void precomputeViewMatrices();
	glm::mat4 modelMatrix() const;
	std::shared_ptr<ShaderProgram> getShader(ShaderVariantFlagSet flags) const;

private:
	Properties m_properties;

	std::string m_shaderName = "ImpostorGrain";
	mutable std::vector<std::shared_ptr<ShaderProgram>> m_shaders;

	std::weak_ptr<TransformBehavior> m_transform;
	std::weak_ptr<GrainBehavior> m_grain;
	std::weak_ptr<IPointCloudData> m_pointData;
	std::weak_ptr<PointCloudSplitter> m_splitter;

	std::unique_ptr<GlTexture> m_colormapTexture;
	std::unique_ptr<GlBuffer> m_precomputedViewMatrices;

	float m_time;
};

REFL_TYPE(ImpostorGrainRenderer::Properties)
REFL_FIELD(grainScale)
REFL_FIELD(debugShape)
REFL_FIELD(interpolationMode)
REFL_FIELD(samplingMode)
REFL_FIELD(noDiscard)
REFL_FIELD(precomputeViewMatrices)
REFL_FIELD(precomputeInVertex)
REFL_FIELD(prerenderSurface)
REFL_FIELD(firstPassOnly)
REFL_FIELD(hitSphereCorrectionFactor)
REFL_END

registerBehaviorType(ImpostorGrainRenderer)
