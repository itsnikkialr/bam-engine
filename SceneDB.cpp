#include "SceneDB.h"

namespace fs = std::filesystem;

// helpers
static uint64_t XYInt(int x, int y) {
	uint32_t new_x = static_cast<uint32_t>(x);
	uint32_t new_y = static_cast<uint32_t>(y);

	uint64_t result = static_cast<uint64_t>(new_x);
	result = result << 32;
	result = result | static_cast<uint64_t>(new_y);
	return result;
}

static float GetFloatOr(const rapidjson::Value& obj, const char* key, float def) {
	if (!obj.HasMember(key)) return def;
	return obj[key].GetFloat();
}

static int GetIntOr(const rapidjson::Value& obj, const char* key, int def) {
	if (!obj.HasMember(key) || !obj[key].IsInt()) return def;
	return obj[key].GetInt();
}

std::optional<std::string> GetOptTemplateName(const rapidjson::Value& obj) {
    auto it = obj.FindMember("template");
    if (it == obj.MemberEnd() || !it->value.IsString()) return std::nullopt;
    return std::string(it->value.GetString());
}

static std::string GetStringOrEmpty(const rapidjson::Value& obj, const char* key) {
	if (!obj.HasMember(key) || !obj[key].IsString()) return "";
	return obj[key].GetString();
}

void SceneDB::ReportError(const std::string& actor_name, const luabridge::LuaException& e) {
    std::string error_message = e.what();
    std::replace(error_message.begin(), error_message.end(), '\\', '/');
    std::cout << "\033[31m" << actor_name << " : " << error_message << "\033[0m" << std::endl;
}

void SceneDB::CheckAndLoadPendingScene() {
    if (!scene_load_pending) return;
    scene_load_pending = false;
    
    current_scene_name = pending_scene_name;
    std::string scene_path = "resources/scenes/" + pending_scene_name + ".scene";
    pending_scene_name = "";
    
    instance->LoadSceneFile(scene_path, *renderer_ref, *image_db_ref, *template_db_ref, *comp_db_ref);
}

SceneDB::SceneDB() {
    Actor::scene_actors = &actors;
    instance = this;
}

void SceneDB::SetRefs(SDL_Renderer*& r, ImageDB& img, TemplateDB& t, ComponentDB& c) {
    renderer_ref = &r;
    image_db_ref = &img;
    template_db_ref = &t;
    comp_db_ref = &c;
}

void SceneDB::Load(const std::string& scene_name) {
    pending_scene_name = scene_name;
    scene_load_pending = true;
}

std::string SceneDB::GetCurrent() {
    return current_scene_name;
}

void SceneDB::DontDestroy(Actor* actor) {
    actor->dont_destroy = true;
}

void SceneDB::LoadSceneFile(const std::string& scene_path, SDL_Renderer*& sdl_renderer, ImageDB& image_db, TemplateDB& template_db, ComponentDB& comp_db) {
    if (!fs::exists(scene_path)) {
        exit(0);
    }

    // keep preserved, remove rest from vector (no delete)
    std::vector<Actor*> preserved;
    for (Actor* a : actors) {
        if (a->dont_destroy) {
            preserved.push_back(a);
        }
        else {
            CallOnDestroyForActor(a);
        }
    }
    
    
    actors.clear();
    for (Actor* a : preserved) {
        actors.push_back(a);
    }

    rapidjson::Document doc;
    EngineUtils::ReadJsonFile(scene_path, doc);
    image_db.LoadActorImages(sdl_renderer, doc, "actors");
    LoadActorsFromDoc(doc, sdl_renderer, image_db, template_db, comp_db);
}

void SceneDB::LoadActorsFromDoc(const rapidjson::Document& doc,
                                SDL_Renderer*& sdl_renderer,
                                ImageDB& image_db,
                                TemplateDB& template_db,
                                ComponentDB& comp_db) {
    if (!doc.HasMember("actors") || !doc["actors"].IsArray()) {
        return;
    }

    const auto& arr = doc["actors"].GetArray();
    actors.reserve(arr.Size());

    for (const auto& a : arr) {
        if (!a.IsObject()) continue;

        Actor* actor = new Actor();
        actors.push_back(actor);

        // 1. inherit template defaults first
        if (auto tname = GetOptTemplateName(a)) {
            if (!template_db.Has(*tname)) {
                std::cout << "error: template " << *tname << " is missing";
                exit(0);
            }

            const TemplateActorData& temp = template_db.Get(*tname);

            actor->actor_name = temp.actor_name;

            for (const auto& [key, temp_comp] : temp.components_by_key) {
                std::string comp_key = key;
                std::string comp_type = temp_comp.type;

                Component c;
                if (comp_type == "Rigidbody" || comp_type == "ParticleSystem") {
                    c = comp_db.createCppComponentInstance(comp_key, comp_type, actor);
                    comp_db.ApplyPropertiesToCppComponent(c, temp_comp);
                }
                else {
                    c = comp_db.createComponentInstance(comp_key, comp_type, actor);
                    comp_db.ApplyPropertiesToComponent(c, temp_comp);
                }
                actor->components_by_key[comp_key] = std::move(c);
            }
        }

        // 2. actor-level basic overrides
        if (a.HasMember("name") && a["name"].IsString()) {
            actor->actor_name = a["name"].GetString();
        }

        // 3. actor-level component overrides / additions
        if (a.HasMember("components") && a["components"].IsObject()) {
            const auto& components = a["components"];

            for (auto it = components.MemberBegin(); it != components.MemberEnd(); ++it) {
                std::string key = it->name.GetString();
                const auto& comp_obj = it->value;

                if (!comp_obj.IsObject()) continue;

                auto found = actor->components_by_key.find(key);

                if (found != actor->components_by_key.end()) {
                    // inherited component override
                    Component& c = found->second;

                    // optional: if scene explicitly provides a type, rebuild if different
                    if (comp_obj.HasMember("type") && comp_obj["type"].IsString()) {
                        std::string new_type = comp_obj["type"].GetString();
                        if (new_type != c.type) {
                            Component rebuilt;
                            if (new_type == "Rigidbody" || new_type == "ParticleSystem") {
                                rebuilt = comp_db.createCppComponentInstance(key, new_type, actor);
                                comp_db.ApplyPropertiesToCppComponent(rebuilt, comp_obj);
                            }
                            else {
                                rebuilt = comp_db.createComponentInstance(key, new_type, actor);
                                comp_db.ApplyPropertiesToComponent(rebuilt, comp_obj);
                            }
                            actor->components_by_key[key] = std::move(rebuilt);
                        }
                        else {
                            comp_db.ApplyPropertiesToComponent(c, comp_obj);
                        }
                    }
                    else {
                        comp_db.ApplyPropertiesToComponent(c, comp_obj);
                    }
                }
                else {
                    // brand new component
                    if (!comp_obj.HasMember("type") || !comp_obj["type"].IsString()) {
                        continue;
                    }

                    std::string type = comp_obj["type"].GetString();
                    Component c;
                    if (type == "Rigidbody" || type == "ParticleSystem") {
                        c = comp_db.createCppComponentInstance(key, type, actor);
                        comp_db.ApplyPropertiesToCppComponent(c, comp_obj);
                    }
                    else {
                        c = comp_db.createComponentInstance(key, type, actor);
                        comp_db.ApplyPropertiesToComponent(c, comp_obj);
                    }
                    actor->components_by_key[key] = std::move(c);
                }
            }
        }
    }
}

void SceneDB::RunComponentOnStarts() {
    for (size_t i = 0; i < lifecycle_actor_count; i++) {
        Actor* actor = actors[i];
        if (!actor) continue;
        if (actor->pending_destroy) continue;
        for (auto& [key, comp] : actor->components_by_key) {
            if (!comp.isEnabled()) {
                continue;
            }
            if (comp.started) continue;
            
            if (comp.type == "Rigidbody") {
                try {
                    Rigidbody* rb = comp.componentRef->cast<Rigidbody*>();
                    if (rb && !rb->started) {
                        rb->OnStart();
                    }
                }
                catch (...) {
                }
            }
            
            if (comp.type == "ParticleSystem") {
                try {
                    ParticleSystem* ps = comp.componentRef->cast<ParticleSystem*>();
                    if (ps && !ps->started) {
                        ps->OnStart();
                    }
                } catch (...) {}
            }

            if (comp.hasOnStart) {
                try {
                    luabridge::LuaRef& self = *comp.componentRef;
                    self["OnStart"](self);
                }
                catch (const luabridge::LuaException& e) {
                    ReportError(actor->actor_name, e);
                }
            }

            comp.started = true;
        }
    }
}

void SceneDB::RunOnUpdates() {
    for (size_t i = 0; i < lifecycle_actor_count; i++) {
        Actor* actor = actors[i];
        if (!actor) continue;
        if (actor->pending_destroy) continue;
        for (auto& [key, comp] : actor->components_by_key) {
            if (!comp.isEnabled()) continue;
            if (comp.hasOnUpdate) {
                try {
                    luabridge::LuaRef& self = *comp.componentRef;
                    self["OnUpdate"](self);
                }
                catch (const luabridge::LuaException& e) {
                    ReportError(actor->actor_name, e);
                }
            }
            if (comp.type == "ParticleSystem") {
                try {
                    ParticleSystem* ps = comp.componentRef->cast<ParticleSystem*>();
                    if (ps && ps->enabled) {
                        ps->OnUpdate();
                    }
                } catch (...) {}
            }
        }
    }
}

void SceneDB::RunOnLateUpdates() {
    for (size_t i = 0; i < lifecycle_actor_count; i++) {
        Actor* actor = actors[i];
        if (!actor) continue;
        if (actor->pending_destroy) continue;
        if (actor->pending_destroy) continue;
        for (auto& [key, comp] : actor->components_by_key) {
            if (!comp.isEnabled()) continue;
            if (comp.hasOnLateUpdate) {
                try {
                    luabridge::LuaRef& self = *comp.componentRef;
                    self["OnLateUpdate"](self);
                }
                catch (const luabridge::LuaException& e) {
                    ReportError(actor->actor_name, e);
                }
            }
        }
    }
}

void SceneDB::ProcessPendingChanges() {
    for (Actor* actor : actors) {
        actor->ProcessComponentAdds();
        actor->ProcessComponentRemovals();
    }
    
    // remove from vector but DON'T delete
    auto it = std::remove_if(actors.begin(), actors.end(),
        [](Actor* a) { return a->pending_destroy; });
    actors.erase(it, actors.end());
}

void SceneDB::ClearScene() {
    for (Actor* a : actors) delete a;
	actors.clear();
}

void SceneDB::SetLifecycleCount() {
    lifecycle_actor_count = actors.size();
}

void SceneDB::StepPhysics() {
    b2World* world = Rigidbody::GetWorld();
    if (world) {
        world->Step(1.0f / 60.0f, 8, 3);
    }
}

void SceneDB::ProcessCollisionEvents() {
    for (auto& event : ContactListener::events) {
        Collision collisionA;
        collisionA.other = event.actorB;
        collisionA.point = event.point;
        collisionA.relative_velocity = event.relative_velocity;
        collisionA.normal = event.normal;

        Collision collisionB;
        collisionB.other = event.actorA;
        collisionB.point = event.point;
        collisionB.relative_velocity = event.relative_velocity;
        collisionB.normal = event.normal;

        std::string func_name;
        if (event.is_trigger) {
            func_name = event.is_begin ? "OnTriggerEnter" : "OnTriggerExit";
        } else {
            func_name = event.is_begin ? "OnCollisionEnter" : "OnCollisionExit";
        }

        // Actor A's components first
        for (auto& [key, comp] : event.actorA->components_by_key) {
            if (!comp.isEnabled()) continue;
            luabridge::LuaRef& self = *comp.componentRef;
            if (self[func_name].isFunction()) {
                try {
                    self[func_name](self, collisionA);
                } catch (const luabridge::LuaException& e) {
                    ReportError(event.actorA->actor_name, e);
                }
            }
        }

        // Actor B's components second
        for (auto& [key, comp] : event.actorB->components_by_key) {
            if (!comp.isEnabled()) continue;
            luabridge::LuaRef& self = *comp.componentRef;
            if (self[func_name].isFunction()) {
                try {
                    self[func_name](self, collisionB);
                } catch (const luabridge::LuaException& e) {
                    ReportError(event.actorB->actor_name, e);
                }
            }
        }
    }
    ContactListener::events.clear();
}

void SceneDB::CallOnDestroyForActor(Actor* actor) {
    for (auto& [key, comp] : actor->components_by_key) {
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
                } catch (const luabridge::LuaException& e) {
                    ReportError(actor->actor_name, e);
                }
            }
        }
    }
}
