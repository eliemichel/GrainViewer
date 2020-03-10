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

// Test hierarchcal Z buffer
bool testHzb();

int main(int argc, char** argv) {
	bool success = true;
	success = success && testHzb();
	return EXIT_SUCCESS;
}

bool testHzb()
{
	Window window(512, 512, "SandViewer Tests - HZB");
	
	// Loading
	/*
	loadGeometry();
	loadOcclusionProxy();
	*/

	Framebuffer fbo(512, 512, { {GL_RGBA32F, GL_COLOR_ATTACHMENT0} });
	Framebuffer hierarchicalDepthBuffer(512, 512, { {GL_RGBA32F, GL_COLOR_ATTACHMENT0} }, true /* mipmapDepthBuffer */);

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

	scene->setResolution(512, 512);
	scene->update(0);
	glViewport(0, 0, 1920, 1080);

	// Render pipeline
	//renderHzb();

	hierarchicalDepthBuffer.bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

	window.swapBuffers();

	return true;
}
