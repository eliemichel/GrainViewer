#pragma once

#include <memory>

#include <rapidjson/document.h> // rapidjson::Value

#include "RenderType.h"
#include "Camera.h"
#include "World.h"
#include "IBehaviorHolder.h"
#include "utils/tplutils.h" // for ENABLE_TYPENAME

#define registerBehaviorType(T) ENABLE_TYPENAME(T)

/**
 * There is a little mix of "Behavior" and "Component" to mean the same thing. TODO: clean that
 * IMPORTANT: Call registerBehaviorType(MyBehavior) macro for each new behavior
 */
class Behavior {
public:
	template<typename T>
	std::weak_ptr<T> getComponent() {
		auto parent = m_parent.lock();
		if (!parent) {
			return std::weak_ptr<T>();
		}
		else {
			return parent->getBehavior<T>();
		}
	}

	~Behavior() { onDestroy(); }

	void setEnabled(bool value = true) { m_enabled = value; }
	bool isEnabled() const { return m_enabled; }

public:
	virtual bool deserialize(const rapidjson::Value & json) { return true; }
	virtual void start() {}
	virtual void onDestroy() {}
	virtual void update(float time) {}
	virtual void render(const Camera & camera, const World & world, RenderType target) const {}
	virtual void reloadShaders() {} // TODO: shader loading and reloading should be handled by some Shader Pool object

private:
	friend class IBehaviorHolder;
	void setParent(const std::shared_ptr<IBehaviorHolder> & parent) { m_parent = parent; }

private:
	std::weak_ptr<IBehaviorHolder> m_parent;
	bool m_enabled = true;
};

