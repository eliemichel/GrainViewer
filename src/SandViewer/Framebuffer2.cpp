// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#include "Framebuffer2.h"
#include "Logger.h"
#include "GlTexture.h"

using namespace std;

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

void Framebuffer2::attachTexture(int attachment, const GlTexture& texture, GLint level)
{
	glNamedFramebufferTexture(m_raw, GL_COLOR_ATTACHMENT0 + attachment, texture.raw(), level);
}
