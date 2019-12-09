#pragma once

#include <memory>
#include <string>
#include "Behavior.h"

class TransformBehavior;
class ShaderProgram;
class PointCloudDataBehavior;
class GlTexture;

class FarSandRenderer : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void render(const Camera & camera, const World & world, RenderType target) const override;

public:
	// Public properties
	struct Properties {
		float radius = 0.007f;
	};
	Properties & properties() { return m_properties; }
	const Properties & properties() const { return m_properties; }

private:
	glm::mat4 modelMatrix() const;

public:
	std::string m_shaderName;
	std::string m_colormapTextureName = "";
	Properties m_properties;

	std::shared_ptr<ShaderProgram> m_shader;
	std::weak_ptr<TransformBehavior> m_transform;
	std::weak_ptr<PointCloudDataBehavior> m_pointData;
	std::unique_ptr<GlTexture> m_colormapTexture;
};

registerBehaviorType(FarSandRenderer)
