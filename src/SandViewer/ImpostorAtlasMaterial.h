#pragma once

#include <OpenGL>
#include "GlTexture.h"
#include "Filtering.h"

#include <glm/glm.hpp>
#include <rapidjson/document.h>

#include <string>
#include <memory>

class ShaderProgram;
class MeshDataBehavior;

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

	bool bake = false;
	// If 'bake' is true, impostor is computed in engine at startup rather than being loaded from files.
	// in this case, the following options are used:
	int angularDefinition = 128; // rounded to the closest number such that 2n²
	int spatialDefinition = 128; // number of pixels on each dimension of a precomputed view

	bool deserialize(const rapidjson::Value& json);
	GLint setUniforms(const ShaderProgram& shader, const std::string& prefix, GLint nextTextureUnit) const;

private:
	void bakeMaps(const MeshDataBehavior& mesh, float radius, glm::vec3 center);
};
