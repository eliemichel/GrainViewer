/**
 * This file is part of GrainViewer
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#include "PostEffect.h"

std::unique_ptr<PostEffect> PostEffect::s_instance;

void PostEffect::Draw(bool disableDepthTest)
{
	if (!s_instance) s_instance = std::make_unique<PostEffect>();
	s_instance->draw(disableDepthTest);
}

void PostEffect::DrawInstanced(int n)
{
	if (!s_instance) s_instance = std::make_unique<PostEffect>();
	s_instance->draw(true, n);
}

PostEffect::PostEffect()
{
	static GLfloat points[] = {
		-1, -1, 0,
		3, -1, 0,
		-1, 3, 0
	};

	glCreateBuffers(1, &m_vbo);
	glNamedBufferStorage(m_vbo, sizeof(points), points, 0);

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

void PostEffect::draw(bool disableDepthTest, int instances)
{
	if (disableDepthTest) glDisable(GL_DEPTH_TEST);
	glBindVertexArray(m_vao);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 3, instances);
}

