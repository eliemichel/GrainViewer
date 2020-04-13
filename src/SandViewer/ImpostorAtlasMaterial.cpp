#include "ImpostorAtlasMaterial.h"
#include "ShaderProgram.h"
#include "utils/jsonutils.h"
#include "utils/fileutils.h"
#include "ResourceManager.h"
#include "Filtering.h"

#include <glm/gtc/type_ptr.hpp>

bool ImpostorAtlasMaterial::deserialize(const rapidjson::Value& json)
{
	jrOption(json, "enableLeanMapping", enableLeanMapping);

#define readAtlas(name) \
	std::string name; \
	if (jrOption(json, #name, name)) { \
		name ## Texture = ResourceManager::loadTextureStack(name); \
	}
	readAtlas(normalAlpha);
	readAtlas(baseColor);
	readAtlas(metallicRoughness);
#undef readAtlas

	jrOption(json, "metallic", metallic, metallic);
	jrOption(json, "roughness", roughness, roughness);

	if (normalAlphaTexture && enableLeanMapping) {
		leanTextures = Filtering::CreateLeanTexture(*normalAlphaTexture);
	}

	GLuint n = normalAlphaTexture->depth();
	viewCount = static_cast<GLuint>(sqrt(n / 2));

	if (baseColorTexture && normalAlphaTexture) {
		Filtering::MipMapUsingAlpha(*baseColorTexture, *normalAlphaTexture);
	}

	return true;
}

GLint ImpostorAtlasMaterial::setUniforms(const ShaderProgram& shader, const std::string& prefix, GLint nextTextureUnit) const
{
	GLint o = nextTextureUnit;

	shader.setUniform(prefix + "viewCount", viewCount);
	shader.setUniform(prefix + "baseColor", baseColor);
	shader.setUniform(prefix + "metallic", metallic);
	shader.setUniform(prefix + "roughness", roughness);

	normalAlphaTexture->bind(o);
	shader.setUniform(prefix + "normalAlphaTexture", o++);

	if (leanTextures) {
		leanTextures->lean1.bind(o);
		shader.setUniform(prefix + "lean1Texture", o++);
		leanTextures->lean2.bind(o);
		shader.setUniform(prefix + "lean2Texture", o++);
	}

	if (baseColorTexture) {
		baseColorTexture->bind(o);
		shader.setUniform(prefix + "baseColorTexture", o++);
	}
	shader.setUniform(prefix + "hasBaseColorMap", static_cast<bool>(baseColorTexture));

	if (metallicRoughnessTexture) {
		metallicRoughnessTexture->bind(o);
		shader.setUniform(prefix + "metallicRoughnesTexture", o++);
	}
	shader.setUniform(prefix + "hasMetallicRoughnessMap", static_cast<bool>(metallicRoughnessTexture));
	
	shader.setUniform("hasLeanMapping", enableLeanMapping);

	return o;
}

