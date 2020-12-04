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

#include "Logger.h"
#include "Filtering.h"
#include "ShaderPool.h"
#include "utils/ScopedFramebufferOverride.h"

#include <algorithm>
#include <cassert>

std::unique_ptr<MipmapDepthBufferGenerator> Filtering::s_mipmapDepthBufferGenerator;
std::unique_ptr<Framebuffer2> Filtering::s_postEffectFramebuffer;
std::unique_ptr<Framebuffer2> Filtering::s_postEffectDepthOnlyFramebuffer;

//-----------------------------------------------------------------------------

void Filtering::Blit(GlTexture & destination, GLuint source, const ShaderProgram & shader)
{
	// Init on first use
	if (!s_postEffectFramebuffer)
	{
		s_postEffectFramebuffer = std::make_unique<Framebuffer2>();
		std::vector<GLenum> drawBuffers = { GL_COLOR_ATTACHMENT0 };
		glNamedFramebufferDrawBuffers(s_postEffectFramebuffer->raw(), static_cast<GLsizei>(drawBuffers.size()), &drawBuffers[0]);
	}

	glNamedFramebufferTexture(s_postEffectFramebuffer->raw(), GL_COLOR_ATTACHMENT0, destination.raw(), 0);
	s_postEffectFramebuffer->bind();

	shader.use();
	glBindTextureUnit(0, source);
	shader.setUniform("uMainTexture", 0);
	glViewport(0, 0, destination.width(), destination.height());
	PostEffect::Draw();

	glTextureBarrier();
}

void Filtering::Blit(GlTexture & destination, const GlTexture & source, const ShaderProgram & shader)
{
	Blit(destination, source.raw(), shader);
}

void Filtering::BlitDepth(GlTexture & destination, GLuint source, const ShaderProgram & shader)
{
	// Init on first use
	if (!s_postEffectDepthOnlyFramebuffer)
	{
		s_postEffectDepthOnlyFramebuffer = std::make_unique<Framebuffer2>();
		glNamedFramebufferDrawBuffer(s_postEffectDepthOnlyFramebuffer->raw(), GL_NONE);
	}

	glNamedFramebufferTexture(s_postEffectDepthOnlyFramebuffer->raw(), GL_DEPTH_ATTACHMENT, destination.raw(), 0);
	s_postEffectDepthOnlyFramebuffer->bind();

	shader.use();
	glBindTextureUnit(0, source);
	shader.setUniform("uMainTexture", 0);
	glViewport(0, 0, destination.width(), destination.height());

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
	PostEffect::DrawWithDepthTest();

	glTextureBarrier();
}

void Filtering::BlitDepth(GlTexture & destination, const GlTexture & source, const ShaderProgram & shader)
{
	BlitDepth(destination, source.raw(), shader);
}

void Filtering::MipMapUsingAlpha(GlTexture& texture, const GlTexture& alpha)
{
	ScopedFramebufferOverride framebufferOverride;

	Framebuffer2 fbo;
	fbo.bind();
	std::vector<GLenum> drawBuffers(1);
	drawBuffers[0] = GL_COLOR_ATTACHMENT0;
	glNamedFramebufferDrawBuffers(fbo.raw(), static_cast<GLsizei>(drawBuffers.size()), &drawBuffers[0]);

	const ShaderProgram& shader = *ShaderPool::GetShader("MipMapUsingAlpha");
	shader.use();
	texture.bind(0);
	shader.setUniform("uPreviousLevel", 0);
	alpha.bind(1);
	shader.setUniform("uAlphaLevel", 1);

	// Save texture limits
	GLint w, h, numLayers;
	glGetTextureLevelParameteriv(texture.raw(), 0, GL_TEXTURE_WIDTH, &w);
	glGetTextureLevelParameteriv(texture.raw(), 0, GL_TEXTURE_HEIGHT, &h);
	glGetTextureLevelParameteriv(texture.raw(), 0, GL_TEXTURE_DEPTH, &numLayers);

	GLint baseLevel, maxLevel;
	glGetTextureParameteriv(texture.raw(), GL_TEXTURE_BASE_LEVEL, &baseLevel);
	glGetTextureParameteriv(texture.raw(), GL_TEXTURE_MAX_LEVEL, &maxLevel);
	GLsizei numLevels = static_cast<GLsizei>(1 + floor(log2(std::max(w, h))));

	for (GLsizei level = 0; level < numLevels - 1; ++level) {
		shader.setUniform("uLevel", static_cast<GLint>(level));

		GLsizei nextLevel = level + 1;
		GLint w, h;
		glGetTextureLevelParameteriv(texture.raw(), nextLevel, GL_TEXTURE_WIDTH, &w);
		glGetTextureLevelParameteriv(texture.raw(), nextLevel, GL_TEXTURE_HEIGHT, &h);

		glTextureParameteri(texture.raw(), GL_TEXTURE_BASE_LEVEL, level);
		glTextureParameteri(texture.raw(), GL_TEXTURE_MAX_LEVEL, level);

		fbo.attachTexture(0, texture, nextLevel);

		glViewport(0, 0, w, h);
		glClear(GL_COLOR_BUFFER_BIT);

		for (GLint layer = 0; layer < numLayers; ++layer) {
			shader.setUniform("uLayer", layer);
			PostEffect::Draw();
		}

		glTextureBarrier();
	}

	// Restore mipmap limits
	glTextureParameteri(texture.raw(), GL_TEXTURE_BASE_LEVEL, baseLevel);
	glTextureParameteri(texture.raw(), GL_TEXTURE_MAX_LEVEL, maxLevel);
}

//-----------------------------------------------------------------------------

MipmapDepthBufferGenerator::MipmapDepthBufferGenerator()
{
	m_shader = ShaderPool::GetShader("GenerateMipmapDepthBuffer");
}

void MipmapDepthBufferGenerator::generate(Framebuffer & framebuffer)
{
	framebuffer.bind();
	glEnable(GL_DEPTH_TEST);
	m_shader->use();
	glBindTextureUnit(0, framebuffer.depthTexture());
	m_shader->setUniform("previousLevel", 0);

	GLint baseLevel, maxLevel;
	glGetTextureParameteriv(framebuffer.depthTexture(), GL_TEXTURE_BASE_LEVEL, &baseLevel);
	glGetTextureParameteriv(framebuffer.depthTexture(), GL_TEXTURE_MAX_LEVEL, &maxLevel);

	for (int i = 0; i < framebuffer.colorTextureCount(); ++i) {
		glNamedFramebufferTexture(framebuffer.raw(), GL_COLOR_ATTACHMENT0 + i, 0, 0);
	}

	for (GLsizei level = 0; level < framebuffer.depthLevels() - 1; ++level) {
		GLsizei nextLevel = level + 1;
		GLint w, h;
		glGetTextureLevelParameteriv(framebuffer.depthTexture(), nextLevel, GL_TEXTURE_WIDTH, &w);
		glGetTextureLevelParameteriv(framebuffer.depthTexture(), nextLevel, GL_TEXTURE_HEIGHT, &h);
		
		glTextureParameteri(framebuffer.depthTexture(), GL_TEXTURE_BASE_LEVEL, level);
		glTextureParameteri(framebuffer.depthTexture(), GL_TEXTURE_MAX_LEVEL, level);

		glNamedFramebufferTexture(framebuffer.raw(), GL_DEPTH_ATTACHMENT, framebuffer.depthTexture(), nextLevel);
		glViewport(0, 0, w, h);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DrawWithDepthTest();
		glTextureBarrier();
	}

	// Restore framebuffer
	glTextureParameteri(framebuffer.depthTexture(), GL_TEXTURE_BASE_LEVEL, baseLevel);
	glTextureParameteri(framebuffer.depthTexture(), GL_TEXTURE_MAX_LEVEL, maxLevel);

	glNamedFramebufferTexture(framebuffer.raw(), GL_DEPTH_ATTACHMENT, framebuffer.depthTexture(), 0);
	for (int i = 0; i < framebuffer.colorTextureCount(); ++i) {
		glNamedFramebufferTexture(framebuffer.raw(), GL_COLOR_ATTACHMENT0 + i, framebuffer.colorTexture(i), 0);
	}
}
