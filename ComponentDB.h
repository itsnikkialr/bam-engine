#ifndef COMPONENTDB_H
#define COMPONENTDB_H

#include "AudioHelper.h"

#include "lua.hpp"
#include "LuaBridge.h"

#include "box2d/box2d.h"

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

#include "Component.h"
#include "TemplateDB.h"
#include "Input.h"
#include "AudioDB.h"
#include "ImageDB.h"
#include "SceneDB.h"
#include "Rigidbody.h"
#include "Collision.h"
#include "ContactListener.h"
#include "RaycastCallback.h"
#include "HitResult.h"
#include "EventBus.h"
#include "ParticleSystem.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <thread>
#include <cstdlib>

class Actor;

class ComponentDB {
private:
    lua_State* lua_state = nullptr;

    struct ComponentTypeData {
        std::shared_ptr<luabridge::LuaRef> baseTable;
        bool hasOnStart = false;
        bool hasOnUpdate = false;
        bool hasOnLateUpdate = false;
    };

    std::unordered_map<std::string, ComponentTypeData> component_types;
    
public:
    ComponentDB();
    bool doesComponentExist(const std::string& comp_name);
    void readComponent(std::string& comp_name);
    Component createComponentInstance(std::string& key, std::string& comp_name, Actor* owner);
    void establishInheritance(luabridge::LuaRef& instance_table, luabridge::LuaRef & parent_table);
    void ApplyPropertiesToComponent(Component& c, const rapidjson::Value& comp_obj);
    void ApplyPropertiesToComponent(Component& c, const TemplateComponentData& comp_data);
    Component createCppComponentInstance(std::string& key, std::string& comp_name, Actor* owner);
    void ApplyPropertiesToCppComponent(Component& c, const rapidjson::Value& comp_obj);
    void ApplyPropertiesToCppComponent(Component& c, const TemplateComponentData& comp_data);
};

#endif
