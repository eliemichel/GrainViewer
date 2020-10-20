#pragma once

#include <OpenGL>
#include "GlTexture.h"

#include <glm/glm.hpp>
#include <rapidjson/document.h>
#include <tiny_obj_loader.h>

#include <string>
#include <memory>

class ShaderProgram;


struct StandardMaterial
{
	glm::vec3 baseColor = glm::vec3(1.0f, 0.5f, 0.0f);
	GLfloat metallic = 0.0f;
	GLfloat roughness = 0.5f;
	std::unique_ptr<GlTexture> baseColorMap;
	std::unique_ptr<GlTexture> normalMap;
	std::unique_ptr<GlTexture> metallicRoughnessMap;
	std::unique_ptr<GlTexture> metallicMap;
	std::unique_ptr<GlTexture> roughnessMap;

	bool deserialize(const rapidjson::Value& json);
	void fromTinyObj(const tinyobj::material_t & mat, const std::string& textureRoot);
	// return the next available texture unit
	GLuint setUniforms(const ShaderProgram& shader, const std::string & prefix, GLuint nextTextureUnit) const;
};
