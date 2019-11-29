#include <sstream>
#include <cstdlib>

#include "utils/jsonutils.h"
#include "ShaderProgram.h"
#include "ShaderPool.h"
#include "TestPrefixSumRenderer.h"

///////////////////////////////////////////////////////////////////////////////
// Behavior implementation
///////////////////////////////////////////////////////////////////////////////

bool TestPrefixSumRenderer::deserialize(const rapidjson::Value & json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);
	jrOption(json, "pointCount", m_pointCount, m_pointCount);
	return true;
}

void TestPrefixSumRenderer::start()
{
	m_shader = ShaderPool::GetShader(m_shaderName);

	m_elementBuffer = std::make_unique<GlBuffer>(GL_SHADER_STORAGE_BUFFER);
	m_elementBuffer->addBlock<GLuint>(m_pointCount);
	m_elementBuffer->alloc();
}

void TestPrefixSumRenderer::update(float time)
{
	if (m_shader && m_shader->isValid()) {
		m_shader->setUniform("uTime", time);
	}
}

void TestPrefixSumRenderer::render(const Camera & camera, const World & world, RenderType target) const
{
	int iterationCount = static_cast<double>(2 * (floor(log(m_pointCount)/log(2))+1));

	// 1. Random init element buffer
	m_elementBuffer->fillBlock<GLuint>(0, [](GLuint *data, size_t size) {
		std::stringstream oss;
		for (size_t i = 0 ; i < size ; ++i) {
			data[i] = rand() % 5;
			if (i > 0) oss << ", ";
			oss << data[i];
		}
		DEBUG_LOG << "Element buffer before sorting: " << oss.str();
	});

	// 2. Sort element buffer
	m_shader->use();
	m_elementBuffer->bindSsbo(1);
	for (int i = 0 ; i < iterationCount ; ++i) {
		m_shader->setUniform("uIteration", i);
		glDispatchCompute(static_cast<GLuint>((m_pointCount + 127) / 128), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	// 3. Display sorted buffer
	m_elementBuffer->readBlock<GLuint>(0, [](GLuint *data, size_t size) {
		std::stringstream oss;
		for (size_t i = 0 ; i < size ; ++i) {
			if (i > 0) oss << ", ";
			oss << data[i];
		}
		DEBUG_LOG << "Element buffer sorted: " << oss.str();
	});
}
