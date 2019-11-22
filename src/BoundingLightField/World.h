#pragma once

#include "Camera.h"

class World {
public:
	World();
	void reloadShaders();
	void render(const Camera & camera) const;
};
