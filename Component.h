#ifndef COMPONENT_H
#define COMPONENT_H

#include "lua.hpp"
#include "LuaBridge.h"

#include <string>
#include <memory>

class Component {
public:
    bool enabled = true;
    bool started = false;
    bool removed = false;
    
    std::shared_ptr<luabridge::LuaRef> componentRef;
    std::string key;
    std::string type;

    bool hasOnStart = false;
    bool hasOnUpdate = false;
    bool hasOnLateUpdate = false;
    
    bool isEnabled();
    
    
};

#endif
