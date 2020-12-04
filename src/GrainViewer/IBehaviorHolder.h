/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
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

#include "Logger.h"
#include "BehaviorRegistryEntry.h"

#include <memory>
#include <map>
#include <string>

class Behavior;

/**
 * Interface for objects holding Behavior components.
 * Might not be the best design, maybe this should be merged into RuntimeObject.
 */
class IBehaviorHolder {
public:
	template<typename BehaviorType, typename BehaviorHolderType>
	static std::shared_ptr<BehaviorType> addBehavior(std::shared_ptr<BehaviorHolderType> & parent) {
		auto iparent = std::static_pointer_cast<IBehaviorHolder>(parent);
		auto behavior = std::make_shared<BehaviorType>();

		behavior->setParent(iparent);

		const char *type = BehaviorRegistryEntry<BehaviorType>::Name();
		auto pair = std::pair(type, behavior);
		iparent->m_behaviors.push_back(pair);
		iparent->m_behaviorsMap.insert(pair);
		return behavior;
	}

	template<typename BehaviorType>
	std::weak_ptr<BehaviorType> getBehavior() {
		const char *type = BehaviorRegistryEntry<BehaviorType>::Name();
		std::string key = type;
		auto result = m_behaviorsMap.equal_range(key);
		auto count = std::distance(result.first, result.second);
		if (count == 0) {
			return std::weak_ptr<BehaviorType>();
		} else {
			if (count > 1) {
				WARN_LOG << "This call to getComponent() is ambiguous, prefere getAllComponents() here";
			}
			return std::static_pointer_cast<BehaviorType>(result.first->second);
		}
	}

public:
	using BehaviorIterator = std::vector<std::pair<std::string, std::shared_ptr<Behavior>>>::iterator;
	using ConstBehaviorIterator = std::vector<std::pair<std::string, std::shared_ptr<Behavior>>>::const_iterator;
	BehaviorIterator beginBehaviors() { return m_behaviors.begin(); }
	ConstBehaviorIterator cbeginBehaviors() const { return m_behaviors.cbegin(); }
	BehaviorIterator endBehaviors() { return m_behaviors.end(); }
	ConstBehaviorIterator cendBehaviors() const { return m_behaviors.cend(); }

private:
	// intentional redundancy for quicker access, even though it is a contestable choice
	std::vector<std::pair<std::string,std::shared_ptr<Behavior>>> m_behaviors; // ordered
	std::multimap<std::string,std::shared_ptr<Behavior>> m_behaviorsMap; // faster for search
};


