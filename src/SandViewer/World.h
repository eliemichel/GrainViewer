#pragma once

#include <glad/glad.h>
#include <vector>
#include <memory>

#include <rapidjson/document.h>

#include "Camera.h"

class Light;
class ShaderProgram;

/**
 * Contains all lighting information for a render
 */
class World {
public:
	World();
	bool deserialize(const rapidjson::Value & json);
	void start();
	void reloadShaders();
	void render(const Camera & camera) const;

	const std::vector<std::shared_ptr<Light>> & lights() const { return m_lights; }

	void clear();

private:
	void initVao();

private:
	std::string m_shaderName = "World";
	std::vector<std::shared_ptr<Light>> m_lights;
	std::shared_ptr<ShaderProgram> m_shader;
	GLuint m_vbo; // TODO: use GlBuffer here!
	GLuint m_vao;
};
