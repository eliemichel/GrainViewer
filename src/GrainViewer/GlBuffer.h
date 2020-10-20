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

#pragma once

#include <OpenGL>

#include <vector>
#include <functional>
#include <cassert>

#include "Logger.h"

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
		: m_buffer(0 /* todo: replace with invalid id */)
		, m_target(target)
		, m_isAllocated(false)
	{}
	~GlBuffer();
	GlBuffer(const GlBuffer &) = delete;
	GlBuffer & operator=(const GlBuffer &) = delete;

	template <class T>
	inline void addBlock(size_t nbElements = 1) {
		GLsizei stride = static_cast<GLsizei>(sizeof(T));
		m_blocks.push_back(Block{ nbElements, stride, byteSize() + static_cast<GLsizei>(nbElements * stride) });
	}

	/**
	 * Short for:
	 *   addBlock(data.size());
	 *   alloc();
	 *   fillBlock(...); // from data
	 */
	template <class T>
	inline void importBlock(const std::vector<T> & data) {
		addBlock<T>(data.size());
		alloc();
		fillBlock<T>(m_blocks.size() - 1, [&data](T *gpuData, size_t size) {
			memcpy(gpuData, data.data(), size * sizeof(T));
		});
	}

	template <class T>
	inline void exportBlock(size_t blockId, std::vector<T>& data) const {
		readBlock<T>(blockId, [&data](T* gpuData, size_t size) {
			memcpy(data.data(), gpuData, size * sizeof(T));
		});
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
	inline void readBlock(size_t blockId, std::function<void(T*, size_t)> fill_callback) const {
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
	void enableAttributes(GLuint vao);

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
	struct BlockAttribute {
		GLint size = 0;
		GLenum type = GL_ARRAY_BUFFER;
		GLuint divisor = 1;
		GLsizei byteOffset = 0;
	};

	struct Block {
		size_t nbElements = 0;
		GLsizei stride = 0;
		GLsizei endByteOffset = 0;
		std::vector<BlockAttribute> attributes = {};
	};

private:
	GLuint m_buffer;
	GLenum m_target;
	bool m_isAllocated;
	std::vector<Block> m_blocks;
};
