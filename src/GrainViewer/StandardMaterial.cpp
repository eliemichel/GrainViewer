/**
 * This file is part of GrainViewer
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#include "StandardMaterial.h"
#include "ShaderProgram.h"
#include "utils/jsonutils.h"
#include "utils/fileutils.h"
#include "ResourceManager.h"

#include <glm/gtc/type_ptr.hpp>
#include <tiny_obj_loader.h>

bool StandardMaterial::deserialize(const rapidjson::Value& json)
{
#define readAtlas(name) \
	std::string name; \
	if (jrOption(json, #name, name)) { \
		name ## Map = ResourceManager::loadTexture(name); \
	}
	readAtlas(baseColor);
	readAtlas(normal);
	readAtlas(metallicRoughness);
	readAtlas(metallic);
	readAtlas(roughness);
#undef readAtlas

	jrOption(json, "baseColor", baseColor, baseColor);
	jrOption(json, "metallic", metallic, metallic);
	jrOption(json, "roughness", roughness, roughness);

	return true;
}

void StandardMaterial::fromTinyObj(const tinyobj::material_t& mat, const std::string & textureRoot)
{
	baseColor = glm::make_vec3(mat.diffuse);
	roughness = mat.roughness;
	metallic = mat.metallic;
	if (!mat.diffuse_texname.empty()) {
		baseColorMap = ResourceManager::loadTexture(joinPath(textureRoot, mat.diffuse_texname));
	}
	if (!mat.metallic_texname.empty()) {
		metallicMap = ResourceManager::loadTexture(joinPath(textureRoot, mat.metallic_texname));
	}
	if (!mat.reflection_texname.empty()) {
		metallicMap = ResourceManager::loadTexture(joinPath(textureRoot, mat.reflection_texname));
	}
	if (!mat.specular_highlight_texname.empty()) {
		roughnessMap = ResourceManager::loadTexture(joinPath(textureRoot, mat.specular_highlight_texname));
	}
	if (!mat.roughness_texname.empty()) {
		roughnessMap = ResourceManager::loadTexture(joinPath(textureRoot, mat.roughness_texname));
	}
	if (!mat.bump_texname.empty()) {
		normalMap = ResourceManager::loadTexture(joinPath(textureRoot, mat.bump_texname));
	}
	if (!mat.normal_texname.empty()) {
		normalMap = ResourceManager::loadTexture(joinPath(textureRoot, mat.normal_texname));
	}
}

GLuint StandardMaterial::setUniforms(const ShaderProgram& shader, const std::string& prefix, GLuint nextTextureUnit) const
{
	GLint o = nextTextureUnit;

	if (baseColorMap) {
		baseColorMap->bind(o);
		shader.setUniform(prefix + "baseColorMap", o++);
	}
	shader.setUniform(prefix + "hasBaseColorMap", static_cast<bool>(baseColorMap));
	
	if (normalMap) {
		normalMap->bind(o);
		shader.setUniform(prefix + "normalMap", o++);
	}
	shader.setUniform(prefix + "hasNormalMap", static_cast<bool>(normalMap));

	if (metallicRoughnessMap) {
		metallicRoughnessMap->bind(o);
		shader.setUniform(prefix + "metallicRoughnessMap", o++);
	}
	shader.setUniform(prefix + "hasMetallicRoughnessMap", static_cast<bool>(metallicRoughnessMap));

	if (metallicMap) {
		metallicMap->bind(o);
		shader.setUniform(prefix + "metallicMap", o++);
	}
	shader.setUniform(prefix + "hasMetallicMap", static_cast<bool>(metallicMap));

	if (roughnessMap) {
		roughnessMap->bind(o);
		shader.setUniform(prefix + "roughnessMap", o++);
	}
	shader.setUniform(prefix + "hasRoughnessMap", static_cast<bool>(roughnessMap));

	shader.setUniform(prefix + "baseColor", baseColor);
	shader.setUniform(prefix + "metallic", metallic);
	shader.setUniform(prefix + "roughness", roughness);

	return o;
}
