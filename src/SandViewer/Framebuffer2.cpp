// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

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
