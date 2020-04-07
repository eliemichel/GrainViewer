#pragma once

#include <OpenGL>
#include "GlTexture.h"
#include "Filtering.h"

#include <glm/glm.hpp>
#include <rapidjson/document.h>

#include <string>
#include <memory>

class ShaderProgram;

/**
 * Matches SphericalImpostor struct in include/impostor.inc.glsl
 */
struct ImpostorAtlasMaterial
{
	bool enableLeanMapping;
	std::unique_ptr<GlTexture> normalAlphaTexture;
	std::unique_ptr<GlTexture> baseColorTexture;
	std::unique_ptr<GlTexture> metallicRoughnessTexture;
	glm::vec3 baseColor;
	float metallic = 0.0;
	float roughness = 0.5;

	GLuint viewCount; // number of precomputed views, computed from normalAlphaTexture depth
	std::unique_ptr<LeanTexture> leanTextures; // generated iff enableLeanMapping is on

	bool deserialize(const rapidjson::Value& json);
	GLint setUniforms(const ShaderProgram& shader, const std::string& prefix, GLint nextTextureUnit) const;
};
