/**
 * This file is part of GrainViewer
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#pragma once

#include <memory>

#include <rapidjson/document.h> // rapidjson::Value

#include "RenderType.h"
#include "Camera.h"
#include "World.h"
#include "EnvironmentVariables.h"
#include "IBehaviorHolder.h"
#include "SerializationType.h"

class AnimationManager;

template <typename T> struct BehaviorRegistryEntry { static const char* Name() { return "object"; } };
#define registerBehaviorType(T) \
registerSerializationType(T) \
template<> struct BehaviorRegistryEntry<T> { static const char *Name() { return #T; }};

/**
 * Behaviors are attached to objects, kind of following an entity-component design
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
	 * Called just before rendering.
	 */
	virtual void onPreRender(const Camera& camera, const World& world, RenderType target) {}

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

