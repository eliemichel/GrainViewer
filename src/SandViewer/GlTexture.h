#pragma once

#include <glad/glad.h>

class GlTexture {
public:
	GlTexture(GLenum target);
	GlTexture(GLuint id, GLenum target); // move from raw gl texture. This object will take care of deleting texture

	GlTexture(const GlTexture&) = delete;
	void operator=(const GlTexture&) = delete;
	GlTexture(GlTexture&&) = default;
	GlTexture& operator=(GlTexture&&) = default;

	~GlTexture();

	void storage(GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth);
	void storage(GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height);
	void subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels);
	void subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels);
	void generateMipmap() const;
	void setWrapMode(GLenum wrap) const;

	GLuint raw() const { return m_id; }
	GLsizei width() const { return m_width; }
	GLsizei height() const { return m_height; }
	GLsizei depth() const { return m_depth; }
	GLenum target() const { return m_target; }

	bool isValid() const { return m_id != invalid; }
	void bind() const; // depreciated
	void bind(GLint unit) const;
	void bind(GLuint unit) const;
	
private:
	static const GLuint invalid;

private:
	GLuint m_id;
	GLenum m_target;
	GLsizei m_width;
	GLsizei m_height;
	GLsizei m_depth;
};
