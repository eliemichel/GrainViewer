#include <algorithm>
#include <cassert>
#include "Logger.h"
#include "Filtering.h"
#include "ShaderPool.h"

std::unique_ptr<LeanTexture> Filtering::CreateLeanTexture(const GlTexture & sourceTexture)
{
	auto tex = std::make_unique<LeanTexture>(sourceTexture.target());
	auto shader = ShaderPool::GetShader("GenerateLeanMaps");

	GLsizei width = sourceTexture.width();
	GLsizei height = sourceTexture.height();
	GLsizei depth = sourceTexture.depth();
	GLsizei levels = static_cast<GLsizei>(1 + floor(log2(std::max(width, height))));

	switch (sourceTexture.target()) {
	case GL_TEXTURE_2D:
		assert(depth == 1);
		tex->lean1.storage(levels, GL_RGBA16, width, height);
		tex->lean2.storage(levels, GL_RGBA16, width, height);
		break;
	case GL_TEXTURE_2D_ARRAY:
	case GL_TEXTURE_3D:
		tex->lean1.storage(levels, GL_RGBA16, width, height, depth);
		tex->lean2.storage(levels, GL_RGBA16, width, height, depth);
		break;
	}

	GLuint l = 0; // level

	shader->use();
	glBindImageTexture(0, sourceTexture.raw(), l, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(1, tex->lean1.raw(), l, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(2, tex->lean2.raw(), l, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glDispatchCompute(width, height, depth);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	tex->lean1.generateMipmap();
	tex->lean2.generateMipmap();

	return std::move(tex);
}


/*


	// Create FBO writing to textures
	GLuint fbo;
	glCreateFramebuffers(1, &fbo);
	glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, tex->lean1.raw(), l);
	glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT1, tex->lean2.raw(), l);
	std::vector<GLenum> drawBuffers = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glNamedFramebufferDrawBuffers(fbo, static_cast<GLsizei>(drawBuffers.size()), &drawBuffers[0]);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		ERR_LOG << "Framebuffer not complete!";
	}

	// Push current FBO binding state
	GLint drawFboId = 0, readFboId = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFboId);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);

	shader->use();
	shader->setUniform("uWidth", width);
	shader->setUniform("uHeight", height);
	shader->setUniform("uDepth", depth);
	sourceTexture.bind(0);

	glDispatchCompute(width, height, depth);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// Pop FBO bindings
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER_BINDING, drawFboId);
	glBindFramebuffer(GL_READ_FRAMEBUFFER_BINDING, readFboId);


*/