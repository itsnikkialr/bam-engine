#ifndef TEMPLATEDB_H
#define TEMPLATEDB_H

#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

#include "Actor.h"

#include "lua.hpp"
#include "LuaBridge.h"

#include <string>
#include <unordered_map>
#include <map>

struct PropertyValue {
    enum class Type {
        Int,
        Float,
        Bool,
        String
    };

    Type type = Type::Int;
    int int_val = 0;
    float float_val = 0.0f;
    bool bool_val = false;
    std::string string_val = "";
};

struct TemplateComponentData {
    std::string key;
    std::string type;
    std::unordered_map<std::string, PropertyValue> properties;
};

struct TemplateActorData {
    std::string actor_name = "";
    std::map<std::string, TemplateComponentData> components_by_key;
};

class TemplateDB {
public:
    void LoadAll(const std::string& dir);

    bool Has(const std::string& name) const;
    const TemplateActorData& Get(const std::string& name) const;

private:
    std::unordered_map<uint64_t, TemplateActorData> s_templates;

    static uint64_t hash_fnv1a(const std::string& str);
};

#endif
