#include "EventBus.h"
#include <iostream>
#include <algorithm>

void EventBus::Publish(const std::string& event_type, luabridge::LuaRef event_object) {
    auto it = subscribers.find(event_type);
    if (it == subscribers.end()) return;

    for (auto& sub : it->second) {
        try {
            (*sub.function)(*sub.component, event_object);
        } catch (const luabridge::LuaException& e) {
            std::cout << "\033[31m" << e.what() << "\033[0m" << std::endl;
        }
    }
}

void EventBus::Subscribe(const std::string& event_type, luabridge::LuaRef component, luabridge::LuaRef function) {
    PendingSubscription p;
    p.event_type = event_type;
    p.component = std::make_shared<luabridge::LuaRef>(component);
    p.function = std::make_shared<luabridge::LuaRef>(function);
    p.is_subscribe = true;
    pending.push_back(p);
}

void EventBus::Unsubscribe(const std::string& event_type, luabridge::LuaRef component, luabridge::LuaRef function) {
    PendingSubscription p;
    p.event_type = event_type;
    p.component = std::make_shared<luabridge::LuaRef>(component);
    p.function = std::make_shared<luabridge::LuaRef>(function);
    p.is_subscribe = false;
    pending.push_back(p);
}

void EventBus::ProcessPending() {
    for (auto& p : pending) {
        if (p.is_subscribe) {
            Subscription sub;
            sub.component = p.component;
            sub.function = p.function;
            subscribers[p.event_type].push_back(sub);
        } else {
            auto it = subscribers.find(p.event_type);
            if (it == subscribers.end()) continue;

            auto& subs = it->second;
            subs.erase(
                std::remove_if(subs.begin(), subs.end(),
                    [&p](const Subscription& sub) {
                        return *sub.component == *p.component && *sub.function == *p.function;
                    }),
                subs.end()
            );
        }
    }
    pending.clear();
}
