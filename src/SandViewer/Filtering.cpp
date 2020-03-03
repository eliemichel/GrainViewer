#include <algorithm>
#include <cassert>
#include "Logger.h"
#include "Filtering.h"
#include "ShaderPool.h"

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
