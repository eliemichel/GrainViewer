#include <cstdio>
#include <string>
#include <sstream>

#include <glad/modernglad.h>
#include <GLFW/glfw3.h>

#include "Logger.h"
#include "ShaderPool.h"
#include "GlBuffer.h"
#include "Ui/Window.h"

typedef std::shared_ptr<ShaderProgram> ShaderPtr;

struct Settings {
	std::string shaderName = "benchmark-prefixsum-culling";
	int pointCount = 10000;

	// The next three must sum to at most 1
	float impostorProportion = 0.1f;
	float instanceProportion = 0.2f;
	float pointProportion = 0.5f;

	// Must be in sync with shader qualifiers
	int local_size_x = 128;
};

enum RenderModels {
	InstanceModel = 0,
	ImpostorModel,
	PointModel,
	NoModel,
};
enum PrefixSumCullingSteps {
	MarkCulling = 0,
	Group,
	BuildCommandBuffer
};
static const std::vector<std::string> prefixSumCullingSteps = {
	"STEP_MARK_CULLING",
	"STEP_GROUP",
	"STEP_BUILD_COMMAND_BUFFER",
};

struct PrefixSumInfoSsbo {
	GLuint instanceCount;
	GLuint impostorCount;
	// Keep culling flag for last elements, because the prefix sum discards them
	GLuint isLastPointInstance;
	GLuint isLastPointImpostor;
};

std::vector<ShaderPtr> loadCullingShaders(const std::string & baseFile);

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

	Settings settings;

	LOG << "Openning OpenGL context";
	auto window = std::make_shared<Window>(128, 128, "BenchmarkCullingMethods");
	if (!window->isValid()) return EXIT_FAILURE;

	LOG << "Loading shaders...";
	std::vector<ShaderPtr> cullingShaders = loadCullingShaders(settings.shaderName);
	if (cullingShaders.empty()) return EXIT_FAILURE;

	LOG << "Generating reference buffers...";
	GlBuffer renderTypeSsbo(GL_SHADER_STORAGE_BUFFER);
	renderTypeSsbo.addBlock<GLuint>(settings.pointCount);
	renderTypeSsbo.alloc();
	renderTypeSsbo.fillBlock<GLuint>(0, [&settings](GLuint *data, size_t size) {
		for (int i = 0; i < size; +i) {
			// TODO: pick randomly
			data[i] = static_cast<GLuint>(ImpostorModel);
			data[i] = static_cast<GLuint>(InstanceModel);
			data[i] = static_cast<GLuint>(PointModel);
			data[i] = static_cast<GLuint>(NoModel);
		}
	});

	GlBuffer prefixSumInfoSsbo(GL_SHADER_STORAGE_BUFFER);
	prefixSumInfoSsbo.addBlock<PrefixSumInfoSsbo>();
	prefixSumInfoSsbo.alloc();

	GlBuffer elementBuffers[2]{ GL_SHADER_STORAGE_BUFFER , GL_SHADER_STORAGE_BUFFER };

	// 1. Mark instances
	{
		const ShaderProgram & shader = *cullingShaders[MarkCulling];
		shader.use();
		shader.setUniform("uType", static_cast<GLuint>(0));
		shader.setUniform("uPointCount", settings.pointCount);

		prefixSumInfoSsbo.bindSsbo(0);
		renderTypeSsbo.bindSsbo(1);
		elementBuffers[1].bindSsbo(2);
		int xWorkGroups = (settings.pointCount + (settings.local_size_x - 1)) / settings.local_size_x;
		glDispatchCompute(static_cast<GLuint>(xWorkGroups), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	glfwSwapBuffers(window->glfw());

	return EXIT_SUCCESS;
}

std::vector<ShaderPtr> loadCullingShaders(const std::string & baseFile)
{
	std::string baseName = "Culling";
	ShaderPool::AddShader(baseName, baseFile, ShaderProgram::ComputeShader, {});

	std::vector<ShaderPtr> shaders;
	for (const auto & step : prefixSumCullingSteps) {
		std::string name = baseName + "_STEP_" + step;
		ShaderPool::AddShaderVariant(name, baseName, step);
		shaders.push_back(ShaderPool::GetShader(name));
		if (!shaders.back()->isValid()) {
			ERR_LOG << "Invalid shader";
			return {};
		}
	}

	return shaders;
}
