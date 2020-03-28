#pragma once

#include <memory>

#include <rapidjson/document.h> // rapidjson::Value

#include "RenderType.h"
#include "Camera.h"
#include "World.h"
#include "EnvironmentVariables.h"
#include "IBehaviorHolder.h"
#include "utils/tplutils.h" // for ENABLE_TYPENAME

class AnimationManager;

#define registerBehaviorType(T) ENABLE_TYPENAME(T)

/**
 * There is a little mix of "Behavior" and "Component" to mean the same thing. TODO: clean that
 * IMPORTANT: Call registerBehaviorType(MyBehavior) macro for each new behavior
 * Also, you must add it to a Scene_load function for loading to work
 */
class Behavior {
public:
	template<typename T>
	std::weak_ptr<T> getComponent() {
		auto parent = m_parent.lock();
		return parent ? parent->getBehavior<T>() : std::weak_ptr<T>();
	}

	~Behavior() { onDestroy(); }

	void setEnabled(bool value = true) { m_enabled = value; }
	bool isEnabled() const { return m_enabled; }

public:
	/**
	 * Load component data from json file. Do NOT call OpenGL functions here, as this must be thread safe
	 */
	virtual bool deserialize(const rapidjson::Value & json) { return true; }
	virtual bool deserialize(const rapidjson::Value & json, const EnvironmentVariables & env, std::shared_ptr<AnimationManager> animations) { return deserialize(json); }

	/**
	 * Called on main loop start
	 */
	virtual void start() {}

	/**
	 * Call before destroying.
	 */
	virtual void onDestroy() {}

	/**
	 * Called at every frame, can alter this object
	 * Precondition: start() has been called before, and onDestroy has not been called yet.
	 */
	virtual void update(float time) {}
	virtual void update(float time, int frame) { update(time); }
	/**
	 * Called for rendering, cannot change this object data
	 * Precondition: start() has been called before, and onDestroy has not been called yet.
	 */
	virtual void render(const Camera & camera, const World & world, RenderType target) const {}

	/**
	 * Called after render, at the time of the next frame, but before frame
	 * number and animations have been incremented.
	 */
	virtual void onPostRender(float time, int frame) {}

	/**
	 * Called when it is needed to reload shaders.
	 * TODO: shader loading and reloading should be handled by some Shader Pool object
	 */
	virtual void reloadShaders() {}

private:
	friend class IBehaviorHolder;
	void setParent(const std::shared_ptr<IBehaviorHolder> & parent) { m_parent = parent; }

private:
	std::weak_ptr<IBehaviorHolder> m_parent;
	bool m_enabled = true;
};

