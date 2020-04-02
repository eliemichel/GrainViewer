
#pragma once

#include <OpenGL>
#include "Behavior.h"
#include <refl.hpp>
#include <glm/glm.hpp>
#include <memory>

class ShaderProgram;
class TransformBehavior;

class ImpostorSandRenderer : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void update(float time, int frame) override;
	void render(const Camera& camera, const World& world, RenderType target) const override;

public:
	// Properties (serialized and displayed in UI)
	struct Properties {
		bool someOption = true;
	};
	Properties & properties() { return m_properties; }
	const Properties& properties() const { return m_properties; }

private:
	glm::mat4 modelMatrix() const;

private:
	Properties m_properties;

	std::string m_shaderName = "ImpostorSandRendererShader";
	std::shared_ptr<ShaderProgram> m_shader;

	std::weak_ptr<TransformBehavior> m_transform;
};

REFL_TYPE(ImpostorSandRenderer::Properties)
REFL_FIELD(someOption)
REFL_END

registerBehaviorType(ImpostorSandRenderer)
