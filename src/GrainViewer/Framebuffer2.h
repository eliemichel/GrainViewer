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
	bool check() const;
	GLuint raw() const { return m_raw; }
	void attachTexture(int attachment, const GlTexture & texture, GLint level);
	void attachDepthTexture(const GlTexture& texture, GLint level);

	// Enables drawing to attachements 0, 1, ..., n - 1
	void enableDrawBuffers(int n);

private:
	GLuint m_raw;
};

