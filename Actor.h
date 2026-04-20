#ifndef ACTOR_H
#define ACTOR_H

#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

#include <string>
#include <vector>
#include <limits>
#include <optional>
#include <unordered_set>
#include <map>

#include "lua.hpp"
#include "LuaBridge.h"

#include "glm/glm.hpp"

#include "SDL2_image/SDL_image.h"

#include "TextDB.h"
#include "Component.h"

class ComponentDB;
class TemplateDB;

class Actor {
public:
    Actor();
    static lua_State* lua_state;
    static std::vector<Actor*>* scene_actors;
    static ComponentDB* component_db;
    static TemplateDB* template_db;
    static int n;
    static class SceneDB* scene_db;
    
    int actor_id = 0;
    std::string actor_name = "";
    std::map<std::string, Component> components_by_key;
    std::vector<std::string> comp_to_remove;
    std::vector<Component> comp_to_add;
    
    bool pending_destroy = false;
    bool dont_destroy = false;
    
    std::string GetName();
    int GetID();
    
    // self.actor:
    luabridge::LuaRef GetComponentByKey(const std::string& key);
    luabridge::LuaRef GetComponent(const std::string& type_name);
    luabridge::LuaRef GetComponents(const std::string& type_name);
    luabridge::LuaRef AddComponent(std::string type_name);
    void RemoveComponent(luabridge::LuaRef component_ref);
    
    // Actor.
    static luabridge::LuaRef Find(std::string name);
    static luabridge::LuaRef FindAll(std::string name);
    static luabridge::LuaRef Instantiate(std::string actor_template_name);
    static void Destroy(Actor* actor);
    
    // helpers
    void ProcessComponentRemovals();
    void ProcessComponentAdds();
};

#endif
