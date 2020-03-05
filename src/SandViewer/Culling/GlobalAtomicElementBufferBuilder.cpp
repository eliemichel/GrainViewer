#include "GlBuffer.h"
#include "ShaderPool.h"
#include "Logger.h"
#include "GlobalAtomicElementBufferBuilder.h"

//#define EXTRA_DEBUG

bool GlobalAtomicElementBufferBuilder::load(const Settings & settings)
{
	m_xWorkGroups = (settings.pointCount + (settings.local_size_x - 1)) / settings.local_size_x;

	LOG << "Loading shaders...";
	ShaderPool::AddShader("GlobalAtomic", settings.globalAtomicShaderName, ShaderProgram::ComputeShader, {});
	m_shader = ShaderPool::GetShader("GlobalAtomic");
	if (!m_shader->isValid()) return false;

	LOG << "Allocate element buffer section info SSBO...";
	m_elementBuffer.addBlock<GLuint>(settings.pointCount);
	m_elementBuffer.alloc();

	m_countersSsbo.addBlock<GLuint>(2 * _RenderModelCount);
	m_countersSsbo.alloc();
	m_countersSsbo.fillBlock<GLuint>(0, [](GLuint *data, size_t count) {
		memset(data, 0, count * sizeof(GLuint));
	});

	LOG << "Sending reference render type info to GPU...";
	m_renderTypeSsbo.importBlock(renderTypeData());

	return true;
}

void GlobalAtomicElementBufferBuilder::build()
{
	m_shader->use();
	m_shader->setUniform("uPointCount", pointCount());
	m_shader->setUniform("uRenderModelCount", static_cast<GLuint>(_RenderModelCount));

	m_countersSsbo.bindSsbo(0);
	m_renderTypeSsbo.bindSsbo(1);
	m_elementBuffer.bindSsbo(2);

	for (GLuint i = 0; i < 4; ++i) {
		m_shader->setUniform("uStep", i);
		glDispatchCompute(i == 0 || i == 2 ? 1 : static_cast<GLuint>(m_xWorkGroups), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

#ifdef EXTRA_DEBUG
		std::ostringstream out;
		m_countersSsbo.readBlock<GLuint>(0, [&out](GLuint *data, size_t size) {
			for (int i = 0; i < size; ++i) {
				if (i != 0) out << ", ";
				out << data[i];
			}
		});
		LOG << "countersSsbo: [" << out.str() << "]";
#endif // EXTRA_DEBUG
	}
}
