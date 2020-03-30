#include "PostEffect.h"

std::unique_ptr<PostEffect> PostEffect::s_instance;

void PostEffect::Draw(bool disableDepthTest)
{
	if (!s_instance) s_instance = std::make_unique<PostEffect>();
	s_instance->draw(disableDepthTest);
}

PostEffect::PostEffect()
{
	static GLfloat points[] = {
		-1, -1, 0,
		3, -1, 0,
		-1, 3, 0
	};

	glCreateBuffers(1, &m_vbo);
	glNamedBufferStorage(m_vbo, sizeof(points), points, NULL);

	glCreateVertexArrays(1, &m_vao);
	glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, 3 * sizeof(GLfloat));
	glEnableVertexArrayAttrib(m_vao, 0);
	glVertexArrayAttribBinding(m_vao, 0, 0);
	glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
}

PostEffect::~PostEffect()
{
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vbo);
}

void PostEffect::draw(bool disableDepthTest)
{
	if (disableDepthTest) glDisable(GL_DEPTH_TEST);
	glBindVertexArray(m_vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

