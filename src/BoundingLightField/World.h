#pragma once

#include "Camera.h"

/**
 * Contains all lighting information for a render
 */
class World {
public:
	World();
	void reloadShaders();
	void render(const Camera & camera) const;
};
