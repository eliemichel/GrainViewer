#pragma once

#include <memory>
#include <string>
#include <refl.hpp>
#include "Behavior.h"
#include "utils/ReflectionAttributes.h"

class TransformBehavior;
class ShaderProgram;
class IPointCloudData;
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
		DebugShape debugShape = DebugShapeDisc;
		WeightMode weightMode = WeightModeLinear;
		bool shellDepthFalloff = true;
		bool useEarlyDepthTest = true;

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
		_ShaderVariantFlagsCount = 1 << 4,
	};
	typedef int ShaderVariantFlagSet;
	static const std::vector<std::string> s_shaderVariantDefines;

private:
	void draw(const IPointCloudData& pointData) const;
	void renderToGBuffer(const IPointCloudData& pointData, const Camera& camera, const World& world) const;
	void renderToShadowMap(const IPointCloudData& pointData, const Camera& camera, const World& world) const;
	glm::mat4 modelMatrix() const;
	void setCommonUniforms(const ShaderProgram & shader, const Camera & camera) const;
	void bindDepthTexture(ShaderProgram & shader, GLuint textureUnit = 7) const;
	std::shared_ptr<ShaderProgram> getShader(ShaderVariantFlagSet flags = 0) const;

public:
	std::string m_shaderName = "UberSand";
	std::string m_colormapTextureName = "";
	Properties m_properties;

	// One shader by combination of flags
	mutable std::vector<std::shared_ptr<ShaderProgram>> m_shaders; // mutable for lazy loading, do NOT use this directly, rather use getShader()
	std::weak_ptr<TransformBehavior> m_transform;
	std::weak_ptr<IPointCloudData> m_pointData;
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
REFL_FIELD(shellDepthFalloff)
REFL_FIELD(useEarlyDepthTest)
REFL_FIELD(useBbox)
REFL_FIELD(bboxMin, Range(-1.0f, 1.0f))
REFL_FIELD(bboxMax, Range(-1.0f, 1.0f))
REFL_FIELD(checkboardSprites)
REFL_FIELD(showSampleCount)
REFL_END

registerBehaviorType(UberSandRenderer)
