#pragma once

#include "Behavior.h"
#include "ShaderProgram.h"
#include "GlTexture.h"
#include "StandardMaterial.h"

#include <refl.hpp>
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

public:
	struct Properties {
		float normalMapping = 1.0f;
	};
	Properties& properties() { return m_properties; }
	const Properties& properties() const { return m_properties; }

private:
	glm::mat4 modelMatrix() const;

private:
	Properties m_properties;
	std::string m_shaderName = "Mesh";
	std::weak_ptr<MeshDataBehavior> m_meshData;
	std::weak_ptr<TransformBehavior> m_transform;
	std::shared_ptr<ShaderProgram> m_shader;
	std::vector<StandardMaterial> m_materials; // may be emtpy, in which case materials from MeshData are used
};

#define _ ReflectionAttributes::
REFL_TYPE(MeshRenderer::Properties)
REFL_FIELD(normalMapping, _ Range(0, 2))
REFL_END
#undef _

registerBehaviorType(MeshRenderer)
