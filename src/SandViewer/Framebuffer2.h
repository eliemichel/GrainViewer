// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/modernglad.h>

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

private:
	GLuint m_raw;
};

