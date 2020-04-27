#include "QuadMeshData.h"

#include "utils/behaviorutils.h"

void QuadMeshData::start()
{
	m_vbo = std::make_unique<GlBuffer>(GL_ARRAY_BUFFER);
	m_vbo->importBlock(std::vector<glm::vec2>{
		{ 0, 0 },
		{ 1, 0 },
		{ 0, 1 },
		{ 1, 1 }
	});
	m_vbo->addBlockAttribute(0, 2);  // uv
	
	glCreateVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	m_vbo->bind();
	m_vbo->enableAttributes(m_vao);
	glBindVertexArray(0);

	m_vbo->finalize();
}

void QuadMeshData::onDestroy()
{
	glDeleteVertexArrays(1, &m_vao);
}

void QuadMeshData::draw() const
{
	glBindVertexArray(m_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
