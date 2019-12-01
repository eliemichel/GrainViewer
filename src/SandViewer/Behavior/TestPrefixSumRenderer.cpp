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

	m_elementBuffer1 = std::make_unique<GlBuffer>(GL_SHADER_STORAGE_BUFFER);
	m_elementBuffer1->addBlock<GLuint>(m_pointCount);
	m_elementBuffer1->alloc();

	m_elementBuffer2 = std::make_unique<GlBuffer>(GL_SHADER_STORAGE_BUFFER);
	m_elementBuffer2->addBlock<GLuint>(m_pointCount);
	m_elementBuffer2->alloc();
}

void TestPrefixSumRenderer::update(float time)
{
	if (m_shader && m_shader->isValid()) {
		m_shader->setUniform("uTime", time);
	}
	m_frame++;
}

void TestPrefixSumRenderer::render(const Camera & camera, const World & world, RenderType target) const
{
	if (m_frame % 100 != 1) return; // run only one frame out of 100
	if (!m_shader || !m_shader->isValid()) return;

	int iterationCount = static_cast<int>(2 * (floor(log(m_pointCount)/log(2))+1));

	// 1. Random init element buffer
	m_elementBuffer1->fillBlock<GLuint>(0, [](GLuint *data, size_t size) {
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
	m_shader->setUniform("uPointCount", static_cast<GLuint>(m_pointCount));
	for (int i = 0 ; i < iterationCount ; ++i) {
		// Element buffers 1 and 2 are alternatively used as previous and current buffer
		m_elementBuffer1->bindSsbo(i % 2 == 0 ? 1 : 2);
		m_elementBuffer2->bindSsbo(i % 2 == 0 ? 2 : 1);
		m_shader->setUniform("uIteration", static_cast<GLuint>(i));
		glDispatchCompute(static_cast<GLuint>((m_pointCount + 127) / 128), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	// 3. Display sorted buffer
	auto & resultBuffer = iterationCount % 2 == 0 ? m_elementBuffer1 : m_elementBuffer2;
	resultBuffer->readBlock<GLuint>(0, [](GLuint *data, size_t size) {
		std::stringstream oss;
		for (size_t i = 0 ; i < size ; ++i) {
			if (i > 0) oss << ", ";
			oss << data[i];
		}
		DEBUG_LOG << "Element buffer sorted: " << oss.str();
	});
}
