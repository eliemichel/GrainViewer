#pragma once

#include <glad/glad.h>
#include <vector>
#include <memory>

#include <rapidjson/document.h>

#include "Camera.h"

class Light;
class ShaderProgram;
class RuntimeObject;

/**
 * Contains all lighting information for a render
 */
class World {
public:
	World();
	bool deserialize(const rapidjson::Value & json);
	void start();
	void update(float time);
	void reloadShaders();
	void render(const Camera & camera) const;
	void renderShadowMaps(const Camera & camera, const std::vector<std::shared_ptr<RuntimeObject>> & objects) const;

	const std::vector<std::shared_ptr<Light>> & lights() const { return m_lights; }

	void clear();

	bool isShadowMapEnabled() const { return m_isShadowMapEnabled; }
	void setShadowMapEnabled(bool value) { m_isShadowMapEnabled = value; }

private:
	void initVao();

private:
	std::string m_shaderName = "World";
	std::vector<std::shared_ptr<Light>> m_lights;
	std::shared_ptr<ShaderProgram> m_shader;
	GLuint m_vbo; // TODO: use GlBuffer here!
	GLuint m_vao;
	bool m_isShadowMapEnabled = true;
};
