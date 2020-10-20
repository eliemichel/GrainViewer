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

class GlTexture {
public:
	/**
	 * Create a texture for a given target (same target as in glCreateTextures)
	 */
	explicit GlTexture(GLenum target);
	
	/**
	 * Take ownership of a raw gl texture.
	 * This object will take care of deleting texture, unless you release() it.
	 */
	explicit GlTexture(GLuint id, GLenum target);

	~GlTexture();
	GlTexture(const GlTexture&) = delete;
	GlTexture& operator=(const GlTexture&) = delete;
	GlTexture(GlTexture&&) = default;
	GlTexture& operator=(GlTexture&&) = default;

	void storage(GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth);
	void storage(GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height);
	void subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
	void subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels);
	void generateMipmap() const;
	void setWrapMode(GLenum wrap) const;

	GLuint raw() const { return m_id; }
	GLsizei width() const { return m_width; }
	GLsizei height() const { return m_height; }
	GLsizei depth() const { return m_depth; }
	GLsizei levels() const { return m_levels; }
	GLenum target() const { return m_target; }

	bool isValid() const { return m_id != invalid; }
	void bind(GLint unit) const;
	void bind(GLuint unit) const;

	// forget texture without deleting it, use with caution
	void release() { m_id = invalid; }
	
private:
	static const GLuint invalid;

private:
	GLuint m_id;
	GLenum m_target;
	GLsizei m_width = 0;
	GLsizei m_height = 0;
	GLsizei m_depth = 0;
	GLsizei m_levels = 1;
};
