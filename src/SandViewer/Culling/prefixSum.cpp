#include "GlBuffer.h"
#include "ShaderProgram.h"

int prefixSum(const GlBuffer & buffer0, const GlBuffer & buffer1, const ShaderProgram & shader, GLuint pointCount)
{
	if (!shader.isValid()) return -1;

	GLuint iterationCount = static_cast<GLuint>(2 * (floor(log(pointCount) / log(2)) + 1));

	shader.use();
	shader.setUniform("uElementCount", pointCount);
	for (GLuint i = 0; i < iterationCount; ++i) {
		// Element buffers 1 and 2 are alternatively used as previous and current buffer
		buffer0.bindSsbo(i % 2 == 0 ? 1 : 2);
		buffer1.bindSsbo(i % 2 == 0 ? 2 : 1);
		shader.setUniform("uIteration", i);
		glDispatchCompute((pointCount + 127) / 128, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	return iterationCount % 2;
}

