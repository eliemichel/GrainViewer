
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
class SandBehavior;
class IPointCloudData;
class PointCloudSplitter;

class ImpostorSandRenderer : public Behavior {
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
	std::shared_ptr<ShaderProgram> ImpostorSandRenderer::getShader(ShaderVariantFlagSet flags) const;

private:
	Properties m_properties;

	std::string m_shaderName = "ImpostorSand";
	mutable std::vector<std::shared_ptr<ShaderProgram>> m_shaders;

	std::weak_ptr<TransformBehavior> m_transform;
	std::weak_ptr<SandBehavior> m_sand;
	std::weak_ptr<IPointCloudData> m_pointData;
	std::weak_ptr<PointCloudSplitter> m_splitter;

	std::unique_ptr<GlTexture> m_colormapTexture;
	std::unique_ptr<GlBuffer> m_precomputedViewMatrices;

	float m_time;
};

REFL_TYPE(ImpostorSandRenderer::Properties)
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

registerBehaviorType(ImpostorSandRenderer)
