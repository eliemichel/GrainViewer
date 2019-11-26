// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include "Logger.h"

#include <glad/glad.h>
#include <vector>
#include <functional>
#include <cassert>

using std::placeholders::_1;
using std::placeholders::_2;

/**
 * Wrapper around OpenGL's named buffers
 * Usage:
 *   GlBuffer buff;
 *   buff.addBlock<Type1>(nb);
 *   buff.addBlock<Type2>(nb);
 *   buff.alloc();
 *   buff.fillBlock(0, fillingFunction1, ...);
 *   buff.fillBlock(1, fillingFunction2, ...);
 *   // [...]
 *   buff.bind();
 *   buff.enableAttributes();
 *   // When nothing will change any more
 *   buff.finalize();
 *
 * TODO: Distinguish VertexBufferLayout from the buffer itself?
 */
class GlBuffer {
public:
	inline GlBuffer(GLenum target)
		: m_target(target)
		, m_isAllocated(false)
	{}
	~GlBuffer();

	template <class T>
	inline void addBlock(size_t nbElements = 1) {
		GLsizei stride = static_cast<GLsizei>(sizeof(T));
		m_blocks.push_back(Block{ nbElements, stride, byteSize() + static_cast<GLsizei>(nbElements * stride) });
	}

	void addBlockAttribute(size_t blockId, GLint size, GLuint divisor = 0);
	void addBlockAttributeUint(size_t blockId, GLint size, GLuint divisor = 0);

	/// Allocate buffer
	void alloc();
	/// Free buffer
	void free();

	/// Fill buffer
	template <class T>
	inline void fillBlock(size_t blockId, std::function<void(T*, size_t)> fill_callback) {
		const Block & b = m_blocks[blockId];
		assert(sizeof(T) == b.stride);
		GLsizei size = static_cast<GLsizei>(b.nbElements * sizeof(T));
		GLsizei offset = b.endByteOffset - size;
		T *attributes = static_cast<T*>(glMapNamedBufferRange(
			m_buffer, offset, size,
			GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT
		));
		fill_callback(attributes, b.nbElements);
		glUnmapNamedBuffer(m_buffer);
	}

	template <class T>
	inline void readBlock(size_t blockId, std::function<void(T*, size_t)> fill_callback) {
		const Block & b = m_blocks[blockId];
		assert(sizeof(T) == b.stride);
		GLsizei size = static_cast<GLsizei>(b.nbElements * sizeof(T));
		GLsizei offset = b.endByteOffset - size;
		T *attributes = static_cast<T*>(glMapNamedBufferRange(
			m_buffer, offset, size,
			GL_MAP_READ_BIT
		));
		fill_callback(attributes, b.nbElements);
		glUnmapNamedBuffer(m_buffer);
	}

	// Only works with const reference arguments (I gave up for the more general one)
	// There must be sth to do w/ std::forward<Args>(args)...
	template <class T, typename... Args>
	inline void fillBlock(size_t blockId, void(*fill_callback)(T*, size_t, const Args&...), const Args&... args) {
		fillBlock<T>(blockId, std::bind(fill_callback, _1, _2, args...));
	}

	/// To be called when buffer is bound, in a VAO
	void enableAttributes();

	/// Free memory that was used for building. Never call fill_block() or fill() after that
	void finalize();

	void bind() const;
	/// Bind as SSBO
	void bindSsbo(GLuint index) const;
	void unbind() const;

	inline GLuint name() const { return m_buffer; }
	inline bool isAllocated() const { return m_isAllocated; }

private:
	inline GLsizei byteSize() { return m_blocks.size() == 0 ? 0 : m_blocks.back().endByteOffset; }

private:
	typedef struct {
		GLint size;
		GLenum type;
		GLuint divisor;
		GLsizei byteOffset;
	} BlockAttribute;

	typedef struct {
		size_t nbElements;
		GLsizei stride;
		GLsizei endByteOffset;
		std::vector<BlockAttribute> attributes;
	} Block;

private:
	GLuint m_buffer;
	GLenum m_target;
	bool m_isAllocated;
	std::vector<Block> m_blocks;
};
