// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include <OpenGL>

class GlTexture;

/**
 * New version of Framebuffer, but teh latter is used in so many places that as
 * a transitional solution it is better to have two versions at the same time.
 */
class Framebuffer2 {
public:
	Framebuffer2();
	~Framebuffer2();
	Framebuffer2(Framebuffer2&) = delete;
	Framebuffer2& operator=(Framebuffer2&) = delete;
	Framebuffer2(Framebuffer2&&) = default;
	Framebuffer2& operator=(Framebuffer2&&) = default;

	void bind() const;
	GLuint raw() const { return m_raw; }
	void attachTexture(int attachment, const GlTexture & texture, GLint level);

private:
	GLuint m_raw;
};

