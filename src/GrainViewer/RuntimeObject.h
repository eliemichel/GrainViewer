#pragma once

#include "IBehaviorHolder.h"
#include "Camera.h"
#include "World.h"
#include "RenderType.h"
#include "ViewLayerMask.h"

#include <rapidjson/document.h>
#include <string>

class RuntimeObject : public IBehaviorHolder {
public:
	RuntimeObject();

	void start();
	void reloadShaders();
	void update(float time, int frame);
	void render(const Camera & camera, const World & world, RenderType target) const;
	void onPreRender(const Camera& camera, const World& world, RenderType target);
	void onPostRender(float time, int frame);

	bool deserialize(const rapidjson::Value& json);

	std::string name;
	ViewLayerMask viewLayers;
};
