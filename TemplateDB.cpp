#include "TemplateDB.h"
#include <iostream>
#include <cstdlib>
#include <filesystem>

#include "rapidjson/document.h"
#include "EngineUtils.h"

namespace fs = std::filesystem;

static void ReadPropertyValue(const rapidjson::Value& val, PropertyValue& out) {
    if (val.IsInt()) {
        out.type = PropertyValue::Type::Int;
        out.int_val = val.GetInt();
    }
    else if (val.IsNumber()) {
        out.type = PropertyValue::Type::Float;
        out.float_val = val.GetFloat();
    }
    else if (val.IsBool()) {
        out.type = PropertyValue::Type::Bool;
        out.bool_val = val.GetBool();
    }
    else if (val.IsString()) {
        out.type = PropertyValue::Type::String;
        out.string_val = val.GetString();
    }
}

static void ApplyAllFieldsFromJsonObject(const rapidjson::Value& obj, TemplateActorData& actor_data) {
    if (obj.HasMember("name") && obj["name"].IsString()) {
        actor_data.actor_name = obj["name"].GetString();
    }

    if (obj.HasMember("components") && obj["components"].IsObject()) {
        const auto& components = obj["components"];

        for (auto it = components.MemberBegin(); it != components.MemberEnd(); ++it) {
            std::string key = it->name.GetString();
            const auto& comp_obj = it->value;

            if (!comp_obj.IsObject()) continue;
            if (!comp_obj.HasMember("type") || !comp_obj["type"].IsString()) continue;

            TemplateComponentData comp_data;
            comp_data.key = key;
            comp_data.type = comp_obj["type"].GetString();

            for (auto prop = comp_obj.MemberBegin(); prop != comp_obj.MemberEnd(); ++prop) {
                std::string prop_name = prop->name.GetString();
                if (prop_name == "type") continue;

                PropertyValue val;
                ReadPropertyValue(prop->value, val);
                comp_data.properties[prop_name] = std::move(val);
            }

            actor_data.components_by_key[key] = std::move(comp_data);
        }
    }
}

uint64_t TemplateDB::hash_fnv1a(const std::string& str) {
    const uint64_t fnv_prime = 1099511628211ULL;
    uint64_t hash = 1469598103934665603ULL;

    for (unsigned char c : str) {
        hash ^= c;
        hash *= fnv_prime;
    }
    return hash;
}

void TemplateDB::LoadAll(const std::string& dir) {
    s_templates.clear();

    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        return;
    }

    for (const auto& entry : fs::directory_iterator(dir)) {
        const fs::path p = entry.path();
        if (p.extension() != ".template") continue;

        rapidjson::Document doc;
        EngineUtils::ReadJsonFile(p.string(), doc);
        if (!doc.IsObject()) continue;

        TemplateActorData actor_data;
        ApplyAllFieldsFromJsonObject(doc, actor_data);

        const std::string name = p.stem().string();
        const uint64_t key = hash_fnv1a(name);

        s_templates[key] = std::move(actor_data);
    }
}

bool TemplateDB::Has(const std::string& name) const {
    const uint64_t key = hash_fnv1a(name);
    return s_templates.find(key) != s_templates.end();
}

const TemplateActorData& TemplateDB::Get(const std::string& name) const {
    const uint64_t key = hash_fnv1a(name);
    return s_templates.at(key);
}
