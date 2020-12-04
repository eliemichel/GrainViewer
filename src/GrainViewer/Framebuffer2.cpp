/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
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

#include "Framebuffer2.h"
#include "Logger.h"
#include "GlTexture.h"

#include <vector>

Framebuffer2::Framebuffer2()
{
	glCreateFramebuffers(1, &m_raw);
}

Framebuffer2::~Framebuffer2()
{
	glDeleteFramebuffers(1, &m_raw);
}

void Framebuffer2::bind() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_raw);
}

bool Framebuffer2::check() const
{
	return glCheckNamedFramebufferStatus(m_raw, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

void Framebuffer2::attachTexture(int attachment, const GlTexture& texture, GLint level)
{
	glNamedFramebufferTexture(m_raw, GL_COLOR_ATTACHMENT0 + attachment, texture.raw(), level);
}

void Framebuffer2::attachDepthTexture(const GlTexture& texture, GLint level)
{
	glNamedFramebufferTexture(m_raw, GL_DEPTH_ATTACHMENT, texture.raw(), level);
}

void Framebuffer2::enableDrawBuffers(int n)
{
	std::vector<GLenum> drawBuffers(n);
	for (size_t k = 0; k < n; ++k) {
		drawBuffers[k] = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(k);
	}
	glNamedFramebufferDrawBuffers(m_raw, static_cast<GLsizei>(drawBuffers.size()), &drawBuffers[0]);
}
