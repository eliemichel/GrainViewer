#pragma once

#include <vector>
#include <memory>

#include <rapidjson/document.h>

#include "Camera.h"

class Light;

/**
 * Contains all lighting information for a render
 */
class World {
public:
	World();
	bool deserialize(const rapidjson::Value & json);
	void reloadShaders();
	void render(const Camera & camera) const;

	const std::vector<std::shared_ptr<Light>> & lights() const { return m_lights; }

	void clear();

private:
	std::vector<std::shared_ptr<Light>> m_lights;
};
