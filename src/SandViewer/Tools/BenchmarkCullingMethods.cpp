#include <cstdio>
#include <cstdlib>
#include <string>
#include <sstream>
#include <chrono>

#include <glad/modernglad.h>
#include <GLFW/glfw3.h>

#include "Logger.h"
#include "ShaderPool.h"
#include "GlBuffer.h"
#include "Ui/Window.h"

#include "Culling/AbstractElementBufferBuilder.h"
#include "Culling/PrefixSumElementBufferBuilder.h"

int main(int argc, char **argv)
{
	std::cout << R"(== BenchmarkCullingMethods ==

The culling problem is the following. We have N points identified by their
index from 0 to N-1 to render, but some are:
 (a) rendered using impostors
 (b) rendered using instances
 (c) rendered using points
 (d) not rendered at all (they are out of sight)
We need to build an element buffer (a buffer of indices) where elements
rendered with the same model are contiguous.
	)" << std::endl;

	// Important: for tests, always fix the random generator seed
	std::srand(3615);

	Settings settings;
	settings.pointCount = 1000000;
	LOG << "Generating buffers of " << settings.pointCount << " elements.";

	AbstractElementBufferBuilder *builder = new PrefixSumElementBufferBuilder();

	LOG << "Openning OpenGL context";
	Window window(128, 128, "BenchmarkCullingMethods");
	if (!window.isValid()) return EXIT_FAILURE;

	if (!builder->init(settings)) return EXIT_FAILURE;

	LOG << "Building element buffer 1000 times...";
	auto startTime = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < 1000; ++i) {
		builder->build();
	}
	auto endTime = std::chrono::high_resolution_clock::now();
	double ellapsed = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(endTime - startTime).count();
	LOG << "Ellapsed time: " << ellapsed << " seconds.";

	LOG << "Testing output...";
	bool success = builder->check();
	LOG << (success ? "Success." : "Failure.");

	glfwSwapBuffers(window.glfw()); // To help debugging in RenderDoc

	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
