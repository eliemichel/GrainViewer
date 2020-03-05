#include <cstdio>
#include <cstdlib>
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
	GLuint pointCount = 20;

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
	_RenderModelCount
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

// Used as SSBO
struct PrefixSumInfo {
	GLuint instanceCount;
	GLuint impostorCount;
	// Keep culling flag for last elements, because the prefix sum discards them
	GLuint isLastPointInstance;
	GLuint isLastPointImpostor;
};
struct DrawArraysIndirectCommand {
	GLuint  count;
	GLuint  instanceCount;
	GLuint  first;
	GLuint  baseInstance;
};
struct DrawElementsIndirectCommand {
	GLuint  count;
	GLuint  instanceCount;
	GLuint  firstIndex;
	GLuint  baseVertex;
	GLuint  baseInstance;
};
struct CommandBuffer {
	DrawElementsIndirectCommand impostorCommand;
	DrawArraysIndirectCommand instanceCommand;
};

std::vector<ShaderPtr> loadCullingShaders(const std::string & baseFile);

/**
 * Input buffer is buffer0, output buffer is buffer0 or buffer1, as specified
 * by the returned value.
 */
int prefixSum(const GlBuffer & buffer0, const GlBuffer & buffer1, int elementCount, const ShaderProgram & shader);

/**
 * Return a random number between 0 and upper - 1
 */
int randint(int upper);

void printGLuintBuffer(const GlBuffer & buffer, std::string name);

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
	ShaderPtr prefixSumShader = ShaderPool::GetShader("PrefixSum");

	LOG << "Generating reference buffer...";
	LOG << "(Reference buffer is a buffer with numbers from 0 to 3 telling which model to use. It mocks the actual culling procedures.)";
	std::vector<GLuint> renderTypeData(settings.pointCount);
	std::srand(3615);
	std::ostringstream ss;
	for (GLuint i = 0; i < settings.pointCount; ++i) {
		renderTypeData[i] = static_cast<GLuint>(randint(_RenderModelCount));
		if (i != 0) ss << ", ";
		ss << renderTypeData[i];
	}
	GlBuffer renderTypeSsbo(GL_SHADER_STORAGE_BUFFER);
	renderTypeSsbo.importBlock(renderTypeData);

	LOG << "renderTypeData: [" << ss.str() << "]";

	LOG << "Generating 3 element buffers...";
	LOG << "(One element buffer will be the output, the others are used as intermediary steps for prefix sum)";
	GlBuffer elementBuffers[3]{ GL_SHADER_STORAGE_BUFFER, GL_SHADER_STORAGE_BUFFER, GL_SHADER_STORAGE_BUFFER };
	for (int i = 0; i < 3; ++i) {
		elementBuffers[i].addBlock<GLuint>(settings.pointCount);
		elementBuffers[i].alloc();
	}

	GlBuffer prefixSumInfoSsbo(GL_SHADER_STORAGE_BUFFER);
	prefixSumInfoSsbo.addBlock<PrefixSumInfo>();
	prefixSumInfoSsbo.alloc();

	GlBuffer commandBuffer(GL_DRAW_INDIRECT_BUFFER);
	commandBuffer.addBlock<CommandBuffer>();
	commandBuffer.alloc();

	int xWorkGroups = (settings.pointCount + (settings.local_size_x - 1)) / settings.local_size_x;
	int resultIndex;

	printGLuintBuffer(renderTypeSsbo, "RenderTypeSsbo");

	LOG << "1. Mark instances";
	{
		const ShaderProgram & shader = *cullingShaders[MarkCulling];
		shader.use();
		shader.setUniform("uType", static_cast<GLuint>(InstanceModel));
		shader.setUniform("uPointCount", settings.pointCount);
		prefixSumInfoSsbo.bindSsbo(0);
		renderTypeSsbo.bindSsbo(1);
		elementBuffers[1].bindSsbo(2);
		glDispatchCompute(static_cast<GLuint>(xWorkGroups), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	printGLuintBuffer(elementBuffers[1], "Current Element Buffer 1");

	LOG << "2. Prefix sum instances";
	resultIndex = prefixSum(elementBuffers[1], elementBuffers[2], settings.pointCount, *prefixSumShader);

	printGLuintBuffer(elementBuffers[1 + resultIndex], "Current Element Buffer #summed");

	LOG << "3. Group instances";
	{
		const ShaderProgram & shader = *cullingShaders[Group];
		shader.use();
		shader.setUniform("uType", static_cast<GLuint>(InstanceModel));
		shader.setUniform("uPointCount", settings.pointCount);
		prefixSumInfoSsbo.bindSsbo(0);
		elementBuffers[1 + resultIndex].bindSsbo(1);
		elementBuffers[0].bindSsbo(2);
		glDispatchCompute(static_cast<GLuint>(xWorkGroups), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	printGLuintBuffer(elementBuffers[0], "Current Element Buffer 0");

	LOG << "4. Mark impostors";
	{
		const ShaderProgram & shader = *cullingShaders[MarkCulling];
		shader.use();
		shader.setUniform("uType", static_cast<GLuint>(ImpostorModel));
		shader.setUniform("uPointCount", settings.pointCount);
		prefixSumInfoSsbo.bindSsbo(0);
		renderTypeSsbo.bindSsbo(1);
		elementBuffers[1].bindSsbo(2);
		glDispatchCompute(static_cast<GLuint>(xWorkGroups), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	printGLuintBuffer(elementBuffers[1], "Current Element Buffer 1");

	LOG << "5. Prefix sum impostors";
	resultIndex = prefixSum(elementBuffers[1], elementBuffers[2], settings.pointCount, *prefixSumShader);

	printGLuintBuffer(elementBuffers[1 + resultIndex], "Current Element Buffer #summed");

	LOG << "6. Group impostors";
	{
		const ShaderProgram & shader = *cullingShaders[Group];
		shader.use();
		shader.setUniform("uType", static_cast<GLuint>(ImpostorModel));
		shader.setUniform("uPointCount", settings.pointCount);
		prefixSumInfoSsbo.bindSsbo(0);
		elementBuffers[1 + resultIndex].bindSsbo(1);
		elementBuffers[0].bindSsbo(2);
		glDispatchCompute(static_cast<GLuint>(xWorkGroups), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	printGLuintBuffer(elementBuffers[0], "Current Element Buffer 0");

	LOG << "7. Build command buffers";
	{
		const ShaderProgram & shader = *cullingShaders[BuildCommandBuffer];
		shader.use();
		shader.setUniform("uPointCount", settings.pointCount);
		shader.setUniform("uGrainMeshPointCount", static_cast<GLuint>(314));
		prefixSumInfoSsbo.bindSsbo(0);
		commandBuffer.bindSsbo(1);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	LOG << "8. Collect info";
	PrefixSumInfo outputPrefixSumInfo;
	CommandBuffer outputCommandBuffer;
	prefixSumInfoSsbo.readBlock<PrefixSumInfo>(0, [&outputPrefixSumInfo](PrefixSumInfo *data, size_t _) {
		outputPrefixSumInfo = data[0];
	});
	commandBuffer.readBlock<CommandBuffer>(0, [&outputCommandBuffer](CommandBuffer *data, size_t _) {
		outputCommandBuffer = data[0];
	});

	LOG << "Checking output...";
	std::cout
		<< "PrefixSumInfo {\n"
		<< "  instanceCount: " << outputPrefixSumInfo.instanceCount << "\n"
		<< "  impostorCount: " << outputPrefixSumInfo.impostorCount << "\n"
		<< "  isLastPointInstance: " << outputPrefixSumInfo.isLastPointInstance << "\n"
		<< "  isLastPointImpostor: " << outputPrefixSumInfo.isLastPointImpostor << "\n"
		<< "}\n";

	std::cout
		<< "CommandBuffer {\n"
		<< "  impostorCommand: {\n"
		<< "    count: " << outputCommandBuffer.impostorCommand.count << "\n"
		<< "    instanceCount: " << outputCommandBuffer.impostorCommand.instanceCount << "\n"
		<< "    firstIndex: " << outputCommandBuffer.impostorCommand.firstIndex << "\n"
		<< "    baseVertex: " << outputCommandBuffer.impostorCommand.baseVertex << "\n"
		<< "    baseInstance: " << outputCommandBuffer.impostorCommand.baseInstance << "\n"
		<< "  }\n"
		<< "  instanceCommand: {\n"
		<< "    count: " << outputCommandBuffer.instanceCommand.count << "\n"
		<< "    instanceCount: " << outputCommandBuffer.instanceCommand.instanceCount << "\n"
		<< "    first: " << outputCommandBuffer.instanceCommand.first << "\n"
		<< "    baseInstance: " << outputCommandBuffer.instanceCommand.baseInstance << "\n"
		<< "  }\n"
		<< "}\n";

	std::ostringstream elementSs;
	std::ostringstream modelSs;
	elementBuffers[0].readBlock<GLuint>(0, [&renderTypeData, &elementSs, &modelSs](GLuint *data, size_t size) {
		auto seenModels = std::set<GLuint>();
		GLuint currentModel = 999;
		for (int i = 0; i < size; ++i) {
			int model = renderTypeData[data[i]];
			if (currentModel != model) {
				if (seenModels.count(model) > 0) {
					ERR_LOG << "Invalid output element buffer at index #" << i << ": new block of model type " << model << " starts here.";
					return;
				}
				seenModels.insert(model);
				currentModel = model;
			}
			if (i != 0) {
				elementSs << ", ";
				modelSs << ", ";
			}
			elementSs << data[i];
			modelSs << model;
		}
	});

	LOG << "Output Element Buffer: [" << elementSs.str() << "]";
	LOG << "Output Models: [" << modelSs.str() << "]";

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

int prefixSum(const GlBuffer & buffer0, const GlBuffer & buffer1, int elementCount, const ShaderProgram & shader)
{
	if (!shader.isValid()) return -1;

	int iterationCount = static_cast<int>(2 * (floor(log(elementCount) / log(2)) + 1));

	shader.use();
	shader.setUniform("uElementCount", static_cast<GLuint>(elementCount));
	for (int i = 0; i < iterationCount; ++i) {
		// Element buffers 1 and 2 are alternatively used as previous and current buffer
		buffer0.bindSsbo(i % 2 == 0 ? 1 : 2);
		buffer1.bindSsbo(i % 2 == 0 ? 2 : 1);
		shader.setUniform("uIteration", static_cast<GLuint>(i));
		glDispatchCompute(static_cast<GLuint>((elementCount + 127) / 128), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	return iterationCount % 2;
}

int randint(int upper)
{
	int x = upper;
	while (x >= upper)
		x = std::rand() / ((RAND_MAX + 1u) / upper);
	return x;
}

void printGLuintBuffer(const GlBuffer & buffer, std::string name)
{
	std::ostringstream ss;
	buffer.readBlock<GLuint>(0, [&ss](GLuint *data, size_t size) {
		for (int i = 0; i < size; ++i) {
			if (i != 0) ss << ", ";
			ss << data[i];
		}
	});
	LOG << name << ": [" << ss.str() << "]";
}
