#include <OpenGL>

#include "Logger.h"
#include "ShaderPool.h"
#include "ResourceManager.h"
#include "GlTexture.h"
#include "Ui/Window.h"
#include "Ui/Gui.h"
#include "utils/jsonutils.h"
#include "Behavior/QuadMeshData.h"

#include <imgui.h>

#include <cstdio>
#include <string>
#include <iostream>
#include <chrono>
#include <algorithm>

extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

bool mainGui(const rapidjson::Value& json);

int main(int argc, char **argv)
{
	std::cout << R"(== Histogram Tools ==

This is a set of tests around texture historgram transfer, equalization, etc.
Based on Burley19 and HeitzNeyret18.
	)" << std::endl;

	std::srand(3615);

	std::string filename = "C:/Elie/Data/Histogram/scene.json";
	if (argc >= 2) {
		filename = std::string(argv[1]);
	}

	rapidjson::Document document;
	if (!openJson(filename, document)) return false;
	const rapidjson::Value& root = document["augen"];

	return mainGui(root) ? EXIT_SUCCESS : EXIT_FAILURE;
}

std::unique_ptr<QuadMeshData> quad;

void drawTextureFit(const GlTexture& texture, float x, float y, float width, float height)
{
	float w = static_cast<float>(texture.width());
	float h = static_cast<float>(texture.height());
	float ratio = h / w;
	float scale = std::min(height / (ratio * w), width / w);

	const ShaderProgram& shader = *ShaderPool::GetShader("Quad");
	shader.setUniform("uPosition", glm::vec2(x, y));
	shader.setUniform("uSize", glm::vec2(w * scale, w * ratio * scale));
	texture.bind(0);
	shader.setUniform("uTexture", 0);
	shader.use();
	quad->draw();
}

std::unique_ptr<GlTexture> computeHistogram(const GlTexture & image)
{
	// Create texture
	auto histogram = std::make_unique<GlTexture>(GL_TEXTURE_2D);
	histogram->storage(1, GL_R32I, 256, 4);
	glTextureParameteri(histogram->raw(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(histogram->raw(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	histogram->generateMipmap();

	// Compute histogram
	const ShaderProgram& shader = *ShaderPool::GetShader("Histogram");
	glBindImageTexture(0, image.raw(), 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
	glBindImageTexture(1, histogram->raw(), 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32I);
	shader.use();
	shader.setUniform("uStep", 0);
	glDispatchCompute(histogram->width(), histogram->height(), 1);
	shader.setUniform("uStep", 1);
	glDispatchCompute(image.width(), image.height(), 1);
	glTextureBarrier();

	return histogram;
}

// Cumulative Distribution Function (prefix-summed histogram)
std::unique_ptr<GlTexture> accumulate(const GlTexture& histogram)
{
	// Create texture
	auto cdf = std::make_unique<GlTexture>(GL_TEXTURE_2D);
	cdf->storage(1, GL_R32I, 256, 4);
	glTextureParameteri(cdf->raw(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(cdf->raw(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	cdf->generateMipmap();

	// TODO: compute on GPU
	int count = histogram.width() * histogram.height();
	std::vector<GLint> data(count);
	glGetTextureImage(histogram.raw(), 0, GL_RED_INTEGER, GL_INT, count * sizeof(GLint), data.data());

	for (size_t i = 1; i < 256; ++i) {
		for (size_t k = 0; k < 4; ++k) {
			data[i + k * 256] += data[(i - 1) + k * 256];
		}
	}

	cdf->subImage(0, 0, 0, cdf->width(), cdf->height(), GL_RED_INTEGER, GL_INT, data.data());

	return cdf;
}

std::unique_ptr<GlTexture> inverseCdf(const GlTexture& cdf)
{
	// Create texture
	auto invcdf = std::make_unique<GlTexture>(GL_TEXTURE_2D);
	invcdf->storage(1, GL_R32I, 256, 4);
	glTextureParameteri(invcdf->raw(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(invcdf->raw(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	invcdf->generateMipmap();



	// TODO: compute on GPU
	int count = cdf.width() * cdf.height();
	std::vector<GLint> data(count);
	std::vector<GLint> invdata(count);
	glGetTextureImage(cdf.raw(), 0, GL_RED_INTEGER, GL_INT, count * sizeof(GLint), data.data());

	for (size_t k = 0; k < 4; ++k) {
		GLint *d = data.data() + k * 256;
		GLint* invd = invdata.data() + k * 256;
		float total = static_cast<float>(d[255]);
		int last_c = -1;
		float x0 = 0;
		for (int i = 0; i < 256; ++i) {
			float x1 = static_cast<float>(d[i]) * 255 / total;
			for (int c = last_c + 1; c + 1 <= floor(x1); ++c) {
				float fc = static_cast<float>(c);
				float fi = static_cast<float>(i);
				invd[c] = static_cast<int>(ceil((fi + (fc - x0 + 1) / (x1 - x0)) * total / 256));
				assert(c == last_c + 1);
				last_c = c;
			}
			x0 = x1;
		}
		for (int c = last_c + 1; c < 256; ++c) {
			invd[c] = d[255];
		}
	}

	invcdf->subImage(0, 0, 0, invcdf->width(), invcdf->height(), GL_RED_INTEGER, GL_INT, invdata.data());

	return invcdf;
}

void drawHistogram(const GlTexture & histogram, const GlTexture& image, float x, float y, float w, float h)
{
	const ShaderProgram& shader = *ShaderPool::GetShader("HistogramQuad");
	shader.setUniform("uNormalization", 1.0f / static_cast<float>(image.width() * image.height()));
	shader.setUniform("uPosition", glm::vec2(x, y));
	shader.setUniform("uSize", glm::vec2(w, h));
	histogram.bind(0);
	shader.setUniform("uHistogram", 0);
	shader.use();
	quad->draw();
}

std::unique_ptr<GlTexture> applyCdf(const GlTexture& input, const GlTexture& cdf)
{
	auto output = std::make_unique<GlTexture>(GL_TEXTURE_2D);
	output->storage(input.levels(), GL_RGBA8, input.width(), input.height());

	// Compute histogram
	const ShaderProgram& shader = *ShaderPool::GetShader("HistogramTransform");
	glBindImageTexture(0, input.raw(), 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
	glBindImageTexture(1, output->raw(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
	glBindImageTexture(2, cdf.raw(), 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32I);
	shader.use();
	glDispatchCompute(input.width(), input.height(), 1);
	glTextureBarrier();

	output->generateMipmap();

	return output;
}

bool mainGui(const rapidjson::Value& json)
{
	LOG << "Openning OpenGL context";
	glm::vec2 res(1200, 700);
	Window window(static_cast<int>(res.x), static_cast<int>(res.y), "Histogram Tools");
	if (!window.isValid()) return false;

	// Load shaders
	if (json.HasMember("shaders")) ShaderPool::Deserialize(json["shaders"]);
	ShaderPool::GetShader("Quad")->setUniform("uResolution", res);
	ShaderPool::GetShader("HistogramQuad")->setUniform("uResolution", res);

	quad = std::make_unique<QuadMeshData>();
	quad->start();

	// Load textures
	const rapidjson::Value& histJson = json["histogram"];
	std::string textureFilename;
	jrOption(histJson, "inputTexture1", textureFilename);
	auto inputTexture1 = ResourceManager::loadTexture(textureFilename);
	jrOption(histJson, "inputTexture2", textureFilename);
	auto inputTexture2 = ResourceManager::loadTexture(textureFilename);

	// Main
	auto histogram1 = computeHistogram(*inputTexture1);
	auto histogram2 = computeHistogram(*inputTexture2);

	auto cdf1 = accumulate(*histogram1);
	auto cdf2 = accumulate(*histogram2);
	auto invCdf1 = inverseCdf(*cdf1);
	auto invCdf2 = inverseCdf(*cdf2);

	auto tmp = applyCdf(*inputTexture1, *cdf1);
	auto outputTexture1 = applyCdf(*tmp, *invCdf2);
	auto histogramOutput1 = computeHistogram(*outputTexture1);
	auto cdfOuput1 = accumulate(*histogramOutput1);
	
	// Draw textures
	drawTextureFit(*inputTexture1, 0, 0, res.x/3, res.y * 0.6f);
	drawHistogram(*cdf1, *inputTexture1, 0, res.y * 0.6f, res.x / 3, res.y * 0.4f);

	drawTextureFit(*inputTexture2, res.x/3, 0, res.x/3, res.y * 0.6f);
	drawHistogram(*cdf2, *inputTexture2, res.x/3, res.y * 0.6f, res.x / 3, res.y * 0.4f);

	drawTextureFit(*outputTexture1, 2*res.x / 3, 0, res.x / 3, res.y * 0.6f);
	drawHistogram(*cdfOuput1, *outputTexture1, 2*res.x / 3, res.y * 0.6f, res.x / 3, res.y * 0.4f);
	
	window.swapBuffers();
	while (!window.shouldClose()) window.pollEvents();
	return true;
}
