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
bool testSplineBlur();

// Utils
/**
 * camera is the camera that has been used to render the depth buffer
 */
void saveDepthBufferMipMaps(const std::string & prefix, GLuint tex, const Camera & camera);

int main(int argc, char** argv) {
	bool success = true;
	success = success && testHzb();
	//success = success && testSplineBlur();
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
	occlusionGeometry->render(*scene->viewportCamera(), scene->world(), DirectRendering);

	Filtering::MipmapDepthBuffer(hierarchicalDepthBuffer);

	//  B. Render scene
	fbo.bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, W, H);
	glEnable(GL_DEPTH_TEST);
	float debugRadius;
	glm::vec3 debugCenter;
	for (const auto & obj : scene->objects()) {
		if (obj == occlusionGeometry) continue;
		// TODO: test against HZB
		if (auto meshData = obj->getBehavior<MeshDataBehavior>().lock()) {
			glm::vec3 c = meshData->boundingSphereCenter();
			float r = meshData->boundingSphereRadius();
			DEBUG_LOG << "Bounding Sphere: c = (" << c.x << ", " << c.y << ", " << c.z << "), r = " << r;
			glm::vec3 ssCircle = camera.projectSphere(c, r);
			DEBUG_LOG << "(maps to a circle of radius " << ssCircle.z << " at position (" << ssCircle.x << ", " << ssCircle.y << ") in screen space)";
			debugRadius = r;
			debugCenter = c;
		}
		obj->render(camera, scene->world(), DirectRendering);
	}

	//////////////////////////////////////////////////////////////
	// Save results to files

	ResourceManager::saveTexture_libpng("hzb.png", hierarchicalDepthBuffer.colorTexture(0), 0, true /* vflip */);
	ResourceManager::saveTexture_libpng("test.png", fbo.colorTexture(0), 0, true /* vflip */);
	saveDepthBufferMipMaps("hzb_depth_mip", hierarchicalDepthBuffer.depthTexture(), camera);

	// Save debug
	ShaderPool::AddShader("Debug", "debug", ShaderProgram::RenderShader);
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
		ResourceManager::saveTexture_libpng(prefix + std::to_string(level) + ".png", rgb, 0, true /* vflip */);
	}
}


bool testSplineBlur()
{
	Window window(128, 128, "SandViewer Tests - Misc");
	
	ShaderPool::AddShader("Blur", "splineblur/blur");
	ShaderPool::AddShader("Diffusion", "splineblur/diffusion");
	ShaderPool::AddShader("PostEffect", "splineblur/posteffect");

	auto inputImage = ResourceManager::loadTexture("E:/tmp/input.jpg");
	auto blurShader = ShaderPool::GetShader("Blur");
	auto diffusionShader = ShaderPool::GetShader("Diffusion");
	auto postEffectShader = ShaderPool::GetShader("PostEffect");

	GlTexture texture01(GL_TEXTURE_2D);
	texture01.storage(1, GL_RGBA16F, inputImage->width(), inputImage->height());
	GlTexture texture02(GL_TEXTURE_2D);
	texture02.storage(1, GL_RGBA16F, inputImage->width(), inputImage->height());

	GlTexture spline(GL_TEXTURE_2D);
	spline.storage(1, GL_RGBA16F, inputImage->width(), inputImage->height());

	std::vector<glm::vec4> splineData(inputImage->width() * inputImage->height());
	GLfloat oneOverWidth = static_cast<GLfloat>(1.0f / inputImage->width());
	GLfloat oneOverHeight = static_cast<GLfloat>(1.0f / inputImage->height());
	for (int y = 0; y < inputImage->height(); ++y) {
		for (int x = 0; x < inputImage->width(); ++x) {
			glm::vec4 & c = splineData[inputImage->width() * y + x];
			c.x = glm::abs(x - 1120) < 50 && y < 200 ? static_cast<GLfloat>(x) : 0.0f;
			c.y = glm::abs(x - 1120) < 50 && y < 200 ? static_cast<GLfloat>(y) : 0.0f;
			c.z = glm::abs(x - 1120) < 50 && y < 200 ? 0.0f : 999.9f;
			c.a = 0.0;
		}
	}
	spline.subImage(0, 0, 0, inputImage->width(), inputImage->height(), GL_RGBA, GL_FLOAT, splineData.data());
	splineData.resize(0);

	Filtering::Blit(texture01, *inputImage, *blurShader);

	bool flip = true;
	for (int i = 0; i < 20; ++i) {
		Filtering::Blit(flip ? texture02 : texture01, flip ? texture01 : texture02, *blurShader);
		flip = !flip;
	}

	ResourceManager::saveTexture_libpng("E:/tmp/output.png", flip ? texture01 : texture02);

	Filtering::Blit(texture01, spline, *diffusionShader);
	flip = true;
	for (int i = 0; i < 120; ++i) {
		Filtering::Blit(flip ? texture02 : texture01, flip ? texture01 : texture02, *blurShader);
		flip = !flip;
	}

	postEffectShader->setUniform("uWidth", static_cast<GLfloat>(inputImage->width()));
	postEffectShader->setUniform("uHeight", static_cast<GLfloat>(inputImage->height()));
	Filtering::Blit(flip ? texture02 : texture01, flip ? texture01 : texture02, *postEffectShader);
	flip = !flip;

	ResourceManager::saveTexture_libpng("E:/tmp/output2.png", flip ? texture01 : texture02);

	return true;
}
