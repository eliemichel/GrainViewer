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

// Test hierarchcal Z buffer
bool testHzb();

int main(int argc, char** argv) {
	bool success = true;
	success = success && testHzb();
	return EXIT_SUCCESS;
}

bool testHzb()
{
	size_t W = 1920, H = 1080;
	Window window(W, H, "SandViewer Tests - HZB");
	
	// Loading
	/*
	loadGeometry();
	loadOcclusionProxy();
	*/

	Framebuffer fbo(W, H, { {GL_RGBA32F, GL_COLOR_ATTACHMENT0} });
	Framebuffer hierarchicalDepthBuffer(W, H, { {GL_RGBA32F, GL_COLOR_ATTACHMENT0} }, true /* mipmapDepthBuffer */);

	auto scene = std::make_shared<Scene>();
	if (!scene->load(SHARE_DIR "/test/hzb.json")) {
		return false;
	}

	std::shared_ptr<RuntimeObject> occlusionGeometry;

	for (auto & obj : scene->objects()) {
		if (obj->name == "OcclusionGeometry") {
			occlusionGeometry = obj;
		}
	}

	if (!occlusionGeometry) {
		ERR_LOG << "Could not find any object called OcclusionGeometry in hzv.json";
		return false;
	}

	scene->setResolution(W, H);
	scene->update(0);
	glViewport(0, 0, W, H);

	// Render pipeline
	//renderHzb();

	hierarchicalDepthBuffer.bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	occlusionGeometry->render(*scene->viewportCamera(), scene->world(), DirectRendering);

	//render();
	fbo.bind();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	for (const auto & obj : scene->objects()) {
		obj->render(*scene->viewportCamera(), scene->world(), DirectRendering);
	}

	// Save framebuffer
	hierarchicalDepthBuffer.saveToPng("hzb.png");
	fbo.saveToPng("test.png");

	Filtering::MipmapDepthBuffer(hierarchicalDepthBuffer);
	hierarchicalDepthBuffer.saveDepthMipMapsToPng("hzb_depth_mip");

	GlTexture rgb(GL_TEXTURE_2D);
	rgb.storage(1, GL_RGBA16F, W, H);
	rgb.generateMipmap();

	GlTexture depthTexture(hierarchicalDepthBuffer.depthTexture(), GL_TEXTURE_2D);

	Filtering::Blit(rgb, depthTexture, *ShaderPool::GetShader("DepthToColorBuffer"));

	depthTexture.release();

	ResourceManager::saveTexture_libpng("rgb.png", rgb);

	window.swapBuffers();

	return true;
}
