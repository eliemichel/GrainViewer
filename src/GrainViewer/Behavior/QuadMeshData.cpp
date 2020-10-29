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
