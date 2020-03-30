#include "Logger.h"
#include "Ui/Window.h"
#include "Framebuffer.h"
#include "ResourceManager.h"
#include "RuntimeObject.h"
#include "Scene.h"
#include "GlTexture.h"
#include "Filtering.h"
#include "ShaderPool.h"
#include "Behavior/MeshDataBehavior.h"
#include "testHzb.h"

#define W 1920
#define H 1080

/**
 * camera is the camera that has been used to render the depth buffer
 */
void saveDepthBufferMipMaps(const std::string & prefix, GLuint tex, const Camera & camera);

bool testHzb()
{
	Window window(W, H, "SandViewer Tests - HZB");
	ShaderPool::AddShader("ZBufferIdentity", "zbuffer-identity", ShaderProgram::RenderShader);
	ShaderPool::AddShader("Debug", "debug", ShaderProgram::RenderShader);

	//////////////////////////////////////////////////////////////
	// Loading

	Framebuffer fbo(W, H, { {GL_RGBA32F, GL_COLOR_ATTACHMENT0} });
	Framebuffer hierarchicalDepthBuffer(W, H, { {GL_RGBA32F, GL_COLOR_ATTACHMENT0} }, true /* mipmapDepthBuffer */);

	auto scene = std::make_shared<Scene>();
	if (!scene->load(SHARE_DIR "/test/hzb.json")) return false;

	window.swapBuffers();

	auto occlusionGeometry = scene->findObjectByName("OcclusionGeometry");

	if (!occlusionGeometry) {
		ERR_LOG << "Could not find any object called OcclusionGeometry in hzv.json";
		return false;
	}

	Camera & camera = *scene->viewportCamera();

	scene->setResolution(W, H);
	scene->update(0);
	glViewport(0, 0, W, H);

	//////////////////////////////////////////////////////////////
	// Render pipeline
	//  A. Render HBZ
	hierarchicalDepthBuffer.bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	occlusionGeometry->render(*scene->viewportCamera(), *scene->world(), RenderType::Direct);

	Filtering::MipmapDepthBuffer(hierarchicalDepthBuffer);

	//  B. Render scene
	// get hzb pyramid on CPU
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	std::vector<std::vector<GLfloat>> zbuffers;
	std::vector<GLint> zbuffersWidth;
	std::vector<GLint> zbuffersHeight;
	GLint baseLevel, maxLevel;
	glGetTextureParameteriv(hierarchicalDepthBuffer.depthTexture(), GL_TEXTURE_BASE_LEVEL, &baseLevel);
	glGetTextureParameteriv(hierarchicalDepthBuffer.depthTexture(), GL_TEXTURE_MAX_LEVEL, &maxLevel);
	for (GLint level = baseLevel; level < maxLevel; ++level) {
		GLint w, h;
		glGetTextureLevelParameteriv(hierarchicalDepthBuffer.depthTexture(), level, GL_TEXTURE_WIDTH, &w);
		glGetTextureLevelParameteriv(hierarchicalDepthBuffer.depthTexture(), level, GL_TEXTURE_HEIGHT, &h);

		if (w == 0 || h == 0) break;

		zbuffers.push_back(std::vector<GLfloat>(0));
		zbuffersWidth.push_back(w);
		zbuffersHeight.push_back(h);
		GLsizei byteCount = w * h * sizeof(GLfloat);
		zbuffers.back().resize(w * h);
		glGetTextureSubImage(hierarchicalDepthBuffer.depthTexture(), level, 0, 0, 0, w, h, 1, GL_DEPTH_COMPONENT, GL_FLOAT, byteCount, zbuffers.back().data());
	}

	fbo.bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, W, H);
	glEnable(GL_DEPTH_TEST);
	float debugRadius;
	glm::vec3 debugCenter;
	for (const auto & obj : scene->objects()) {
		if (obj == occlusionGeometry) continue;
		
		bool culling = false;
		if (auto meshData = obj->getBehavior<MeshDataBehavior>().lock()) {
			if (meshData->hasBoundingSphere()) {
				// Test against HZB
				glm::vec3 c = meshData->boundingSphereCenter();
				float r = meshData->boundingSphereRadius();
				DEBUG_LOG << "Bounding Sphere: c = (" << c.x << ", " << c.y << ", " << c.z << "), r = " << r;
				glm::vec3 ssCircle = camera.projectSphere(c, r);
				DEBUG_LOG << "(maps to a circle of radius " << ssCircle.z << " at position (" << ssCircle.x << ", " << ssCircle.y << ") in screen space)";
				int mipmapLevel = std::min(static_cast<int>(ceil(log2(ssCircle.z) + 1)), static_cast<int>(zbuffers.size()) - 1);
				float fpixelx = ssCircle.x / static_cast<float>(pow(2, mipmapLevel));
				float fpixely = ssCircle.y / static_cast<float>(pow(2, mipmapLevel));
				float fractx = fpixelx - floor(fpixelx);
				float fracty = fpixely - floor(fpixely);
				int pixelx = static_cast<int>(floor(fpixelx));
				int pixelx2 = pixelx + (fractx > 0.5 ? 1 : -1);
				int pixely = static_cast<int>(floor(fpixely));
				int pixely2 = pixely + (fracty > 0.5 ? 1 : -1);
				DEBUG_LOG << "Querying level " << mipmapLevel << " at point (" << pixelx << "/" << pixelx2 << ", " << pixely << "/" << pixely2 << ")";
				debugRadius = r;
				debugCenter = c;

				int bw = zbuffersWidth[mipmapLevel];
				int bh = zbuffersHeight[mipmapLevel];
				GLfloat z1 = camera.linearDepth(zbuffers[mipmapLevel][bw * (bh - pixely - 1) + pixelx]);
				GLfloat z2 = camera.linearDepth(zbuffers[mipmapLevel][bw * (bh - pixely - 1) + pixelx2]);
				GLfloat z3 = camera.linearDepth(zbuffers[mipmapLevel][bw * (bh - pixely2 - 1) + pixelx]);
				GLfloat z4 = camera.linearDepth(zbuffers[mipmapLevel][bw * (bh - pixely2 - 1) + pixelx2]);

				GLfloat minDistToBSphere = glm::length(glm::vec3(camera.viewMatrix() * glm::vec4(c, 1.0f))) - r;

				DEBUG_LOG << "z1: " << z1 << ", z2: " << z2 << ", z3: " << z3 << ", z4: " << z4 << ", distance to sphere: " << minDistToBSphere;

				if (z1 < minDistToBSphere && z2 < minDistToBSphere && z3 < minDistToBSphere && z4 < minDistToBSphere) {
					DEBUG_LOG << "culling out!";
					culling = true;
				}
			}
		}
		if (!culling) {
			obj->render(camera, *scene->world(), RenderType::Direct);
		}
	}

	//////////////////////////////////////////////////////////////
	// Save results to files

	ResourceManager::saveTexture_libpng("hzb.png", hierarchicalDepthBuffer.colorTexture(0), 0, true /* vflip */);
	ResourceManager::saveTexture_libpng("test.png", fbo.colorTexture(0), 0, true /* vflip */);
	saveDepthBufferMipMaps("hzb_depth_mip", hierarchicalDepthBuffer.depthTexture(), camera);

	// Save debug
	GlTexture debugTexture(GL_TEXTURE_2D);
	debugTexture.storage(1, GL_RGBA16F, W, H);

	auto debugShader = ShaderPool::GetShader("Debug");
	debugShader->setUniform("uRadius", debugRadius);
	debugShader->setUniform("uCenter", debugCenter);
	debugShader->setUniform("uFocalLength", camera.focalLength());
	debugShader->setUniform("uViewMatrix", camera.viewMatrix());
	debugShader->setUniform("uProjectionMatrix", camera.projectionMatrix());
	debugShader->setUniform("uResolution", glm::vec2(W, H));
	Filtering::Blit(debugTexture, fbo.colorTexture(0), *debugShader);
	ResourceManager::saveTexture_libpng("debug.png", debugTexture, 0, true /* vflip */);

	window.swapBuffers();

	return true;
}

void saveDepthBufferMipMaps(const std::string & prefix, GLuint tex, const Camera & camera)
{
	//ResourceManager::saveTextureMipMaps("hzb_depth_mip", hierarchicalDepthBuffer.depthTexture());
	auto convertShader = ShaderPool::GetShader("DepthToColorBuffer");
	convertShader->setUniform("uNear", camera.nearDistance());
	convertShader->setUniform("uFar", camera.farDistance());
	GLint baseLevel, maxLevel;
	glGetTextureParameteriv(tex, GL_TEXTURE_BASE_LEVEL, &baseLevel);
	glGetTextureParameteriv(tex, GL_TEXTURE_MAX_LEVEL, &maxLevel);
	for (GLint level = baseLevel; level < maxLevel; ++level) {
		GLint w, h;
		glGetTextureLevelParameteriv(tex, level, GL_TEXTURE_WIDTH, &w);
		glGetTextureLevelParameteriv(tex, level, GL_TEXTURE_HEIGHT, &h);

		if (w == 0 || h == 0) break;

		GlTexture rgb(GL_TEXTURE_2D);
		rgb.storage(1, GL_RGBA16F, w, h);

		convertShader->setUniform("uMipMapLevel", level);

		Filtering::Blit(rgb, tex, *convertShader);
		ResourceManager::saveTexture_tinyexr(prefix + std::to_string(level) + ".exr", rgb);

		ResourceManager::saveTexture_tinyexr(prefix + std::to_string(level) + "_raw.exr", tex, level);
	}
}

