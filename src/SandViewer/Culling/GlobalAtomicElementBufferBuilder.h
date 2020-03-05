#pragma once

#include <memory>
#include "AbstractElementBufferBuilder.h"

class ShaderProgram;

typedef std::shared_ptr<ShaderProgram> ShaderPtr;

class GlobalAtomicElementBufferBuilder : public AbstractElementBufferBuilder {
public:
	bool load(const Settings & settings) override;
	void build() override;
	const GlBuffer & elementBuffer() override { return m_elementBuffer; }

private:
	ShaderPtr m_shader;
	GlBuffer m_renderTypeSsbo = GL_SHADER_STORAGE_BUFFER;
	GlBuffer m_countersSsbo = GL_SHADER_STORAGE_BUFFER;
	GlBuffer m_elementBuffer = GL_SHADER_STORAGE_BUFFER;
	int m_xWorkGroups;
};
