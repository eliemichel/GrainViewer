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

#include "GlBuffer.h"

GlBuffer::~GlBuffer() {
	free();
}

void GlBuffer::addBlockAttribute(size_t blockId, GLint size, GLuint divisor) {
	Block & b = m_blocks[blockId];
	BlockAttribute attr{ size, GL_FLOAT, divisor, 0 };
	if (!b.attributes.empty()) {
		BlockAttribute prevAttr = b.attributes.back();
		attr.byteOffset = prevAttr.byteOffset + prevAttr.size * sizeof(GLfloat);
	}
	b.attributes.push_back(attr);
}

void GlBuffer::addBlockAttributeUint(size_t blockId, GLint size, GLuint divisor) {
	Block & b = m_blocks[blockId];
	BlockAttribute attr{ size, GL_UNSIGNED_INT, divisor, 0 };
	if (!b.attributes.empty()) {
		BlockAttribute prevAttr = b.attributes.back();
		attr.byteOffset = prevAttr.byteOffset + prevAttr.size * sizeof(GLuint);
	}
	b.attributes.push_back(attr);
}

void GlBuffer::alloc() {
	if (m_isAllocated) {
		ERR_LOG << "Cannot allocate buffer twice";
		return;
	}
	glCreateBuffers(1, &m_buffer);
	glNamedBufferStorage(m_buffer, byteSize(), NULL, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
	m_isAllocated = true;
}

void GlBuffer::free() {
	if (isAllocated()) {
		glDeleteBuffers(1, &m_buffer);
		m_isAllocated = false;
		m_buffer = 0;
	}
}

void GlBuffer::enableAttributes(GLuint vao) {
	GLintptr blockOffset = 0;
	GLuint id = 0;
	for (auto b : m_blocks) {
		for (auto attr : b.attributes) {
			GLintptr offset = blockOffset + static_cast<GLintptr>(attr.byteOffset);
			/* // More modern, but fails on some devices :/
			glEnableVertexArrayAttrib(vao, id);
			glVertexAttribPointer(id, attr.size, attr.type, GL_FALSE, b.stride, static_cast<GLvoid*>(static_cast<char*>(0) + offset));
			glVertexAttribDivisor(id, attr.divisor);
			/*/
			GLuint bindingindex = id;
			glVertexArrayVertexBuffer(vao, bindingindex, m_buffer, offset, b.stride);
			glVertexArrayBindingDivisor(vao, bindingindex, attr.divisor);
			glEnableVertexArrayAttrib(vao, id);
			glVertexArrayAttribBinding(vao, id, bindingindex);
			glVertexArrayAttribFormat(vao, id, attr.size, attr.type, GL_FALSE, 0);
			//*/
			++id;
		}
		blockOffset = b.endByteOffset;
	}
}

void GlBuffer::finalize() {
	// Free memory
	m_blocks.resize(0);
}

void GlBuffer::bind() const {
	glBindBuffer(m_target, m_buffer);
}

void GlBuffer::bindSsbo(GLuint index) const {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_buffer);
}

void GlBuffer::unbind() const {
	glBindBuffer(m_target, 0);
}
