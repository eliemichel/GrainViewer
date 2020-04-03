#pragma once

#include "Behavior/PointCloudSplitter.h" // for RenderModel

#include <memory>
#include <string>

class Behavior;
class RuntimeObject;
class IPointCloudData;

class BehaviorRegistry {
public:
	/**
	 * Add a behavior to a RuntimeObject shared pointer
	 * TODO: Find a way to avoid this function
	 */
	static void addBehavior(std::shared_ptr<Behavior> & b, std::shared_ptr<RuntimeObject> & obj, const std::string & type);

    /**
     * TODO: Our component system does not play along well with inheritance, hence
     * this registry that emulates something that could be something like
     * getComponent<IPointCloudData>()
     */
    static std::weak_ptr<IPointCloudData> getPointCloudDataComponent(
        Behavior& behavior,
        PointCloudSplitter::RenderModel preferedModel = PointCloudSplitter::RenderModel::None);
};
