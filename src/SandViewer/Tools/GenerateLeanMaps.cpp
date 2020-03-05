#include <cstdio>
#include <string>

#include <png.h>

#include <glad/modernglad.h>
#include <GLFW/glfw3.h>

#include "Logger.h"
#include "ResourceManager.h"
#include "Filtering.h"
#include "Ui/Window.h"

#include "ShaderPool.h"

int main(int argc, char **argv)
{
	if (argc < 2) {
		LOG << "Usage: " << (argc >= 1 ? argv[0] : "GenerateLeanMaps") << " <normal-map.png> [<lean-map-prefix>]";
		return EXIT_FAILURE;
	}

	std::string inputFilename(argv[1]);

	std::string prefix;
	if (argc < 3) {
		prefix = inputFilename.substr(0, inputFilename.size() - 4);
	} else {
		prefix = std::string(argv[2]);
	}
	std::string outputFilename1 = prefix + "_lean1.png";
	std::string outputFilename2 = prefix + "_lean2.png";

	LOG << "== GenerateLeanMaps ==";

	LOG << "Openning OpenGL context";
	auto window = std::make_shared<Window>(128, 128, "GenerateLeanMaps");
	if (!window->isValid()) {
		return EXIT_FAILURE;
	}

	LOG << "Loading normal map from file '" << inputFilename << "'...";
	auto inputTexture = ResourceManager::loadTexture(inputFilename, 1);

	LOG << "Creating LEAN maps...";
	auto leanTextures = Filtering::CreateLeanTexture(*inputTexture);

	LOG << "Saving LEAN map 1 to file '" << outputFilename1 << "'...";
	ResourceManager::saveTexture_libpng(outputFilename1, leanTextures->lean1);

	LOG << "Saving LEAN map 2 to file '" << outputFilename2 << "'...";
	ResourceManager::saveTexture_libpng(outputFilename2, leanTextures->lean2);

	glfwSwapBuffers(window->glfw());

	LOG << "Test again using texture arrays";
	{
		outputFilename1 = prefix + "_lean1_array.png";
		outputFilename2 = prefix + "_lean2_array.png";

		LOG << "Loading normal map from file '" << inputFilename << "' as a texture array...";
		auto inputStack = std::make_unique<GlTexture>(GL_TEXTURE_2D_ARRAY);
		inputStack->storage(1, GL_RGBA8, inputTexture->width(), inputTexture->height(), 1);
		ResourceManager::loadTextureSubData(*inputStack, inputFilename, 0, inputStack->width(), inputStack->height());
		inputStack->generateMipmap();

		LOG << "Creating LEAN maps...";
		auto leanTextures = Filtering::CreateLeanTexture(*inputStack);

		LOG << "Saving LEAN map 1 to file '" << outputFilename1 << "'...";
		ResourceManager::saveTexture_libpng(outputFilename1, leanTextures->lean1);

		LOG << "Saving LEAN map 2 to file '" << outputFilename2 << "'...";
		ResourceManager::saveTexture_libpng(outputFilename2, leanTextures->lean2);
	}
	glfwSwapBuffers(window->glfw());

	return EXIT_SUCCESS;
}
