#pragma once

#include <memory>

#include "Behavior.h"
#include "ShaderProgram.h"
#include "GlTexture.h"

class MeshDataRenderer;
class TransformBehavior;
class MeshDataBehavior;

class MeshRenderer : public Behavior {
public:
	class Material {
	public:
		glm::vec3 baseColor;
		GLfloat metallic;
		GLfloat roughness;
		std::unique_ptr<GlTexture> baseColorMap;
		std::unique_ptr<GlTexture> normalMap;
		std::unique_ptr<GlTexture> metallicRoughnessMap;

		GLuint setUniforms(const ShaderProgram & shader, GLuint id, GLuint firstTextureIndex) const;
		static void deserializeArray(const rapidjson::Value& json, const std::string & key, std::vector<Material> & output);
	};

public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value& json) override;
	void start() override;
	void render(const Camera& camera, const World& world, RenderType target) const override;

private:
	glm::mat4 modelMatrix() const;

private:
	std::string m_shaderName;
	std::weak_ptr<MeshDataBehavior> m_meshData;
	std::weak_ptr<TransformBehavior> m_transform;
	std::shared_ptr<ShaderProgram> m_shader;
	std::vector<Material> m_materials;
};

registerBehaviorType(MeshRenderer)

