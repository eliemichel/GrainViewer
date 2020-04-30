#include <OpenGL>

#include "Logger.h"
#include "ShaderPool.h"
#include "ResourceManager.h"
#include "GlTexture.h"
#include "Ui/Window.h"
#include "Ui/Gui.h"
#include "utils/jsonutils.h"
#include "utils/mathutils.h"
#include "utils/behaviorutils.h"
#include "utils/ReflectionAttributes.h"
#include "Behavior/QuadMeshData.h"
#include "Ui/UiPainter.h"

#include <refl.hpp>
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

std::unique_ptr<GlTexture> computeHistogram(const GlTexture& image)
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

std::unique_ptr<GlTexture> makeGaussianCdf(float sigma)
{
	// Create texture
	auto cdf = std::make_unique<GlTexture>(GL_TEXTURE_2D);
	cdf->storage(1, GL_R32I, 256, 4);
	glTextureParameteri(cdf->raw(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(cdf->raw(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	cdf->generateMipmap();

	int count = cdf->width() * cdf->height();
	std::vector<GLint> data(count);

	float N = 1.0f / (2 * sqrt(2) * sigma);
	for (size_t k = 0; k < 4; ++k) {
		GLint* d = data.data() + k * 256;
		for (size_t i = 255; i > 0; --i) {
			float x = static_cast<float>(i) / 255;
			d[i] = static_cast<int>(1e6f * 0.5f * (1 + djerf((2 * x - 1) * N)));
		}
	}

	cdf->subImage(0, 0, 0, cdf->width(), cdf->height(), GL_RED_INTEGER, GL_INT, data.data());
	return cdf;
}

std::unique_ptr<GlTexture> makeGaussianInverseCdf(float sigma)
{
	// Create texture
	auto cdf = std::make_unique<GlTexture>(GL_TEXTURE_2D);
	cdf->storage(1, GL_R32I, 256, 4);
	glTextureParameteri(cdf->raw(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(cdf->raw(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	cdf->generateMipmap();

	int count = cdf->width() * cdf->height();
	std::vector<GLint> data(count);

	float N = sqrt(2) * sigma;
	for (size_t k = 0; k < 4; ++k) {
		GLint* d = data.data() + k * 256;
		for (size_t i = 255; i > 0; --i) {
			float x = static_cast<float>(i) / 255;
			d[i] = static_cast<int>(1e6f * (0.5f + N * djerfinv(2 * x - 1)));
		}
		d[255] = static_cast<int>(1e6f);
	}

	cdf->subImage(0, 0, 0, cdf->width(), cdf->height(), GL_RED_INTEGER, GL_INT, data.data());
	return cdf;
}

std::unique_ptr<GlTexture> decumulate(const GlTexture& cdf)
{
	// Create texture
	auto histogram = std::make_unique<GlTexture>(GL_TEXTURE_2D);
	histogram->storage(1, GL_R32I, 256, 4);
	glTextureParameteri(histogram->raw(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(histogram->raw(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	histogram->generateMipmap();

	// TODO: compute on GPU
	int count = cdf.width() * cdf.height();
	std::vector<GLint> data(count);
	glGetTextureImage(cdf.raw(), 0, GL_RED_INTEGER, GL_INT, count * sizeof(GLint), data.data());

	for (size_t k = 0; k < 4; ++k) {
		GLint* d = data.data() + k * 256;
		for (size_t i = 255; i > 0; --i) {
			d[i] -= d[i - 1];
		}
	}

	histogram->subImage(0, 0, 0, histogram->width(), histogram->height(), GL_RED_INTEGER, GL_INT, data.data());

	return histogram;
}

/**
 * Box filter on CDF
 */
void regularizeCdf(GlTexture& cdf, int window)
{
	// TODO: compute on GPU
	int count = cdf.width() * cdf.height();
	std::vector<GLint> data(count);
	glGetTextureImage(cdf.raw(), 0, GL_RED_INTEGER, GL_INT, count * sizeof(GLint), data.data());

	int hwindow = window / 2;

	for (size_t k = 0; k < 4; ++k) {
		GLint* d = data.data() + k * 256;
		for (size_t i = 0; i < 256; ++i) {
			GLint sum = 0;
			GLint weight = 0;
			for (int j = -hwindow; j <= hwindow; ++j) {
				if (i + j < 0 || i + j >= 256) continue;
				sum += d[i+j];
				++weight;
			}
			d[i] = sum / weight;
		}
	}

	cdf.subImage(0, 0, 0, cdf.width(), cdf.height(), GL_RED_INTEGER, GL_INT, data.data());
}

bool histogramEq(const GlTexture& hist1, const GlTexture& hist2)
{
	if (hist1.width() != hist2.width()) {
		DEBUG_LOG << "different width";
		return false;
	}
	if (hist1.height() != hist2.height()) {
		DEBUG_LOG << "different height";
		return false;
	}
	int count = hist1.width() * hist1.height();
	std::vector<GLint> data1(count);
	std::vector<GLint> data2(count);
	glGetTextureImage(hist1.raw(), 0, GL_RED_INTEGER, GL_INT, count * sizeof(GLint), data1.data());
	glGetTextureImage(hist2.raw(), 0, GL_RED_INTEGER, GL_INT, count * sizeof(GLint), data2.data());

	for (size_t k = 0; k < 4; ++k) {
		GLint* d1 = data1.data() + k * 256;
		GLint* d2 = data2.data() + k * 256;
		for (size_t i = 1; i < 256; ++i) {
			if (d1[i] != d2[i]) {
				DEBUG_LOG << "Different value at (" << i << ", " << k << "): " << d1[i] << " != " << d2[i];
				return false;
			}
		}
	}

	return true;
}

int histogramMax(const GlTexture& histogram)
{
	int count = histogram.width() * histogram.height();
	std::vector<GLint> data(count);
	glGetTextureImage(histogram.raw(), 0, GL_RED_INTEGER, GL_INT, count * sizeof(GLint), data.data());

	int maxi = 0;
	for (int i = 0; i < count * 3 / 4; ++i) { // 3/4 because we ignore alpha channels for this
		maxi = std::max(maxi, data[i]);
	}
	return maxi;
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
		GLint* d = data.data() + k * 256;
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

std::unique_ptr<GlTexture> tileTexture(const GlTexture& input, float tileSize)
{
	auto output = std::make_unique<GlTexture>(GL_TEXTURE_2D);
	output->storage(input.levels(), GL_RGBA8, input.width(), input.height());

	// Compute histogram
	const ShaderProgram& shader = *ShaderPool::GetShader("TileTexture");
	input.bind(0);
	shader.setUniform("uInputTexture", 0);
	shader.setUniform("uTileSize", tileSize);
	glBindImageTexture(0, output->raw(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
	shader.use();
	glDispatchCompute(input.width(), input.height(), 1);
	glTextureBarrier();

	output->generateMipmap();
	return output;
}

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
	UiPainter::DrawQuad();
}

void drawHistogram(const GlTexture& histogram, float x, float y, float w, float h)
{
	const ShaderProgram& shader = *ShaderPool::GetShader("HistogramQuad");
	shader.setUniform("uNormalization", 1.0f / static_cast<float>(histogramMax(histogram)));
	shader.setUniform("uPosition", glm::vec2(x, y));
	shader.setUniform("uSize", glm::vec2(w, h));
	histogram.bind(0);
	shader.setUniform("uHistogram", 0);
	shader.use();
	UiPainter::DrawQuad();
}

bool test()
{
	std::unique_ptr<GlTexture> input;
	auto histogram = computeHistogram(*input);
	auto cdf = accumulate(*histogram);
	auto histogramb = decumulate(*cdf);
	if (!histogramEq(*histogram, *histogramb)) {
		ERR_LOG << "Test NOT passed: histogramEq";
		return false;
	}
	if (!histogramMax(*cdf) == input->width() * input->height()) {
		ERR_LOG << "Test NOT passed: histogramMax";
		return false;
	}
	return true;
}

class HistogramTool
{
	using TexturePtr = std::unique_ptr<GlTexture>;
private:
	TexturePtr inputTexture1;
	TexturePtr inputTexture2;

public:
	// behavior
	bool deserialize(const rapidjson::Value& json)
	{
		std::string textureFilename;
		jrOption(json, "inputTexture1", textureFilename);
		inputTexture1 = ResourceManager::loadTexture(textureFilename);
		jrOption(json, "inputTexture2", textureFilename);
		//inputTexture2 = ResourceManager::loadTexture(textureFilename);
		autoDeserialize(json, props);
		return true;
	}

	TexturePtr histogram1, histogram2;
	TexturePtr cdf1, invCdf1, gCdf, invgCdf;
	TexturePtr normalizedTexture1, normalizedTiled1, tiled1;
	TexturePtr histogramNormalizedTexture1, histogramNormalizedTiled1, histogramTiled1;
	TexturePtr cdfNormalizedTexture1;

	struct Properties {
		float tileSize = 50;
		bool regularizeCdf = false;
		int regularizeWindow = 3;
		PROPERTIES_OPERATORS_DECL
	};
	Properties props;
	bool mustUpdate = true;

	void updateTextures()
	{
		if (!mustUpdate) return;
		mustUpdate = false;

		histogram1 = computeHistogram(*inputTexture1);
		//histogram2 = computeHistogram(*inputTexture2);

		cdf1 = accumulate(*histogram1);
		//cdf2 = accumulate(*histogram2);
		invCdf1 = inverseCdf(*cdf1);
		//invCdf2 = inverseCdf(*cdf2);
		gCdf = makeGaussianCdf(1.f / 6.f);
		invgCdf = makeGaussianInverseCdf(1.f / 6.f);

		//auto outputTexture1 = applyCdf(*applyCdf(*inputTexture1, *cdf1), *invgCdf);
		//auto histogramOutput1 = computeHistogram(*outputTexture1);
		//auto cdfOuput1 = accumulate(*histogramOutput1);
		//auto histogramOuput1 = decumulate(*cdfOuput1);

		//auto normalizedTexture1 = applyCdf(*applyCdf(*inputTexture1, *cdf1), *invgCdf);
		normalizedTexture1 = applyCdf(*inputTexture1, *cdf1);
		normalizedTexture1->setWrapMode(GL_REPEAT);
		normalizedTiled1 = tileTexture(*normalizedTexture1, props.tileSize);
		tiled1 = applyCdf(*applyCdf(*normalizedTiled1, *gCdf), *invCdf1);

		histogramNormalizedTexture1 = computeHistogram(*normalizedTexture1);
		histogramNormalizedTiled1 = computeHistogram(*normalizedTiled1);
		histogramTiled1 = computeHistogram(*tiled1);

		cdfNormalizedTexture1 = accumulate(*histogramNormalizedTexture1);
		if (props.regularizeCdf) {
			regularizeCdf(*cdfNormalizedTexture1, props.regularizeWindow);
			histogramNormalizedTexture1 = decumulate(*cdfNormalizedTexture1);
		}
	}

	void draw(glm::vec2 res) const
	{
		ShaderPool::GetShader("Quad")->setUniform("uResolution", res);
		ShaderPool::GetShader("HistogramQuad")->setUniform("uResolution", res);

		glm::vec2 step = res / glm::vec2(4, 5);

		// Draw textures
		drawTextureFit(*inputTexture1, 0 * step.x, 0 * step.y, 1 * step.x, 3 * step.y);
		drawHistogram(*histogram1, 0 * step.x, 3 * step.y, 1 * step.x, 2 * step.y);

		drawTextureFit(*normalizedTexture1, 1 * step.x, 0 * step.y, 1 * step.x, 3 * step.y);
		drawHistogram(*cdfNormalizedTexture1, 1 * step.x, 3 * step.y, 1 * step.x, 2 * step.y);

		drawTextureFit(*normalizedTiled1, 2 * step.x, 0 * step.y, 1 * step.x, 3 * step.y);
		drawHistogram(*histogramNormalizedTexture1, 2 * step.x, 3 * step.y, 1 * step.x, 2 * step.y);

		drawTextureFit(*tiled1, 3 * step.x, 0 * step.y, 1 * step.x, 3 * step.y);
		drawHistogram(*histogramTiled1, 3 * step.x, 3 * step.y, 1 * step.x, 2 * step.y);
	}

	void drawGui(glm::vec2 res)
	{
		Properties before = props;
		autoUi(props);
		mustUpdate = props != before;
	}
};

#define _ ReflectionAttributes::
REFL_TYPE(HistogramTool::Properties)
REFL_FIELD(tileSize, _ Range(8, 256))
REFL_FIELD(regularizeCdf)
REFL_FIELD(regularizeWindow, _ Range(1, 15))
REFL_END
PROPERTIES_OPERATORS_DEF(HistogramTool)
#undef _

#include <GLFW/glfw3.h>
class HistogramToolGui : public Gui
{
public:
	HistogramToolGui(std::shared_ptr<Window> window) : Gui(window) {}
	//void onResize(int width, int height) override {}

	void onMouseButton(int button, int action, int mods) override {}

	void onKey(int key, int scancode, int action, int mods) override
	{
		if (action == GLFW_PRESS) {
			switch (key) {
			case GLFW_KEY_ESCAPE:
				if (auto window = getWindow().lock()) {
					glfwSetWindowShouldClose(window->glfw(), GL_TRUE);
				}
				break;
			}
		}
	}

	void onCursorPosition(double x, double y) override {}
};

bool mainGui(const rapidjson::Value& json)
{
	LOG << "Openning OpenGL context";
	auto window = std::make_shared<Window>(1600, 700, "Histogram Tools");
	if (!window->isValid()) return false;

	HistogramTool histogramTool;

	if (json.HasMember("shaders")) ShaderPool::Deserialize(json["shaders"]);
	if (json.HasMember("histogram")) histogramTool.deserialize(json["histogram"]);

	HistogramToolGui gui(window);
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(200, 200));
	
	while (!window->shouldClose()) {
		histogramTool.updateTextures();

		glm::vec2 res(gui.width(), gui.height());

		histogramTool.draw(res);

		Gui::NewFrame();
		ImGui::Begin("Parameters");
		histogramTool.drawGui(res);
		ImGui::End();
		Gui::DrawFrame();

		window->swapBuffers();
		window->pollEvents();
	}

	return true;
}
