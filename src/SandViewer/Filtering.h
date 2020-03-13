#pragma once

#include <memory>
#include "GlTexture.h"
#include "Framebuffer.h"
#include "Framebuffer2.h"

class ShaderProgram;

struct LeanTexture {
	GlTexture lean1;
	GlTexture lean2;

	LeanTexture(GLenum target) : lean1(target), lean2(target) {}
};

/**
 * A large quad covering the whole screen
 */
class PostEffect
{
public:
	PostEffect();
	~PostEffect();
	void draw(bool disableDepthTest = true);
private:
	GLuint m_vao;
	GLuint m_vbo;
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
	/*
	enum Type {
		StandardFiltering,
		LeanFiltering,
	};
	*/

	static std::unique_ptr<LeanTexture> CreateLeanTexture(const GlTexture & sourceTexture);

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

private:
	static std::unique_ptr<MipmapDepthBufferGenerator> s_mipmapDepthBufferGenerator;
	static std::unique_ptr<PostEffect> s_postEffectQuad;
	static std::unique_ptr<Framebuffer2> s_postEffectFramebuffer;
	static std::unique_ptr<Framebuffer2> s_postEffectDepthOnlyFramebuffer;
};
