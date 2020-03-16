#include <vector>
#include <glm/glm.hpp>
#include "ShaderPool.h"
#include "Ui/Window.h"
#include "GlTexture.h"
#include "Filtering.h"
#include "ResourceManager.h"
#include "testSplineBlur.h"

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

