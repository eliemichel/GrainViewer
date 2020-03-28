#pragma once

#include <memory>
#include <string>
#include <functional>

#include "IBehaviorHolder.h"

class Behavior;
class Dialog;
class RuntimeObject;

// From https://dxuuu.xyz/cpp-static-registration.html
class BehaviorRegistry {
public:
    typedef std::function<Behavior* ()> BehaviorFactoryFunction;
    typedef std::function<Dialog* ()> DialogFactoryFunction;
    struct Entry {
        BehaviorFactoryFunction behaviorFactory;
        DialogFactoryFunction dialogFactory;
    };
    typedef std::unordered_map<std::string, Entry> EntryMap;

    static bool add(const std::string& name, BehaviorFactoryFunction bfac, DialogFactoryFunction dfac = nullptr) {
        auto map = getEntryMap();
        if (map.find(name) != map.end()) {
            Entry & current = map[name];
            if (current.behaviorFactory == nullptr) current.behaviorFactory = bfac;
            if (current.dialogFactory == nullptr) current.dialogFactory = dfac;
        }
        else {
            map[name] = Entry{ bfac, dfac };
        }
        return true;
    }

    static Behavior* create(const std::string& name) {
        auto map = getEntryMap();
        if (map.find(name) == map.end()) {
            return nullptr;
        }
        return map[name].behaviorFactory();
    }

    static std::shared_ptr<Behavior> addBehavior(std::shared_ptr<RuntimeObject>& obj, const std::string& type);

private:
    // Use Meyer's singleton to prevent SIOF
    static EntryMap& getEntryMap() {
        static EntryMap map;
        return map;
    }
};
