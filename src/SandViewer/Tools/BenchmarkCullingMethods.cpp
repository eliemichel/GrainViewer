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
#include "Culling/GlobalAtomicElementBufferBuilder.h"

bool doBenchmark(const Settings & settings, std::shared_ptr<AbstractElementBufferBuilder> builder);

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

	bool success = true;

	Settings settings;
	settings.pointCount = 1000000;
	settings.benchmarkRepeat = 1000;
	settings.testResult = false;

	LOG << " -- Using GlobalAtomicElementBufferBuilder on a buffer of " << settings.pointCount << " elements.";
	success = success && doBenchmark(settings, std::make_shared<GlobalAtomicElementBufferBuilder>());

	LOG << " -- Using PrefixSumElementBufferBuilder on a buffer of " << settings.pointCount << " elements.";
	success = success && doBenchmark(settings, std::make_shared<PrefixSumElementBufferBuilder>());

	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

bool doBenchmark(const Settings & settings, std::shared_ptr<AbstractElementBufferBuilder> builder)
{
	LOG << "Openning OpenGL context";
	Window window(128, 128, "BenchmarkCullingMethods");
	if (!window.isValid()) return false;

	if (!builder->init(settings)) return false;

	LOG << "Building element buffer " << settings.benchmarkRepeat << " times...";
	auto startTime = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < settings.benchmarkRepeat; ++i) {
		builder->build();
	}
	auto endTime = std::chrono::high_resolution_clock::now();
	double ellapsed = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(endTime - startTime).count();
	LOG << "Ellapsed time: " << ellapsed << " seconds.";

	bool success = true;
	if (settings.testResult) {
		LOG << "Testing output...";
		bool success = builder->check();
		LOG << (success ? "Success." : "Failure.");
	}

	glfwSwapBuffers(window.glfw()); // To help debugging in RenderDoc

	return true;
}
