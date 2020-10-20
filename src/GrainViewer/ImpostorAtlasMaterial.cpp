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

#include "ImpostorAtlasMaterial.h"
#include "ShaderProgram.h"
#include "utils/jsonutils.h"
#include "utils/fileutils.h"
#include "ResourceManager.h"
#include "ShaderPool.h"
#include "Filtering.h"
#include "utils/impostor.glsl.h"
#include "Framebuffer2.h"
#include "Behavior/MeshDataBehavior.h"
#include "utils/ScopedFramebufferOverride.h"

#include <glm/gtc/type_ptr.hpp>

bool ImpostorAtlasMaterial::deserialize(const rapidjson::Value& json)
{
	bool baseColorOverriden = jrOption(json, "baseColor", baseColor, baseColor);
	bool metallicOverriden = jrOption(json, "metallic", metallic, metallic);
	bool roughnessOverriden = jrOption(json, "roughness", roughness, roughness);

	jrOption(json, "bake", bake, bake);
	if (bake) {
		std::string filename;
		if (!jrOption(json, "filename", filename, filename)) {
			ERR_LOG << "An obj filename must be given when baking atlas.";
			return false;
		}
		jrOption(json, "angularDefinition", angularDefinition, angularDefinition);
		jrOption(json, "spatialDefinition", spatialDefinition, spatialDefinition);
		viewCount = static_cast<GLuint>(sqrt(angularDefinition / 2));
		angularDefinition = static_cast<int>(2 * viewCount * viewCount);

		DEBUG_LOG << "Loading object '" << filename << "' for backing...";
		MeshDataBehavior mesh;
		{
			using namespace rapidjson;
			Document d(kObjectType);
			Value meshOpt(kObjectType);

			Value filenameValue;
			filenameValue.SetString(filename.c_str(), d.GetAllocator());
			meshOpt.AddMember("filename", filenameValue, d.GetAllocator());

			Value boundingValue;
			boundingValue.SetBool(true);
			meshOpt.AddMember("computeBoundingSphere", boundingValue, d.GetAllocator());

			mesh.deserialize(meshOpt);
		}
		mesh.start();
		bakeMaps(mesh, mesh.boundingSphereRadius(), mesh.boundingSphereCenter());

		bool save = false;
		jrOption(json, "save", save, save);
		if (save) {
#define writeAtlas(name) \
			std::string name; \
			if (jrOption(json, #name, name)) { \
				ResourceManager::saveTextureStack(name, *name ## Texture); \
			}
			writeAtlas(normalAlpha);
			writeAtlas(baseColor);
			writeAtlas(metallicRoughness);
#undef writeAtlas
		}
	}
	else {
#define readAtlas(name) \
		std::string name; \
		if (jrOption(json, #name, name)) { \
			name ## Texture = ResourceManager::loadTextureStack(name); \
		}
		readAtlas(normalAlpha);
		readAtlas(baseColor);
		readAtlas(metallicRoughness);
#undef readAtlas
	}

	GLuint n = normalAlphaTexture->depth();
	viewCount = static_cast<GLuint>(sqrt(n / 2));

	if (baseColorTexture && normalAlphaTexture) {
		Filtering::MipMapUsingAlpha(*baseColorTexture, *normalAlphaTexture);
	}
	if (metallicRoughnessTexture && normalAlphaTexture) {
		Filtering::MipMapUsingAlpha(*metallicRoughnessTexture, *normalAlphaTexture);
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

	if (baseColorTexture) {
		baseColorTexture->bind(o);
		shader.setUniform(prefix + "baseColorTexture", o++);
	}
	shader.setUniform(prefix + "hasBaseColorMap", static_cast<bool>(baseColorTexture));

	if (metallicRoughnessTexture) {
		metallicRoughnessTexture->bind(o);
		shader.setUniform(prefix + "metallicRoughnessTexture", o++);
	}
	shader.setUniform(prefix + "hasMetallicRoughnessMap", static_cast<bool>(metallicRoughnessTexture));
	
	return o;
}


static std::unique_ptr<GlTexture> initTexture(GLsizei width, GLsizei depth, GLsizei levels, GLenum internalformat = GL_RGBA8)
{
	auto texture = std::make_unique<GlTexture>(GL_TEXTURE_2D_ARRAY);
	texture->setWrapMode(GL_CLAMP_TO_EDGE);
	texture->storage(levels, internalformat, width, width, depth);
	return texture;
}

void ImpostorAtlasMaterial::bakeMaps(const MeshDataBehavior & mesh, float scale, glm::vec3 center)
{
	ScopedFramebufferOverride framebufferOverride;
	GLsizei width = static_cast<GLsizei>(spatialDefinition);
	GLsizei depth = static_cast<GLsizei>(angularDefinition);
	GLsizei levels = static_cast<GLsizei>(1 + floor(log2(width)));
	DEBUG_LOG << "Baking impostor atlas of size " << width << "x" << width << "x" << depth;
	LOG << "A scale factor of " << scale << " and an offset of (" << center.x << ", " << center.y << ", " << center.z << ") are applied to the object at bake time";

	int msaa = 2;
	float onePixel = 2.0f / static_cast<float>(width);
	
	normalAlphaTexture = initTexture(width, depth, levels);
	baseColorTexture = initTexture(width, depth, levels);
	metallicRoughnessTexture = initTexture(width, depth, levels);

	// temp textures for msaa
	auto depthMsaa = initTexture(width, depth, levels, GL_DEPTH_COMPONENT24);
	auto normalAlphaMsaa = initTexture(width, depth, levels);
	auto baseColorMsaa = initTexture(width, depth, levels);
	auto metallicRoughnessMsaa = initTexture(width, depth, levels);

	Framebuffer2 fbo;
	fbo.attachTexture(0, *normalAlphaTexture, 0);
	fbo.attachTexture(1, *baseColorTexture, 0);
	fbo.attachTexture(2, *metallicRoughnessTexture, 0);
	fbo.enableDrawBuffers(3);
	fbo.bind();

	Framebuffer2 fboMsaa;
	fboMsaa.attachDepthTexture(*depthMsaa, 0);
	fboMsaa.attachTexture(0, *normalAlphaMsaa, 0);
	fboMsaa.attachTexture(1, *baseColorMsaa, 0);
	fboMsaa.attachTexture(2, *metallicRoughnessMsaa, 0);
	fboMsaa.enableDrawBuffers(3);

	glViewport(0, 0, width, width);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const ShaderProgram & shader = *ShaderPool::GetShader("BakeImpostorAtlas");
	const ShaderProgram& blitShader = *ShaderPool::GetShader("BakeImpostorAtlas_Blit");

	shader.setUniform("uModelMatrix", glm::translate(glm::mat4(1), -center));
	shader.setUniform("uProjectionMatrix", glm::ortho(-scale, scale, -scale , scale, -scale, scale));
	shader.setUniform("uReducedViewCount", viewCount);

	GLuint o = 0;
	int n = static_cast<int>(mesh.materials().size());
	for (int i = 0; i < n; ++i) {
		const StandardMaterial& mat = mesh.materials()[i];
		o = mat.setUniforms(shader, MAKE_STR("uMaterial[" << i << "]."), o);
	}

	blitShader.setUniform("uMultiplier", 1.0f / static_cast<float>(msaa * msaa));
	normalAlphaMsaa->bind(0);
	blitShader.setUniform("uNormalAlpha", 0);
	baseColorMsaa->bind(1);
	blitShader.setUniform("uBaseColor", 1);
	metallicRoughnessMsaa->bind(2);
	blitShader.setUniform("uMetallicRoughness", 2);

	for (int xx = 0; xx < msaa; ++xx) {
		for (int yy = 0; yy < msaa; ++yy) {
			
			float dx = xx / static_cast<float>(msaa - 1) - 0.5f;
			float dy = yy / static_cast<float>(msaa - 1) - 0.5f;

			shader.setUniform("uScreenSpaceOffset", glm::vec2(dx * onePixel, dy * onePixel));

			fboMsaa.bind();
			shader.use();
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindVertexArray(mesh.vao());
			glDrawArraysInstanced(GL_TRIANGLES, 0, mesh.pointCount(), angularDefinition);
			glBindVertexArray(0);

			fbo.bind();
			blitShader.use();
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			PostEffect::DrawInstanced(angularDefinition);
		}
	}

	glTextureBarrier();

	normalAlphaTexture->generateMipmap();
	baseColorTexture->generateMipmap();
	metallicRoughnessTexture->generateMipmap();
}
