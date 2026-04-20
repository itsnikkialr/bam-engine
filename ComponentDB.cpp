#include "ComponentDB.h"

namespace fs = std::filesystem;

// namespace functions
static void DebugLog(const std::string& message) {
    std::cout << message << std::endl;
}

static void Quit() {
    exit(0);
}

static void Sleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

static int GetFrame() {
    return Helper::GetFrameNumber();
}

static void CameraSetPosition(float x, float y) { Renderer::SetCameraPosition(x, y); }
static float CameraGetPositionX() { return Renderer::cam_x; }
static float CameraGetPositionY() { return Renderer::cam_y; }
static void CameraSetZoom(float z) {
    Renderer::SetZoomFactor(z);
    SDL_RenderSetScale(Renderer::GetSDLRenderer(), z, z);
}
static float CameraGetZoom() { return Renderer::GetCameraZoomFactor(); }

static void OpenURL(std::string url_name) {
#ifdef _WIN32
    std::system(("start " + url_name).c_str());
#elif __APPLE__
    std::system(("open " + url_name).c_str());
#else
    std::system(("xdg-open " + url_name).c_str());
#endif
}

static void TextDraw(const std::string& text, float x, float y,
                     const std::string& font_name, float font_size,
                     float r, float g, float b, float a) {
    TextDB::QueueDraw(text, static_cast<int>(x), static_cast<int>(y),
                      font_name, static_cast<int>(font_size),
                      static_cast<int>(r), static_cast<int>(g),
                      static_cast<int>(b), static_cast<int>(a));
}

static void AudioPlay(int channel, const std::string& clip_name, bool does_loop) {
    // load clip if not cached, then play
    Mix_Chunk* chunk = AudioDB::GetChunk(clip_name);
    AudioHelper::Mix_PlayChannel(channel, chunk, does_loop ? -1 : 0);
}

static void AudioHalt(int channel) {
    AudioHelper::Mix_HaltChannel(channel);
}

static void AudioSetVolume(int channel, float volume) {
    AudioHelper::Mix_Volume(channel, static_cast<int>(volume));
}

static float Vec2_Distance(const b2Vec2& a, const b2Vec2& b) {
    return b2Distance(a, b);
}

// Scene
static void SceneLoad(const std::string& scene_name) { SceneDB::Load(scene_name); }
static std::string SceneGetCurrent() { return SceneDB::GetCurrent(); }
static void SceneDontDestroy(Actor* actor) { SceneDB::DontDestroy(actor); }

static luabridge::LuaRef PhysicsRaycast(b2Vec2 pos, b2Vec2 dir, float dist) {
    lua_State* L = Actor::lua_state;
    
    if (dist <= 0 || !Rigidbody::GetWorld()) {
        return luabridge::LuaRef(L); // nil
    }
    
    dir.Normalize();
    b2Vec2 endPoint = pos + dist * dir;
    
    RaycastCallback callback;
    Rigidbody::GetWorld()->RayCast(&callback, pos, endPoint);
    
    if (callback.hits.empty()) {
        return luabridge::LuaRef(L); // nil
    }
    
    // find closest hit
    HitResult* closest = &callback.hits[0];
    for (auto& hit : callback.hits) {
        if (hit.fraction < closest->fraction) {
            closest = &hit;
        }
    }
    
    return luabridge::LuaRef(L, *closest);
}

static luabridge::LuaRef PhysicsRaycastAll(b2Vec2 pos, b2Vec2 dir, float dist) {
    lua_State* L = Actor::lua_state;
    luabridge::LuaRef table = luabridge::newTable(L);
    
    if (dist <= 0 || !Rigidbody::GetWorld()) {
        return table; // empty table
    }
    
    dir.Normalize();
    b2Vec2 endPoint = pos + dist * dir;
    
    RaycastCallback callback;
    Rigidbody::GetWorld()->RayCast(&callback, pos, endPoint);
    
    // sort by fraction (distance)
    std::sort(callback.hits.begin(), callback.hits.end(),
              [](const HitResult& a, const HitResult& b) {
        return a.fraction < b.fraction;
    });
    
    int i = 1;
    for (auto& hit : callback.hits) {
        table[i] = hit;
        i++;
    }
    
    return table;
}

static void EventPublish(const std::string& event_type, luabridge::LuaRef event_object) {
    EventBus::Publish(event_type, event_object);
}

static void EventSubscribe(const std::string& event_type, luabridge::LuaRef component, luabridge::LuaRef function) {
    EventBus::Subscribe(event_type, component, function);
}

static void EventUnsubscribe(const std::string& event_type, luabridge::LuaRef component, luabridge::LuaRef function) {
    EventBus::Unsubscribe(event_type, component, function);
}


// componentdb class functions
ComponentDB::ComponentDB() {
    lua_state = luaL_newstate();
    luaL_openlibs(lua_state);
    
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Debug")
        .addFunction("Log", &DebugLog)
        .endNamespace();
    
    // Actor class for self.actor
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<Actor>("Actor")
        .addFunction("GetName", &Actor::GetName)
        .addFunction("GetID", &Actor::GetID)
        .addFunction("GetComponent", &Actor::GetComponent)
        .addFunction("GetComponents", &Actor::GetComponents)
        .addFunction("GetComponentByKey", &Actor::GetComponentByKey)
        .addFunction("AddComponent", &Actor::AddComponent)
        .addFunction("RemoveComponent", &Actor::RemoveComponent)
        .endClass();
    
    // Input class
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Input")
        .addFunction("GetKey", &Input::GetKey)
        .addFunction("GetKeyDown", &Input::GetKeyDown)
        .addFunction("GetKeyUp", &Input::GetKeyUp)
        .addFunction("GetMousePosition", &Input::GetMousePosition)
        .addFunction("GetMouseButton", &Input::GetMouseButton)
        .addFunction("GetMouseButtonDown", &Input::GetMouseButtonDown)
        .addFunction("GetMouseButtonUp", &Input::GetMouseButtonUp)
        .addFunction("GetMouseScrollDelta", &Input::GetMouseScrollDelta)
        .addFunction("HideCursor", &Input::HideCursor)
        .addFunction("ShowCursor", &Input::ShowCursor)
        .endNamespace();
    
    // ImageDB class
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Image")
        .addFunction("DrawUI", &ImageDB::DrawUI)
        .addFunction("DrawUIEx", &ImageDB::DrawUIEx)
        .addFunction("Draw", &ImageDB::Draw)
        .addFunction("DrawEx", &ImageDB::DrawEx)
        .addFunction("DrawPixel", &ImageDB::DrawPixel)
        .endNamespace();
    
    // Scene class
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Scene")
        .addFunction("Load", &SceneLoad)
        .addFunction("GetCurrent", &SceneGetCurrent)
        .addFunction("DontDestroy", &SceneDontDestroy)
        .endNamespace();
    
    // camera
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Camera")
        .addFunction("SetPosition", &CameraSetPosition)
        .addFunction("GetPositionX", &CameraGetPositionX)
        .addFunction("GetPositionY", &CameraGetPositionY)
        .addFunction("SetZoom", &CameraSetZoom)
        .addFunction("GetZoom", &CameraGetZoom)
        .endNamespace();
    
    // static means namespace. Actor. stuff
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Actor")
        .addFunction("Find", &Actor::Find)
        .addFunction("FindAll", &Actor::FindAll)
        .addFunction("Instantiate", &Actor::Instantiate)
        .addFunction("Destroy", &Actor::Destroy)
        .endNamespace();
    
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Application")
        .addFunction("Quit", &Quit)
        .addFunction("Sleep", &Sleep)
        .addFunction("GetFrame", &GetFrame)
        .addFunction("OpenURL", &OpenURL)
        .endNamespace();
    
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<glm::vec2>("vec2")
        .addProperty("x", &glm::vec2::x)
        .addProperty("y", &glm::vec2::y)
        .endClass();
    
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Text")
        .addFunction("Draw", &TextDraw)
        .endNamespace();
    
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Audio")
        .addFunction("Play", &AudioPlay)
        .addFunction("Halt", &AudioHalt)
        .addFunction("SetVolume", &AudioSetVolume)
        .endNamespace();
    
    // box2d
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<b2Vec2>("Vector2")
        .addConstructor<void(*) (float, float)>()
        .addProperty("x", &b2Vec2::x)
        .addProperty("y", &b2Vec2::y)
        .addFunction("Normalize", &b2Vec2::Normalize)
        .addFunction("Length", &b2Vec2::Length)
        .addFunction("__add", &b2Vec2::operator_add)
        .addFunction("__sub", &b2Vec2::operator_sub)
        .addFunction("__mul", &b2Vec2::operator_mul)
        .addStaticFunction("Dot", static_cast<float (*)(const b2Vec2&, const b2Vec2&)>(&b2Dot))
        .addStaticFunction("Distance", &Vec2_Distance)
        .endClass();
    
    // rigidbody
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<Rigidbody>("Rigidbody")
        .addConstructor<void(*)()>()
        .addData("x", &Rigidbody::x)
        .addData("y", &Rigidbody::y)
        .addData("body_type", &Rigidbody::body_type)
        .addData("precise", &Rigidbody::precise)
        .addData("gravity_scale", &Rigidbody::gravity_scale)
        .addData("density", &Rigidbody::density)
        .addData("angular_friction", &Rigidbody::angular_friction)
        .addData("rotation", &Rigidbody::rotation)
        .addData("has_collider", &Rigidbody::has_collider)
        .addData("has_trigger", &Rigidbody::has_trigger)
        .addData("enabled", &Rigidbody::enabled)
        .addFunction("GetPosition", &Rigidbody::GetPosition)
        .addFunction("GetRotation", &Rigidbody::GetRotation)
    
        .addFunction("AddForce", &Rigidbody::AddForce)
        .addFunction("SetVelocity", &Rigidbody::SetVelocity)
        .addFunction("SetPosition", &Rigidbody::SetPosition)
        .addFunction("SetRotation", &Rigidbody::SetRotation)
        .addFunction("SetAngularVelocity", &Rigidbody::SetAngularVelocity)
        .addFunction("SetGravityScale", &Rigidbody::SetGravityScale)
        .addFunction("SetUpDirection", &Rigidbody::SetUpDirection)
        .addFunction("SetRightDirection", &Rigidbody::SetRightDirection)
    
        .addFunction("GetPosition", &Rigidbody::GetPosition)
        .addFunction("GetRotation", &Rigidbody::GetRotation)
        .addFunction("GetVelocity", &Rigidbody::GetVelocity)
        .addFunction("GetAngularVelocity", &Rigidbody::GetAngularVelocity)
        .addFunction("GetGravityScale", &Rigidbody::GetGravityScale)
        .addFunction("GetUpDirection", &Rigidbody::GetUpDirection)
        .addFunction("GetRightDirection", &Rigidbody::GetRightDirection)
    
        .addData("collider_type", &Rigidbody::collider_type)
        .addData("width", &Rigidbody::width)
        .addData("height", &Rigidbody::height)
        .addData("radius", &Rigidbody::radius)
        .addData("friction", &Rigidbody::friction)
        .addData("bounciness", &Rigidbody::bounciness)
    
        .addData("trigger_type", &Rigidbody::trigger_type)
        .addData("trigger_width", &Rigidbody::trigger_width)
        .addData("trigger_height", &Rigidbody::trigger_height)
        .addData("trigger_radius", &Rigidbody::trigger_radius)
    
        .endClass();
    
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<Collision>("Collision")
        .addData("other", &Collision::other)
        .addData("point", &Collision::point)
        .addData("relative_velocity", &Collision::relative_velocity)
        .addData("normal", &Collision::normal)
        .endClass();
    
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<HitResult>("HitResult")
        .addData("actor", &HitResult::actor)
        .addData("point", &HitResult::point)
        .addData("normal", &HitResult::normal)
        .addData("is_trigger", &HitResult::is_trigger)
        .endClass();
    
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Physics")
        .addFunction("Raycast", &PhysicsRaycast)
        .addFunction("RaycastAll", &PhysicsRaycastAll)
        .endNamespace();
    
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Event")
        .addFunction("Publish", &EventPublish)
        .addFunction("Subscribe", &EventSubscribe)
        .addFunction("Unsubscribe", &EventUnsubscribe)
        .endNamespace();
    
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<ParticleSystem>("ParticleSystem")
        .addConstructor<void(*)()>()
        .addData("enabled", &ParticleSystem::enabled)
        .addData("x", &ParticleSystem::x)
        .addData("y", &ParticleSystem::y)
        .addData("frames_between_bursts", &ParticleSystem::frames_between_bursts)
        .addData("burst_quantity", &ParticleSystem::burst_quantity)
        .addData("start_scale_min", &ParticleSystem::start_scale_min)
        .addData("start_scale_max", &ParticleSystem::start_scale_max)
        .addData("rotation_min", &ParticleSystem::rotation_min)
        .addData("rotation_max", &ParticleSystem::rotation_max)
        .addData("start_color_r", &ParticleSystem::start_color_r)
        .addData("start_color_g", &ParticleSystem::start_color_g)
        .addData("start_color_b", &ParticleSystem::start_color_b)
        .addData("start_color_a", &ParticleSystem::start_color_a)
        .addData("emit_radius_min", &ParticleSystem::emit_radius_min)
        .addData("emit_radius_max", &ParticleSystem::emit_radius_max)
        .addData("emit_angle_min", &ParticleSystem::emit_angle_min)
        .addData("emit_angle_max", &ParticleSystem::emit_angle_max)
        .addData("image", &ParticleSystem::image)
        .addData("sorting_order", &ParticleSystem::sorting_order)
        .addData("duration_frames", &ParticleSystem::duration_frames)
        .addData("start_speed_min", &ParticleSystem::start_speed_min)
        .addData("start_speed_max", &ParticleSystem::start_speed_max)
        .addData("rotation_speed_min", &ParticleSystem::rotation_speed_min)
        .addData("rotation_speed_max", &ParticleSystem::rotation_speed_max)
        .addData("gravity_scale_x", &ParticleSystem::gravity_scale_x)
        .addData("gravity_scale_y", &ParticleSystem::gravity_scale_y)
        .addData("drag_factor", &ParticleSystem::drag_factor)
        .addData("angular_drag_factor", &ParticleSystem::angular_drag_factor)
        .addData("end_scale", &ParticleSystem::end_scale)
        .addData("end_color_r", &ParticleSystem::end_color_r)
        .addData("end_color_g", &ParticleSystem::end_color_g)
        .addData("end_color_b", &ParticleSystem::end_color_b)
        .addData("end_color_a", &ParticleSystem::end_color_a)
        .addFunction("Play", &ParticleSystem::Play)
        .addFunction("Stop", &ParticleSystem::Stop)
        .addFunction("Burst", &ParticleSystem::Burst)
        .endClass();
    
    Actor::lua_state = lua_state;
}

bool ComponentDB::doesComponentExist(const std::string& comp_name) {
    return component_types.find(comp_name) != component_types.end();
}

void ComponentDB::readComponent(std::string& comp_name) {
    if (doesComponentExist(comp_name)) return;
    
    std::string lua_comp_path = "resources/component_types/" + comp_name + ".lua";
    // if lua file dne
    if (!fs::exists(lua_comp_path)) {
        std::cout << "error: failed to locate component " << comp_name;
        exit(0);
    }
    
    if (luaL_dofile(lua_state, lua_comp_path.c_str()) != LUA_OK) {
        std::cout << "problem with lua file " << fs::path(lua_comp_path).stem().string();
        exit(0);
    }
    
    luabridge::LuaRef base_table = luabridge::getGlobal(lua_state, comp_name.c_str());
    ComponentTypeData data;
    data.baseTable = std::make_shared<luabridge::LuaRef>(base_table);
    data.hasOnStart = (*data.baseTable)["OnStart"].isFunction();
    data.hasOnUpdate = (*data.baseTable)["OnUpdate"].isFunction();
    data.hasOnLateUpdate = (*data.baseTable)["OnLateUpdate"].isFunction();
    
    component_types[comp_name] = std::move(data);
}

Component ComponentDB::createComponentInstance(std::string& key, std::string& comp_name, Actor* owner) {
    readComponent(comp_name);
    
    ComponentTypeData& type_data = component_types[comp_name];
    
    luabridge::LuaRef instance_table = luabridge::newTable(lua_state);
    luabridge::LuaRef& parent_table = *type_data.baseTable;
    
    establishInheritance(instance_table, parent_table);
    
    instance_table["key"] = key;
    instance_table["enabled"] = true;
    instance_table["actor"] = owner;
    
    Component c;
    c.key = key;
    c.type = comp_name;
    c.componentRef = std::make_shared<luabridge::LuaRef>(instance_table);
    c.hasOnStart = type_data.hasOnStart;
    c.hasOnUpdate = type_data.hasOnUpdate;
    c.hasOnLateUpdate = type_data.hasOnLateUpdate;
    
    return c;
}

void ComponentDB::establishInheritance(luabridge::LuaRef& instance_table, luabridge::LuaRef & parent_table) {
    luabridge::LuaRef new_metatable = luabridge::newTable(lua_state);
    new_metatable["__index"] = parent_table;
    
    instance_table.push(lua_state);
    new_metatable.push(lua_state);
    lua_setmetatable(lua_state, -2);
    lua_pop(lua_state, 1);
}

void ComponentDB::ApplyPropertiesToComponent(Component& c, const rapidjson::Value& comp_obj) {
    luabridge::LuaRef& self = *c.componentRef;
    for (auto oride = comp_obj.MemberBegin(); oride != comp_obj.MemberEnd(); ++oride) {
        std::string property = oride->name.GetString();
        if (property == "type") continue;
        const auto& val = oride->value;
        
        if (val.IsInt()) {
            self[property] = val.GetInt();
        }
        else if (val.IsNumber()) {
            self[property] = val.GetFloat();
        }
        else if (val.IsBool()) {
            self[property] = val.GetBool();
        }
        else if (val.IsString()) {
            self[property] = std::string(val.GetString());
        }
        
    }
}

void ComponentDB::ApplyPropertiesToComponent(Component& c, const TemplateComponentData& comp_data) {
    luabridge::LuaRef& self = *c.componentRef;
    
    for (const auto& [property, val] : comp_data.properties) {
        switch (val.type) {
            case PropertyValue::Type::Int:
                self[property] = val.int_val;
                break;
            case PropertyValue::Type::Float:
                self[property] = val.float_val;
                break;
            case PropertyValue::Type::Bool:
                self[property] = val.bool_val;
                break;
            case PropertyValue::Type::String:
                self[property] = val.string_val;
                break;
        }
    }
}

Component ComponentDB::createCppComponentInstance(std::string& key, std::string& comp_name, Actor* owner) {
    Component c;
    c.key = key;
    c.type = comp_name;
    c.hasOnStart = false;
    c.hasOnUpdate = false;
    c.hasOnLateUpdate = false;
    
    if (comp_name == "Rigidbody") {
        Rigidbody* rb = new Rigidbody();
        rb->actor = owner;
        c.componentRef = std::make_shared<luabridge::LuaRef>(lua_state, rb);
    }
    else if (comp_name == "ParticleSystem") {
        ParticleSystem* ps = new ParticleSystem();
        ps->actor = owner;
        c.componentRef = std::make_shared<luabridge::LuaRef>(lua_state, ps);
    }
    
    return c;
}

void ComponentDB::ApplyPropertiesToCppComponent(Component& c, const rapidjson::Value& comp_obj) {
    if (c.type == "Rigidbody") {
        Rigidbody* rb = c.componentRef->cast<Rigidbody*>();
        if (!rb) return;
        
        if (comp_obj.HasMember("x") && comp_obj["x"].IsNumber()) rb->x = comp_obj["x"].GetFloat();
        if (comp_obj.HasMember("y") && comp_obj["y"].IsNumber()) rb->y = comp_obj["y"].GetFloat();
        if (comp_obj.HasMember("body_type") && comp_obj["body_type"].IsString()) rb->body_type = comp_obj["body_type"].GetString();
        if (comp_obj.HasMember("precise") && comp_obj["precise"].IsBool()) rb->precise = comp_obj["precise"].GetBool();
        if (comp_obj.HasMember("gravity_scale") && comp_obj["gravity_scale"].IsNumber()) rb->gravity_scale = comp_obj["gravity_scale"].GetFloat();
        if (comp_obj.HasMember("density") && comp_obj["density"].IsNumber()) rb->density = comp_obj["density"].GetFloat();
        if (comp_obj.HasMember("angular_friction") && comp_obj["angular_friction"].IsNumber()) rb->angular_friction = comp_obj["angular_friction"].GetFloat();
        if (comp_obj.HasMember("rotation") && comp_obj["rotation"].IsNumber()) rb->rotation = comp_obj["rotation"].GetFloat();
        if (comp_obj.HasMember("has_collider") && comp_obj["has_collider"].IsBool()) rb->has_collider = comp_obj["has_collider"].GetBool();
        if (comp_obj.HasMember("has_trigger") && comp_obj["has_trigger"].IsBool()) rb->has_trigger = comp_obj["has_trigger"].GetBool();
        if (comp_obj.HasMember("collider_type") && comp_obj["collider_type"].IsString()) rb->collider_type = comp_obj["collider_type"].GetString();
        if (comp_obj.HasMember("width") && comp_obj["width"].IsNumber()) rb->width = comp_obj["width"].GetFloat();
        if (comp_obj.HasMember("height") && comp_obj["height"].IsNumber()) rb->height = comp_obj["height"].GetFloat();
        if (comp_obj.HasMember("radius") && comp_obj["radius"].IsNumber()) rb->radius = comp_obj["radius"].GetFloat();
        if (comp_obj.HasMember("friction") && comp_obj["friction"].IsNumber()) rb->friction = comp_obj["friction"].GetFloat();
        if (comp_obj.HasMember("bounciness") && comp_obj["bounciness"].IsNumber()) rb->bounciness = comp_obj["bounciness"].GetFloat();
        if (comp_obj.HasMember("trigger_type") && comp_obj["trigger_type"].IsString()) rb->trigger_type = comp_obj["trigger_type"].GetString();
        if (comp_obj.HasMember("trigger_width") && comp_obj["trigger_width"].IsNumber()) rb->trigger_width = comp_obj["trigger_width"].GetFloat();
        if (comp_obj.HasMember("trigger_height") && comp_obj["trigger_height"].IsNumber()) rb->trigger_height = comp_obj["trigger_height"].GetFloat();
        if (comp_obj.HasMember("trigger_radius") && comp_obj["trigger_radius"].IsNumber()) rb->trigger_radius = comp_obj["trigger_radius"].GetFloat();
    }
    else if (c.type == "ParticleSystem") {
        ParticleSystem* ps = c.componentRef->cast<ParticleSystem*>();
        if (!ps) return;
        
        if (comp_obj.HasMember("x") && comp_obj["x"].IsNumber()) ps->x = comp_obj["x"].GetFloat();
        if (comp_obj.HasMember("y") && comp_obj["y"].IsNumber()) ps->y = comp_obj["y"].GetFloat();
        if (comp_obj.HasMember("frames_between_bursts") && comp_obj["frames_between_bursts"].IsInt()) ps->frames_between_bursts = comp_obj["frames_between_bursts"].GetInt();
        if (comp_obj.HasMember("burst_quantity") && comp_obj["burst_quantity"].IsInt()) ps->burst_quantity = comp_obj["burst_quantity"].GetInt();
        if (comp_obj.HasMember("start_scale_min") && comp_obj["start_scale_min"].IsNumber()) ps->start_scale_min = comp_obj["start_scale_min"].GetFloat();
        if (comp_obj.HasMember("start_scale_max") && comp_obj["start_scale_max"].IsNumber()) ps->start_scale_max = comp_obj["start_scale_max"].GetFloat();
        if (comp_obj.HasMember("rotation_min") && comp_obj["rotation_min"].IsNumber()) ps->rotation_min = comp_obj["rotation_min"].GetFloat();
        if (comp_obj.HasMember("rotation_max") && comp_obj["rotation_max"].IsNumber()) ps->rotation_max = comp_obj["rotation_max"].GetFloat();
        if (comp_obj.HasMember("start_color_r") && comp_obj["start_color_r"].IsInt()) ps->start_color_r = comp_obj["start_color_r"].GetInt();
        if (comp_obj.HasMember("start_color_g") && comp_obj["start_color_g"].IsInt()) ps->start_color_g = comp_obj["start_color_g"].GetInt();
        if (comp_obj.HasMember("start_color_b") && comp_obj["start_color_b"].IsInt()) ps->start_color_b = comp_obj["start_color_b"].GetInt();
        if (comp_obj.HasMember("start_color_a") && comp_obj["start_color_a"].IsInt()) ps->start_color_a = comp_obj["start_color_a"].GetInt();
        if (comp_obj.HasMember("emit_radius_min") && comp_obj["emit_radius_min"].IsNumber()) ps->emit_radius_min = comp_obj["emit_radius_min"].GetFloat();
        if (comp_obj.HasMember("emit_radius_max") && comp_obj["emit_radius_max"].IsNumber()) ps->emit_radius_max = comp_obj["emit_radius_max"].GetFloat();
        if (comp_obj.HasMember("emit_angle_min") && comp_obj["emit_angle_min"].IsNumber()) ps->emit_angle_min = comp_obj["emit_angle_min"].GetFloat();
        if (comp_obj.HasMember("emit_angle_max") && comp_obj["emit_angle_max"].IsNumber()) ps->emit_angle_max = comp_obj["emit_angle_max"].GetFloat();
        if (comp_obj.HasMember("image") && comp_obj["image"].IsString()) ps->image = comp_obj["image"].GetString();
        if (comp_obj.HasMember("sorting_order") && comp_obj["sorting_order"].IsInt()) ps->sorting_order = comp_obj["sorting_order"].GetInt();
        
        // Test Suite #2 properties
        if (comp_obj.HasMember("duration_frames") && comp_obj["duration_frames"].IsInt()) ps->duration_frames = comp_obj["duration_frames"].GetInt();
        if (comp_obj.HasMember("start_speed_min") && comp_obj["start_speed_min"].IsNumber()) ps->start_speed_min = comp_obj["start_speed_min"].GetFloat();
        if (comp_obj.HasMember("start_speed_max") && comp_obj["start_speed_max"].IsNumber()) ps->start_speed_max = comp_obj["start_speed_max"].GetFloat();
        if (comp_obj.HasMember("rotation_speed_min") && comp_obj["rotation_speed_min"].IsNumber()) ps->rotation_speed_min = comp_obj["rotation_speed_min"].GetFloat();
        if (comp_obj.HasMember("rotation_speed_max") && comp_obj["rotation_speed_max"].IsNumber()) ps->rotation_speed_max = comp_obj["rotation_speed_max"].GetFloat();
        if (comp_obj.HasMember("gravity_scale_x") && comp_obj["gravity_scale_x"].IsNumber()) ps->gravity_scale_x = comp_obj["gravity_scale_x"].GetFloat();
        if (comp_obj.HasMember("gravity_scale_y") && comp_obj["gravity_scale_y"].IsNumber()) ps->gravity_scale_y = comp_obj["gravity_scale_y"].GetFloat();
        if (comp_obj.HasMember("drag_factor") && comp_obj["drag_factor"].IsNumber()) ps->drag_factor = comp_obj["drag_factor"].GetFloat();
        if (comp_obj.HasMember("angular_drag_factor") && comp_obj["angular_drag_factor"].IsNumber()) ps->angular_drag_factor = comp_obj["angular_drag_factor"].GetFloat();
        if (comp_obj.HasMember("end_scale") && comp_obj["end_scale"].IsNumber()) ps->end_scale = comp_obj["end_scale"].GetFloat();
        if (comp_obj.HasMember("end_color_r") && comp_obj["end_color_r"].IsInt()) ps->end_color_r = comp_obj["end_color_r"].GetInt();
        if (comp_obj.HasMember("end_color_g") && comp_obj["end_color_g"].IsInt()) ps->end_color_g = comp_obj["end_color_g"].GetInt();
        if (comp_obj.HasMember("end_color_b") && comp_obj["end_color_b"].IsInt()) ps->end_color_b = comp_obj["end_color_b"].GetInt();
        if (comp_obj.HasMember("end_color_a") && comp_obj["end_color_a"].IsInt()) ps->end_color_a = comp_obj["end_color_a"].GetInt();
    }
    
}

void ComponentDB::ApplyPropertiesToCppComponent(Component& c, const TemplateComponentData& comp_data) {
    
    if (c.type == "Rigidbody") {
        if (c.type != "Rigidbody") return;
        
        Rigidbody* rb = c.componentRef->cast<Rigidbody*>();
        if (!rb) return;
        
        for (const auto& [property, val] : comp_data.properties) {
            if (property == "x") {
                if (val.type == PropertyValue::Type::Int) rb->x = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) rb->x = val.float_val;
            }
            else if (property == "y") {
                if (val.type == PropertyValue::Type::Int) rb->y = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) rb->y = val.float_val;
            }
            else if (property == "body_type" && val.type == PropertyValue::Type::String) rb->body_type = val.string_val;
            else if (property == "precise" && val.type == PropertyValue::Type::Bool) rb->precise = val.bool_val;
            else if (property == "gravity_scale") {
                if (val.type == PropertyValue::Type::Int) rb->gravity_scale = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) rb->gravity_scale = val.float_val;
            }
            else if (property == "density") {
                if (val.type == PropertyValue::Type::Int) rb->density = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) rb->density = val.float_val;
            }
            else if (property == "angular_friction") {
                if (val.type == PropertyValue::Type::Int) rb->angular_friction = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) rb->angular_friction = val.float_val;
            }
            else if (property == "rotation") {
                if (val.type == PropertyValue::Type::Int) rb->rotation = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) rb->rotation = val.float_val;
            }
            else if (property == "has_collider" && val.type == PropertyValue::Type::Bool) rb->has_collider = val.bool_val;
            else if (property == "has_trigger" && val.type == PropertyValue::Type::Bool) rb->has_trigger = val.bool_val;
            else if (property == "collider_type" && val.type == PropertyValue::Type::String) rb->collider_type = val.string_val;
            else if (property == "width") {
                if (val.type == PropertyValue::Type::Int) rb->width = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) rb->width = val.float_val;
            }
            else if (property == "height") {
                if (val.type == PropertyValue::Type::Int) rb->height = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) rb->height = val.float_val;
            }
            else if (property == "radius") {
                if (val.type == PropertyValue::Type::Int) rb->radius = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) rb->radius = val.float_val;
            }
            else if (property == "friction") {
                if (val.type == PropertyValue::Type::Int) rb->friction = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) rb->friction = val.float_val;
            }
            else if (property == "bounciness") {
                if (val.type == PropertyValue::Type::Int) rb->bounciness = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) rb->bounciness = val.float_val;
            }
            else if (property == "trigger_type" && val.type == PropertyValue::Type::String) rb->trigger_type = val.string_val;
            else if (property == "trigger_width") {
                if (val.type == PropertyValue::Type::Int) rb->trigger_width = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) rb->trigger_width = val.float_val;
            }
            else if (property == "trigger_height") {
                if (val.type == PropertyValue::Type::Int) rb->trigger_height = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) rb->trigger_height = val.float_val;
            }
            else if (property == "trigger_radius") {
                if (val.type == PropertyValue::Type::Int) rb->trigger_radius = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) rb->trigger_radius = val.float_val;
            }
        }
    }
    else if (c.type == "ParticleSystem") {
        ParticleSystem* ps = c.componentRef->cast<ParticleSystem*>();
        if (!ps) return;
        
        for (const auto& [property, val] : comp_data.properties) {
            if (property == "x") {
                if (val.type == PropertyValue::Type::Int) ps->x = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->x = val.float_val;
            }
            else if (property == "y") {
                if (val.type == PropertyValue::Type::Int) ps->y = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->y = val.float_val;
            }
            else if (property == "frames_between_bursts" && val.type == PropertyValue::Type::Int) ps->frames_between_bursts = val.int_val;
            else if (property == "burst_quantity" && val.type == PropertyValue::Type::Int) ps->burst_quantity = val.int_val;
            else if (property == "start_scale_min") {
                if (val.type == PropertyValue::Type::Int) ps->start_scale_min = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->start_scale_min = val.float_val;
            }
            else if (property == "start_scale_max") {
                if (val.type == PropertyValue::Type::Int) ps->start_scale_max = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->start_scale_max = val.float_val;
            }
            else if (property == "rotation_min") {
                if (val.type == PropertyValue::Type::Int) ps->rotation_min = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->rotation_min = val.float_val;
            }
            else if (property == "rotation_max") {
                if (val.type == PropertyValue::Type::Int) ps->rotation_max = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->rotation_max = val.float_val;
            }
            else if (property == "start_color_r" && val.type == PropertyValue::Type::Int) ps->start_color_r = val.int_val;
            else if (property == "start_color_g" && val.type == PropertyValue::Type::Int) ps->start_color_g = val.int_val;
            else if (property == "start_color_b" && val.type == PropertyValue::Type::Int) ps->start_color_b = val.int_val;
            else if (property == "start_color_a" && val.type == PropertyValue::Type::Int) ps->start_color_a = val.int_val;
            else if (property == "emit_radius_min") {
                if (val.type == PropertyValue::Type::Int) ps->emit_radius_min = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->emit_radius_min = val.float_val;
            }
            else if (property == "emit_radius_max") {
                if (val.type == PropertyValue::Type::Int) ps->emit_radius_max = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->emit_radius_max = val.float_val;
            }
            else if (property == "emit_angle_min") {
                if (val.type == PropertyValue::Type::Int) ps->emit_angle_min = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->emit_angle_min = val.float_val;
            }
            else if (property == "emit_angle_max") {
                if (val.type == PropertyValue::Type::Int) ps->emit_angle_max = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->emit_angle_max = val.float_val;
            }
            else if (property == "image" && val.type == PropertyValue::Type::String) ps->image = val.string_val;
            else if (property == "sorting_order" && val.type == PropertyValue::Type::Int) ps->sorting_order = val.int_val;
            // Test Suite #2 properties
            else if (property == "duration_frames" && val.type == PropertyValue::Type::Int) ps->duration_frames = val.int_val;
            else if (property == "start_speed_min") {
                if (val.type == PropertyValue::Type::Int) ps->start_speed_min = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->start_speed_min = val.float_val;
            }
            else if (property == "start_speed_max") {
                if (val.type == PropertyValue::Type::Int) ps->start_speed_max = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->start_speed_max = val.float_val;
            }
            else if (property == "rotation_speed_min") {
                if (val.type == PropertyValue::Type::Int) ps->rotation_speed_min = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->rotation_speed_min = val.float_val;
            }
            else if (property == "rotation_speed_max") {
                if (val.type == PropertyValue::Type::Int) ps->rotation_speed_max = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->rotation_speed_max = val.float_val;
            }
            else if (property == "gravity_scale_x") {
                if (val.type == PropertyValue::Type::Int) ps->gravity_scale_x = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->gravity_scale_x = val.float_val;
            }
            else if (property == "gravity_scale_y") {
                if (val.type == PropertyValue::Type::Int) ps->gravity_scale_y = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->gravity_scale_y = val.float_val;
            }
            else if (property == "drag_factor") {
                if (val.type == PropertyValue::Type::Int) ps->drag_factor = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->drag_factor = val.float_val;
            }
            else if (property == "angular_drag_factor") {
                if (val.type == PropertyValue::Type::Int) ps->angular_drag_factor = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->angular_drag_factor = val.float_val;
            }
            else if (property == "end_scale") {
                if (val.type == PropertyValue::Type::Int) ps->end_scale = static_cast<float>(val.int_val);
                else if (val.type == PropertyValue::Type::Float) ps->end_scale = val.float_val;
            }
            else if (property == "end_color_r" && val.type == PropertyValue::Type::Int) ps->end_color_r = val.int_val;
            else if (property == "end_color_g" && val.type == PropertyValue::Type::Int) ps->end_color_g = val.int_val;
            else if (property == "end_color_b" && val.type == PropertyValue::Type::Int) ps->end_color_b = val.int_val;
            else if (property == "end_color_a" && val.type == PropertyValue::Type::Int) ps->end_color_a = val.int_val;
        }
    }
}
