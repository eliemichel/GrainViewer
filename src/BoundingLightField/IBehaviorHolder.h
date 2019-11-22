#pragma once

#include <memory>
#include <map>
#include <string>

#include "utils/tplutils.h"

class Behavior;

/**
* Interface for objects holding Behavior components.
* Might not be the best design, maybe this should be merged into GlObject.
*/
class IBehaviorHolder {
public:
	template<typename BehaviorType, typename BehaviorHolderType>
	static std::shared_ptr<BehaviorType> addBehavior(std::shared_ptr<BehaviorHolderType> & parent) {
		auto iparent = std::static_pointer_cast<IBehaviorHolder>(parent);
		auto behavior = std::make_shared<BehaviorType>();

		behavior->setParent(iparent);

		const char *type = TypeName<BehaviorType>().Get();
		iparent->m_behaviors.insert(std::pair<std::string, std::shared_ptr<Behavior>>(type, behavior));
		return behavior;
	}

	template<typename BehaviorType>
	std::weak_ptr<BehaviorType> getBehavior() {
		const char *type = TypeName<BehaviorType>().Get();
		std::string key = type;
		auto result = m_behaviors.equal_range(key);
		auto count = std::distance(result.first, result.second);
		if (count == 0) {
			return std::weak_ptr<BehaviorType>();
		} else {
			if (count > 1) {
				// warning: call to getComponent() is ambiguous, prefere getAllComponents() here
			}
			return std::static_pointer_cast<BehaviorType>(result.first->second);
		}
	}

public:
	using BehaviorIterator = std::multimap<std::string, std::shared_ptr<Behavior>>::iterator;
	using ConstBehaviorIterator = std::multimap<std::string, std::shared_ptr<Behavior>>::const_iterator;
	BehaviorIterator beginBehaviors() { return m_behaviors.begin(); }
	ConstBehaviorIterator cbeginBehaviors() const { return m_behaviors.cbegin(); }
	BehaviorIterator endBehaviors() { return m_behaviors.end(); }
	ConstBehaviorIterator cendBehaviors() const { return m_behaviors.cend(); }

private:
	std::multimap<std::string,std::shared_ptr<Behavior>> m_behaviors;
};


