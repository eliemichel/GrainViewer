#include "BehaviorRegistry.h"
#include "Behavior.h"
#include "RuntimeObject.h"

std::shared_ptr<Behavior> BehaviorRegistry::addBehavior(std::shared_ptr<RuntimeObject>& obj, const std::string& type)
{
	std::shared_ptr<Behavior> behavior = std::shared_ptr<Behavior>(create(type));
	auto iparent = std::static_pointer_cast<IBehaviorHolder>(obj);
	IBehaviorHolder::addBehavior(iparent, behavior, type);
	return behavior;
}
