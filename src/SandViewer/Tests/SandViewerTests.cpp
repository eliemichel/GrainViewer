#include <cstdlib>
#include <memory>

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

#define W 1920
#define H 1080

// Test hierarchcal Z buffer
bool testHzb();

// Utils
/**
 * camera is the camera that has been used to render the depth buffer
 */
void saveDepthBufferMipMaps(const std::string & prefix, GLuint tex, const Camera & camera);

int main(int argc, char** argv) {
	bool success = true;
	success = success && testHzb();
	return EXIT_SUCCESS;
}

bool testHzb()
{
	Window window(W, H, "SandViewer Tests - HZB");
	
	//////////////////////////////////////////////////////////////
	// Loading
	/*
	loadGeometry();
	loadOcclusionProxy();
	*/

	Framebuffer fbo(W, H, { {GL_RGBA32F, GL_COLOR_ATTACHMENT0} });
	Framebuffer hierarchicalDepthBuffer(W, H, { {GL_RGBA32F, GL_COLOR_ATTACHMENT0} }, true /* mipmapDepthBuffer */);

	auto scene = std::make_shared<Scene>();
	if (!scene->load(SHARE_DIR "/test/hzb.json")) return false;

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
	occlusionGeometry->render(*scene->viewportCamera(), scene->world(), DirectRendering);

	Filtering::MipmapDepthBuffer(hierarchicalDepthBuffer);

	//  B. Render scene
	fbo.bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	for (const auto & obj : scene->objects()) {
		if (obj == occlusionGeometry) continue;
		// TODO: test against HZB
		if (auto meshData = obj->getBehavior<MeshDataBehavior>().lock()) {
			glm::vec3 c = meshData->boundingSphereCenter();
			float r = meshData->boundingSphereRadius();
			DEBUG_LOG << "Bounding Sphere: c = (" << c.x << ", " << c.y << ", " << c.z << "), r = " << r;
			float ssRadius = camera.projectSphere(c, r);
			DEBUG_LOG << "(maps to a circle of radius " << ssRadius << " in screen space)";
		}
		obj->render(camera, scene->world(), DirectRendering);
	}

	//////////////////////////////////////////////////////////////
	// Save results to files

	ResourceManager::saveTexture_libpng("hzb.png", hierarchicalDepthBuffer.colorTexture(0));
	ResourceManager::saveTexture_libpng("test.png", fbo.colorTexture(0));
	saveDepthBufferMipMaps("hzb_depth_mip", hierarchicalDepthBuffer.depthTexture(), camera);

	// Save debug
	ShaderPool::AddShader("Debug", "debug", ShaderProgram::RenderShader);
	GlTexture debugTexture(GL_TEXTURE_2D);
	debugTexture.storage(1, GL_RGBA16F, W, H);
	Filtering::Blit(debugTexture, fbo.colorTexture(0), *ShaderPool::GetShader("Debug"));
	ResourceManager::saveTexture_libpng("debug.png", debugTexture);

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
		ResourceManager::saveTexture_libpng(prefix + std::to_string(level) + ".png", rgb);
	}
}

