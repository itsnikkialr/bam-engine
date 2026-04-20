#ifndef EVENTBUS_H
#define EVENTBUS_H

#include "lua.hpp"
#include "LuaBridge.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

struct Subscription {
    std::shared_ptr<luabridge::LuaRef> component;
    std::shared_ptr<luabridge::LuaRef> function;
};

struct PendingSubscription {
    std::string event_type;
    std::shared_ptr<luabridge::LuaRef> component;
    std::shared_ptr<luabridge::LuaRef> function;
    bool is_subscribe;
};

class EventBus {
public:
    static inline std::unordered_map<std::string, std::vector<Subscription>> subscribers;
    static inline std::vector<PendingSubscription> pending;

    static void Publish(const std::string& event_type, luabridge::LuaRef event_object);
    static void Subscribe(const std::string& event_type, luabridge::LuaRef component, luabridge::LuaRef function);
    static void Unsubscribe(const std::string& event_type, luabridge::LuaRef component, luabridge::LuaRef function);
    static void ProcessPending();
};

#endif
