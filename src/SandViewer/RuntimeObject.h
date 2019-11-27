#pragma once

#include "IBehaviorHolder.h"
#include "Camera.h"
#include "World.h"
#include "RenderType.h"

class RuntimeObject : public IBehaviorHolder {
public:
	RuntimeObject();

	void start();
	void reloadShaders();
	void update(float time);
	void render(const Camera & camera, const World & world, RenderType target) const;
};
