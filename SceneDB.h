#ifndef SCENEDB_H
#define SCENEDB_H

#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

#include <vector>
#include <unordered_map>
#include <string>
#include <optional>
#include <filesystem>

#include "lua.hpp"
#include "LuaBridge.h"

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

#include "box2d/box2d.h"

#include "Actor.h"
#include "TemplateDB.h"
#include "ImageDB.h"
#include "Renderer.h"
#include "ComponentDB.h"
#include "Component.h"

#include "Helper.h"
#include "EngineUtils.h"

class SceneDB {
private:
    std::vector<Actor*> actors;
    
    static inline std::string pending_scene_name = "";
    static inline bool scene_load_pending = false;
    
    static inline SceneDB* instance = nullptr;
    static inline SDL_Renderer** renderer_ref = nullptr;
    static inline ImageDB* image_db_ref = nullptr;
    static inline TemplateDB* template_db_ref = nullptr;
    static inline ComponentDB* comp_db_ref = nullptr;

public:
    SceneDB();
    size_t lifecycle_actor_count = 0;
    static inline std::string current_scene_name = "";

    void LoadSceneFile(const std::string& scene_path, SDL_Renderer*& sdl_renderer, ImageDB& image_db, TemplateDB& template_db, ComponentDB& comp_db);
    void LoadActorsFromDoc(const rapidjson::Document& doc, SDL_Renderer*& sdl_renderer, ImageDB& image_db, TemplateDB& template_db, ComponentDB& comp_db);
    
    void SetLifecycleCount();
    void RunComponentOnStarts();
    void RunOnUpdates();
    void RunOnLateUpdates();
    void ClearScene();
    void ProcessPendingChanges();
    
    // Scene API
    static void Load(const std::string& scene_name);
    static std::string GetCurrent();
    static void DontDestroy(Actor* actor);
    void CheckAndLoadPendingScene();
    
    // phsycis
    void StepPhysics();
    
    void ProcessCollisionEvents();
    
    // call once at startup to set refs
    void SetRefs(SDL_Renderer*& r, ImageDB& img, TemplateDB& t, ComponentDB& c);
    
    void CallOnDestroyForActor(Actor* actor);

private:
    void ReportError(const std::string& actor_name, const luabridge::LuaException& e);
};

#endif
