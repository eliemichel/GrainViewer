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
	jrOption(json, "enableLeanMapping", enableLeanMapping);

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

	if (normalAlphaTexture && enableLeanMapping) {
		leanTextures = Filtering::CreateLeanTexture(*normalAlphaTexture);
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
		shader.setUniform(prefix + "metallicRoughnessTexture", o++);
	}
	shader.setUniform(prefix + "hasMetallicRoughnessMap", static_cast<bool>(metallicRoughnessTexture));
	
	shader.setUniform("hasLeanMapping", enableLeanMapping);

	return o;
}


static void initTexture(std::unique_ptr<GlTexture> & texture, GLsizei width, GLsizei depth, GLsizei levels)
{
	texture = std::make_unique<GlTexture>(GL_TEXTURE_2D_ARRAY);
	texture->setWrapMode(GL_CLAMP_TO_EDGE);
	texture->storage(levels, GL_RGBA8, width, width, depth);
}

void ImpostorAtlasMaterial::bakeMaps(const MeshDataBehavior & mesh, float scale, glm::vec3 center)
{
	ScopedFramebufferOverride framebufferOverride;
	GLsizei width = static_cast<GLsizei>(spatialDefinition);
	GLsizei depth = static_cast<GLsizei>(angularDefinition);
	GLsizei levels = static_cast<GLsizei>(1 + floor(log2(width)));
	DEBUG_LOG << "Baking impostor atlas of size " << width << "x" << width << "x" << depth;
	LOG << "A scale factor of " << scale << " and an offset of (" << center.x << ", " << center.y << ", " << center.z << ") are applied to the object at bake time";
	
	initTexture(normalAlphaTexture, width, depth, levels);
	initTexture(baseColorTexture, width, depth, levels);
	initTexture(metallicRoughnessTexture, width, depth, levels);

	// temp texture for z-buffer
	auto depthTexture = std::make_unique<GlTexture>(GL_TEXTURE_2D_ARRAY);
	depthTexture->setWrapMode(GL_CLAMP_TO_EDGE);
	depthTexture->storage(levels, GL_DEPTH_COMPONENT24, width, width, depth);

	Framebuffer2 fbo;
	fbo.attachDepthTexture(*depthTexture, 0);
	fbo.attachTexture(0, *normalAlphaTexture, 0);
	fbo.attachTexture(1, *baseColorTexture, 0);
	fbo.attachTexture(2, *metallicRoughnessTexture, 0);
	fbo.enableDrawBuffers(3);
	if (!fbo.check()) {
		ERR_LOG << "Incomplete Framebuffer";
	}
	fbo.bind();
	glViewport(0, 0, width, width);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);

	const ShaderProgram & shader = *ShaderPool::GetShader("BakeImpostorAtlas");

	shader.setUniform("uModelMatrix", glm::translate(glm::mat4(1), -center));
	shader.setUniform("uProjectionMatrix", glm::ortho(-scale, scale, -scale , scale, -scale, scale));
	shader.setUniform("uReducedViewCount", viewCount);

	GLuint o = 0;
	int n = static_cast<int>(mesh.materials().size());
	for (int i = 0; i < n; ++i) {
		const StandardMaterial& mat = mesh.materials()[i];
		o = mat.setUniforms(shader, MAKE_STR("uMaterial[" << i << "]."), o);
	}

	shader.use();
	glBindVertexArray(mesh.vao());
	glDrawArraysInstanced(GL_TRIANGLES, 0, mesh.pointCount(), angularDefinition);
	glBindVertexArray(0);

	glTextureBarrier();

	normalAlphaTexture->generateMipmap();
	baseColorTexture->generateMipmap();
	metallicRoughnessTexture->generateMipmap();
}
