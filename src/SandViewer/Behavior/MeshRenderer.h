#pragma once

#include "Behavior.h"
#include "ShaderProgram.h"
#include "GlTexture.h"
#include "StandardMaterial.h"

#include <glm/glm.hpp>

#include <memory>

class MeshDataRenderer;
class TransformBehavior;
class MeshDataBehavior;

class MeshRenderer : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value& json) override;
	void start() override;
	void render(const Camera& camera, const World& world, RenderType target) const override;

private:
	glm::mat4 modelMatrix() const;

private:
	std::string m_shaderName = "Mesh";
	std::weak_ptr<MeshDataBehavior> m_meshData;
	std::weak_ptr<TransformBehavior> m_transform;
	std::shared_ptr<ShaderProgram> m_shader;
	std::vector<StandardMaterial> m_materials; // may be emtpy, in which case materials from MeshData are used
};

registerBehaviorType(MeshRenderer)

