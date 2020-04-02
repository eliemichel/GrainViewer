#pragma once

#include <string>
#include "IBehaviorHolder.h"
#include "Camera.h"
#include "World.h"
#include "RenderType.h"

class RuntimeObject : public IBehaviorHolder {
public:
	RuntimeObject();

	void start();
	void reloadShaders();
	void update(float time, int frame);
	void render(const Camera & camera, const World & world, RenderType target) const;
	void onPreRender(const Camera& camera, const World& world, RenderType target);
	void onPostRender(float time, int frame);

	std::string name;
};
