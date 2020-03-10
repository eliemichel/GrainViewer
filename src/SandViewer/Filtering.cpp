#include <algorithm>
#include <cassert>
#include "Logger.h"
#include "Filtering.h"
#include "ShaderPool.h"

std::unique_ptr<MipmapDepthBufferGenerator> Filtering::s_mipmapDepthBufferGenerator;
std::unique_ptr<PostEffect> Filtering::s_postEffectQuad;
std::unique_ptr<Framebuffer2> Filtering::s_postEffectFramebuffer;

//-----------------------------------------------------------------------------

std::unique_ptr<LeanTexture> Filtering::CreateLeanTexture(const GlTexture & sourceTexture)
{
	auto tex = std::make_unique<LeanTexture>(sourceTexture.target());
	std::shared_ptr<ShaderProgram> shader;

	GLsizei width = sourceTexture.width();
	GLsizei height = sourceTexture.height();
	GLsizei depth = sourceTexture.depth();
	GLsizei levels = static_cast<GLsizei>(1 + floor(log2(std::max(width, height))));

	switch (sourceTexture.target()) {
	case GL_TEXTURE_2D:
		assert(depth == 1);
		tex->lean1.storage(levels, GL_RGBA16F, width, height);
		tex->lean2.storage(levels, GL_RGBA16F, width, height);
		shader = ShaderPool::GetShader("GenerateLeanMaps_Image2D");
		break;
	case GL_TEXTURE_2D_ARRAY:
		tex->lean1.storage(levels, GL_RGBA16F, width, height, depth);
		tex->lean2.storage(levels, GL_RGBA16F, width, height, depth);
		shader = ShaderPool::GetShader("GenerateLeanMaps_Image2DArray");
		break;
	case GL_TEXTURE_3D:
		tex->lean1.storage(levels, GL_RGBA16F, width, height, depth);
		tex->lean2.storage(levels, GL_RGBA16F, width, height, depth);
		shader = ShaderPool::GetShader("GenerateLeanMaps_Image3D");
		break;
	default:
		ERR_LOG << "Unsupported texture target: " << sourceTexture.target();
		return nullptr;
	}

	shader->use();
	glBindImageTexture(0, sourceTexture.raw(), 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
	glBindImageTexture(1, tex->lean1.raw(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glBindImageTexture(2, tex->lean2.raw(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute(width, height, depth);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	tex->lean1.generateMipmap();
	tex->lean2.generateMipmap();

	return std::move(tex);
}

//-----------------------------------------------------------------------------

void Filtering::Blit(GlTexture & destination, GLuint source, const ShaderProgram & shader)
{
	// Init on first use
	if (!s_postEffectQuad) s_postEffectQuad = std::make_unique<PostEffect>();
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
	s_postEffectQuad->draw();

	glTextureBarrier();
}

void Filtering::Blit(GlTexture & destination, const GlTexture & source, const ShaderProgram & shader)
{
	Blit(destination, source.raw(), shader);
}

//-----------------------------------------------------------------------------

PostEffect::PostEffect()
{
	static GLfloat points[] = {
		-1, -1, 0,
		3, -1, 0,
		-1, 3, 0
	};

	glCreateBuffers(1, &m_vbo);
	glNamedBufferStorage(m_vbo, sizeof(points), points, NULL);

	glCreateVertexArrays(1, &m_vao);
	glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, 3 * sizeof(GLfloat));
	glEnableVertexArrayAttrib(m_vao, 0);
	glVertexArrayAttribBinding(m_vao, 0, 0);
	glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
}

PostEffect::~PostEffect()
{
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vbo);
}

void PostEffect::draw()
{
	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(m_vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

//-----------------------------------------------------------------------------

MipmapDepthBufferGenerator::MipmapDepthBufferGenerator()
{
	m_shader = ShaderPool::GetShader("GenerateMipmapDepthBuffer");
}

void MipmapDepthBufferGenerator::generate(Framebuffer & framebuffer)
{
	framebuffer.bind();
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
		draw();
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
