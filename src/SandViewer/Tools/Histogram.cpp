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

bool mainGui(const std::string & filename);

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

	return mainGui(filename) ? EXIT_SUCCESS : EXIT_FAILURE;
}

void drawTextureFit(const GlTexture& texture, float x, float y, float width, float height)
{
	float w = static_cast<float>(texture.width());
	float h = static_cast<float>(texture.height());
	float ratio = h / w;
	float scale = std::min(height / (ratio * w), width / w);

	QuadMeshData quad;
	quad.start();
	const ShaderProgram& shader = *ShaderPool::GetShader("Quad");
	shader.setUniform("uResolution", glm::vec2(800, 600));
	shader.setUniform("uPosition", glm::vec2(x, y));
	shader.setUniform("uSize", glm::vec2(w * scale, w * ratio * scale));
	texture.bind(0);
	shader.setUniform("uTexture", 0);
	shader.use();
	quad.draw();
	return;

	ImGui::SetCursorPos(ImVec2(x, y));
	ImGui::Image((void*)(intptr_t)texture.raw(), ImVec2(w * scale, w * ratio * scale));
}

void computeHistogram(const GlTexture & image, const GlTexture & histogram)
{
	const ShaderProgram& shader = *ShaderPool::GetShader("Histogram");
	image.bind(0);
	histogram.bind(1);
	shader.use();
	glDispatchCompute(image.width(), image.height(), 1);
	glTextureBarrier();
}

void drawHistogram(const GlTexture & histogram, float x, float y, float w, float h)
{
	QuadMeshData quad;
	quad.start();
	const ShaderProgram & shader = *ShaderPool::GetShader("Quad");
	shader.setUniform("uResolution", glm::vec2(800, 600));
	shader.setUniform("uPosition", glm::vec2(x, y));
	shader.setUniform("uSize", glm::vec2(w, h));
	histogram.bind(0);
	shader.setUniform("uTexture", 0);
	shader.use();
	quad.draw();
}

bool mainGui(const std::string & filename)
{
	LOG << "Openning OpenGL context";
	Window window(800, 600, "Histogram Tools");
	if (!window.isValid()) return false;

	// Load shaders
	ShaderPool::AddShader("Quad", "quad");
	ShaderPool::AddShader("Histogram", "histogram", ShaderProgram::ComputeShader);

	// Load textures
	rapidjson::Document document;
	if (!openJson(filename, document)) return false;
	const rapidjson::Value & json = document["augen"]["histogram"];

	std::string textureFilename;
	jrOption(json, "inputTexture1", textureFilename);
	auto inputTexture1 = ResourceManager::loadTexture(textureFilename);
	jrOption(json, "inputTexture2", textureFilename);
	auto inputTexture2 = ResourceManager::loadTexture(textureFilename);

	auto histogram1 = GlTexture(GL_TEXTURE_2D);
	histogram1.storage(1, GL_R32I, 256, 3);

	computeHistogram(*inputTexture1, histogram1);
	drawHistogram(histogram1, 0, 0, 400, 300);

	// ImGui init
	Gui::Init(window);
	Gui::NewFrame();
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	//ImGui::SetNextWindowSize(ImVec2(800, 300));

	// Draw textures
	ImGui::Begin("Input Textures");
	ImVec2 s = ImGui::GetWindowSize();
	drawTextureFit(*inputTexture1, 0, 0, s.x/2, s.y);
	drawTextureFit(*inputTexture2, s.x/2, 0, s.x/2, s.y);
	ImGui::End();

	// Draw imgui
	//Gui::DrawFrame();

	window.swapBuffers();
	while (!window.shouldClose()) window.pollEvents();
	Gui::Shutdown();
	return true;
}
