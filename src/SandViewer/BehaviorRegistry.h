#pragma once

#include <memory>
#include <string>

class Behavior;
class RuntimeObject;

class BehaviorRegistry {
public:
	/**
	 * Add a behavior to a RuntimeObject shared pointer
	 * TODO: Find a way to avoid this function
	 */
	static void addBehavior(std::shared_ptr<Behavior> & b, std::shared_ptr<RuntimeObject> & obj, const std::string & type);
};
