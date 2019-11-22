#pragma once

#include <memory>

#include "Behavior.h"
#include "ShaderProgram.h"

class MeshDataRenderer;

class MeshRenderer : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value& json) override;
	void start() override;
	void render(const Camera& camera, const World& world, RenderType target) const override;
	void reloadShaders() override;

private:
	std::string m_shaderName;
	std::weak_ptr<MeshDataBehavior> m_meshData;
	std::unique_ptr<ShaderProgram> m_shader;

	glm::mat4 m_modelMatrix; // TODO: Move to a dedicated TransformBehavior
};

registerBehaviorType(MeshRenderer)

