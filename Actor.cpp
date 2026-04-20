#include <iostream>
#include "Actor.h"
#include "ComponentDB.h"
#include "TemplateDB.h"

lua_State* Actor::lua_state = nullptr;
std::vector<Actor*>* Actor::scene_actors = nullptr;
ComponentDB* Actor::component_db = nullptr;
TemplateDB* Actor::template_db = nullptr;
int Actor::n = 0;
SceneDB* Actor::scene_db = nullptr;

// helpers

void Actor::ProcessComponentRemovals() {
    for (auto& key : comp_to_remove) {
        components_by_key.erase(key);
    }
    comp_to_remove.clear();
}

void Actor::ProcessComponentAdds() {
    for (auto& c : comp_to_add) {
        components_by_key[c.key] = std::move(c);
    }
    comp_to_add.clear();
}

Actor::Actor() {}

std::string Actor::GetName() {
    if (pending_destroy) return luabridge::LuaRef(lua_state);
    return actor_name;
}

int Actor::GetID() {
    return actor_id;
}

luabridge::LuaRef Actor::GetComponentByKey(const std::string& key) {
    if (pending_destroy) return luabridge::LuaRef(lua_state);
    auto it = components_by_key.find(key);
    if (it != components_by_key.end() && !it->second.removed) {
        return *it->second.componentRef;
    }
    else return luabridge::LuaRef(lua_state); // nil
}

luabridge::LuaRef Actor::GetComponent(const std::string& type_name) {
    if (pending_destroy) return luabridge::LuaRef(lua_state);
    for (auto& [key, comp] : components_by_key) {
        if (comp.removed) continue;
        if (comp.type == type_name) return *comp.componentRef;
    }
    return luabridge::LuaRef(lua_state); // nil
}

luabridge::LuaRef Actor::GetComponents(const std::string& type_name) {
    if (pending_destroy) return luabridge::LuaRef(lua_state);
    luabridge::LuaRef new_table = luabridge::newTable(lua_state);
    int i = 1;
    for (auto& [key, comp] : components_by_key) {
        if (comp.removed) continue;
        if (comp.type == type_name) {
            new_table[i] = *comp.componentRef;
            ++i;
        }
    }
    return new_table;
}

luabridge::LuaRef Actor::Find(std::string name) {
    for (auto& actor : *scene_actors) {
        if (actor->pending_destroy) continue;
        if (actor->actor_name == name) {
            return luabridge::LuaRef(lua_state, actor);
        }
    }
    return luabridge::LuaRef(lua_state); // nil
}

luabridge::LuaRef Actor::FindAll(std::string name) {
    luabridge::LuaRef new_table = luabridge::newTable(lua_state);
    int i = 1;
    for (auto& actor : *scene_actors) {
        if (actor->pending_destroy) continue;
        if (actor->actor_name == name) {
            new_table[i] = luabridge::LuaRef(lua_state, actor);
            ++i;
        }
    }
    return new_table;
}

luabridge::LuaRef Actor::AddComponent(std::string type_name) {
    if (pending_destroy) return luabridge::LuaRef(lua_state);
    std::string key = "r" + std::to_string(n);
    n++;

    Component c;
    if (type_name == "Rigidbody") {
        c = component_db->createCppComponentInstance(key, type_name, this);
    } else {
        c = component_db->createComponentInstance(key, type_name, this);
    }
    comp_to_add.push_back(std::move(c));

    return *(comp_to_add.back().componentRef);
}

void Actor::RemoveComponent(luabridge::LuaRef component_ref) {
    if (pending_destroy) return;
    component_ref["enabled"] = false;
    for (auto& [key, comp] : components_by_key) {
        if (*comp.componentRef == component_ref) {
            // call OnDestroy
            if (comp.type == "Rigidbody") {
                try {
                    Rigidbody* rb = comp.componentRef->cast<Rigidbody*>();
                    if (rb) rb->OnDestroy();
                } catch (...) {}
            } else {
                luabridge::LuaRef& self = *comp.componentRef;
                if (self["OnDestroy"].isFunction()) {
                    try {
                        self["OnDestroy"](self);
                    } catch (...) {}
                }
            }
            comp_to_remove.push_back(key);
            comp.removed = true;
            break;
        }
    }
}

luabridge::LuaRef Actor::Instantiate(std::string actor_template_name) {
    Actor* actor = new Actor();
    scene_actors->push_back(actor);
    
    TemplateActorData t_data = template_db->Get(actor_template_name);
    actor->actor_name = t_data.actor_name;
    
    for (auto& [key, temp_comp] : t_data.components_by_key) {
        std::string comp_key = key;
        std::string comp_type = temp_comp.type;
        Component c;
        if (comp_type == "Rigidbody" || comp_type == "ParticleSystem") {
            c = component_db->createCppComponentInstance(comp_key, comp_type, actor);
            component_db->ApplyPropertiesToCppComponent(c, temp_comp);
        } else {
            c = component_db->createComponentInstance(comp_key, comp_type, actor);
            component_db->ApplyPropertiesToComponent(c, temp_comp);
        }
        actor->components_by_key[comp_key] = std::move(c);
    }
    
    return luabridge::LuaRef(lua_state, actor);
}

void Actor::Destroy(Actor* actor) {
    for (auto& [key, comp] : actor->components_by_key) {
        (*comp.componentRef)["enabled"] = false;
    }
    
    if (scene_db) scene_db->CallOnDestroyForActor(actor);
    actor->pending_destroy = true;
}

