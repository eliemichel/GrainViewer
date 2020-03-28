#pragma once

#include <memory>

#include "Behavior.h"
#include "ShaderProgram.h"
#include "GlBuffer.h"

class ShaderProgram;

class LightGizmo : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value& json) override;
	void start() override;
	void render(const Camera& camera, const World& world, RenderType target) const override;

private:
	std::string m_shaderName = "LightGizmo";
	int m_lightIndex = 0;
	std::shared_ptr<ShaderProgram> m_shader;
	std::unique_ptr<GlBuffer> m_vertexBuffer;
	GLuint m_vao;
};

