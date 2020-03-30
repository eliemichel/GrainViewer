#pragma once

#include <memory>
#include <string>
#include <refl.hpp>
#include "Behavior.h"
#include "utils/ReflectionAttributes.h"

class TransformBehavior;
class ShaderProgram;
class PointCloudDataBehavior;
class GlTexture;

/**
 * A sand renderer focused on far grains, not implementing the closer models
 * namely impostors and instances. It does epsilon-depth-testing
 */
class UberSandRenderer : public Behavior {
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
		DebugShapeLitSphere = 0,
		DebugShapeDisc = 1,
		DebugShapeSquare = 2,
		DebugShapeNormalSphere = 3
	};
	enum WeightMode {
		WeightModeNone = -1,
		WeightModeLinear = 0,
		WeightModeQuad = 1,
		WeightModeGaussian = 2,
	};
	enum ShellCullingStrategy {
		ShellCullingFragDepth = 0, // Write to gl_FragDepth (performance issue)
		ShellCullingMoveAway = 1, // Move points away from camera (maybe occlusion issue with other objects)
		ShellCullingDepthRange = 2, // Play with DepthRange for an efficient depth buffer offset, but inexact because offset is in log space
	};
	struct Properties {
		float radius = 0.007f;
		float epsilonFactor = 10.0f; // multiplied by radius
		bool useShellCulling = true;
		DebugShape debugShape = DebugShapeDisc;
		WeightMode weightMode = WeightModeLinear;
		ShellCullingStrategy shellCullingStrategy = ShellCullingMoveAway;
		bool shellDepthFalloff = true;
		bool useEarlyDepthTest = true;
		bool noDiscardInEpsilonZPass = false;
		bool extraInitPass = false;
		bool extraFbo = true; // use an extra fbo for pre-rendering... seems like the best option

		bool useBbox = false; // if true, remove all points out of the supplied bbox
		glm::vec3 bboxMin;
		glm::vec3 bboxMax;

		// For debug
		bool disableBlend = false;
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
		ShaderVariantEpsilonZPass = 1 << 0,
		ShaderVariantNoDiscard = 1 << 1,
		ShaderVariantNoColor = 1 << 2,
		ShaderVariantFragDepth = 1 << 3,
		ShaderVariantExtraInitPass = 1 << 4, // Add an extra initialisation before accumulation to avoid uninitialized pixels when using ShaderVariantNoDiscard
		ShaderVariantBlitToMainFbo = 1 << 5,
		ShaderVariantUsingExtraFbo = 1 << 6,
		_ShaderVariantFlagsCount = 1 << 7,
	};
	typedef int ShaderVariantFlagSet;
	static const std::vector<std::string> s_shaderVariantDefines;

private:
	void renderToShadowMap(const Camera& camera, const World& world, RenderType target) const;
	glm::mat4 modelMatrix() const;
	void setCommonUniforms(ShaderProgram & shader, const Camera & camera) const;
	void setDepthUniform(ShaderProgram & shader) const;
	float depthRangeBias(const Camera & camera) const;
	std::shared_ptr<ShaderProgram> getShader(ShaderVariantFlagSet flags) const;

public:
	std::string m_shaderName = "FarSand";
	std::string m_colormapTextureName = "";
	Properties m_properties;

	// One shader by combination of flags
	mutable std::vector<std::shared_ptr<ShaderProgram>> m_shaders; // mutable for lazy loading, do NOT use this directly, rather use getShader()
	std::weak_ptr<TransformBehavior> m_transform;
	std::weak_ptr<PointCloudDataBehavior> m_pointData;
	std::unique_ptr<GlTexture> m_colormapTexture;

	std::shared_ptr<Framebuffer> m_depthFbo;

	float m_time;
};

using namespace ReflectionAttributes;
REFL_TYPE(UberSandRenderer::Properties)
REFL_FIELD(radius, Range(0.0f, 0.1f))
REFL_FIELD(epsilonFactor, Range(0.01f, 20.0f))
REFL_FIELD(useShellCulling)
REFL_FIELD(debugShape)
REFL_FIELD(weightMode)
REFL_FIELD(shellCullingStrategy)
REFL_FIELD(shellDepthFalloff)
REFL_FIELD(useEarlyDepthTest)
REFL_FIELD(noDiscardInEpsilonZPass)
REFL_FIELD(extraInitPass)
REFL_FIELD(extraFbo)
REFL_FIELD(useBbox)
REFL_FIELD(bboxMin, Range(-1.0f, 1.0f))
REFL_FIELD(bboxMax, Range(-1.0f, 1.0f))
REFL_FIELD(disableBlend)
REFL_FIELD(checkboardSprites)
REFL_FIELD(showSampleCount, HideInDialog())
REFL_END

registerBehaviorType(UberSandRenderer)
