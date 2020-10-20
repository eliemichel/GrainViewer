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

#include "GlTexture.h"
#include "Framebuffer.h"
#include "Framebuffer2.h"
#include "PostEffect.h"

#include <memory>

class ShaderProgram;

struct LeanTexture {
	GlTexture lean1;
	GlTexture lean2;

	LeanTexture(GLenum target) : lean1(target), lean2(target) {}
};

class MipmapDepthBufferGenerator : public PostEffect
{
public:
	MipmapDepthBufferGenerator();
	void generate(Framebuffer & framebuffer);
private:
	std::shared_ptr<ShaderProgram> m_shader;
};

class Filtering {
public:
	/**
	 * Blit texture <source> onto texture <destination> using shader program <shader>
	 */
	static void Blit(GlTexture & destination, GLuint source, const ShaderProgram & shader);
	static void Blit(GlTexture & destination, const GlTexture & source, const ShaderProgram & shader);

	/**
	 * Same as Blit, but attaching destination texture to depth
	 */
	static void BlitDepth(GlTexture & destination, GLuint source, const ShaderProgram & shader);
	static void BlitDepth(GlTexture & destination, const GlTexture & source, const ShaderProgram & shader);

	/**
	 * Used for hierarchical depth buffer.
	 * Assumes that level 0 of depth buffer has been rendered, and compute
	 * other levels by max-ing neighbors
	 */
	static void MipmapDepthBuffer(Framebuffer & framebuffer) {
		if (!s_mipmapDepthBufferGenerator) s_mipmapDepthBufferGenerator = std::make_unique<MipmapDepthBufferGenerator>();
		s_mipmapDepthBufferGenerator->generate(framebuffer);
	}

	/**
	 * Generate mipmaps for a textures weighted by the alpha channel of another one.
	 * (override this texture's alpha)
	 */
	static void MipMapUsingAlpha(GlTexture& texture, const GlTexture& alpha);

private:
	static std::unique_ptr<MipmapDepthBufferGenerator> s_mipmapDepthBufferGenerator;
	static std::unique_ptr<Framebuffer2> s_postEffectFramebuffer;
	static std::unique_ptr<Framebuffer2> s_postEffectDepthOnlyFramebuffer;
};
