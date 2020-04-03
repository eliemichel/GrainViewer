
#pragma once

#include <OpenGL>
#include "Behavior.h"
#include "GlTexture.h"
#include "Filtering.h"
#include <refl.hpp>
#include <glm/glm.hpp>
#include <memory>

class ShaderProgram;
class TransformBehavior;
class SandBehavior;
class IPointCloudData;

// Matches SphericalImpostor struct in include/impostor.inc.glsl
struct ImpostorAtlas {
	bool enableLeanMapping;
	std::unique_ptr<GlTexture> normalAlphaTexture;
	std::unique_ptr<GlTexture> baseColorTexture;
	std::unique_ptr<GlTexture> metallicRoughnessTexture;

	GLuint viewCount; // number of precomputed views, computed from normalAlphaTexture depth
	std::unique_ptr<LeanTexture> leanTextures; // generated iff enableLeanMapping is on

	bool deserialize(const rapidjson::Value& json);
	void setUniforms(const ShaderProgram& shader, GLint & textureUnit, const std::string& prefix) const;
};

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
		DebugShape debugShape = DebugShape::None;
		InterpolationMode interpolationMode = InterpolationMode::Linear;
		SamplingMode samplingMode = SamplingMode::Plane;
		float hitSphereCorrectionFactor = 0.65f;
	};
	Properties & properties() { return m_properties; }
	const Properties& properties() const { return m_properties; }

private:
	glm::mat4 modelMatrix() const;

private:
	Properties m_properties;
	std::vector<ImpostorAtlas> m_atlases;

	std::string m_shaderName = "ImpostorSand";
	std::shared_ptr<ShaderProgram> m_shader;
	std::shared_ptr<ShaderProgram> m_shadowMapShader;

	std::weak_ptr<TransformBehavior> m_transform;
	std::weak_ptr<SandBehavior> m_sand;
	std::weak_ptr<IPointCloudData> m_pointData;

	std::unique_ptr<GlTexture> m_colormapTexture;

	float m_time;
};

REFL_TYPE(ImpostorSandRenderer::Properties)
REFL_FIELD(debugShape)
REFL_FIELD(interpolationMode)
REFL_FIELD(samplingMode)
REFL_FIELD(hitSphereCorrectionFactor)
REFL_END

registerBehaviorType(ImpostorSandRenderer)
