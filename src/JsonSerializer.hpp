#pragma once
#include <nlohmann/json.hpp>
#include "ObjectManager.hpp"
#include <unordered_set>

using json = nlohmann::json;

namespace JsonSerializer {

    // Forward declare the recursive helper
    inline json ToExternal(const json& internal, std::unordered_set<ObjectManager::Handle>& visited, int maxDepth = 64);

    inline json ToExternal(const json& internal) {
        std::unordered_set<ObjectManager::Handle> visited;
        return ToExternal(internal, visited);
    }

    inline json ToExternal(const json& internal, std::unordered_set<ObjectManager::Handle>& visited, int maxDepth) {
        if (maxDepth-- <= 0) return json::object();

        // If this is a __jc_ref, inline the referenced object
        if (internal.is_object()) {
            ObjectManager::Handle h;
            if (ObjectManager::IsRef(internal, &h)) {
                if (visited.contains(h)) {
                    // Circular ref — break the chain
                    return json::object();
                }
                visited.insert(h);
                auto ptr = ObjectManager::Get().GetObject(h);
                if (ptr) {
                    return ToExternal(*ptr, visited, maxDepth);
                }
                return json::object();
            }

            json result = json::object();
            for (auto& [key, val] : internal.items()) {
                result[key] = ToExternal(val, visited, maxDepth);
            }
            return result;
        }

        if (internal.is_array()) {
            json result = json::array();
            for (auto& elem : internal) {
                result.push_back(ToExternal(elem, visited, maxDepth));
            }
            return result;
        }

        // Primitives pass through untouched
        return internal;
    }

    // Helper: convert external nested JSON to internal handle-referenced JSON
    inline json ToInternal(const json& external);

    inline ObjectManager::Handle FromExternal(const json& external) {
        if (external.is_object()) {
            ObjectManager::Handle h = ObjectManager::Get().CreateObject();
            auto ptr = ObjectManager::Get().GetObject(h);
            for (auto& [key, val] : external.items()) {
                (*ptr)[key] = ToInternal(val);
            }
            return h;
        }

        if (external.is_array()) {
            ObjectManager::Handle h = ObjectManager::Get().CreateArray();
            auto ptr = ObjectManager::Get().GetObject(h);
            for (auto& elem : external) {
                ptr->push_back(ToInternal(elem));
            }
            return h;
        }

        // Shouldn't happen at top level, but handle gracefully
        return ObjectManager::Get().CreateNull();
    }

    inline json ToInternal(const json& external) {
        if (external.is_object()) {
            ObjectManager::Handle h = ObjectManager::Get().CreateObject();
            auto ptr = ObjectManager::Get().GetObject(h);
            for (auto& [key, val] : external.items()) {
                (*ptr)[key] = ToInternal(val);
            }
            return ObjectManager::MakeRef(h);
        }

        if (external.is_array()) {
            ObjectManager::Handle h = ObjectManager::Get().CreateArray();
            auto ptr = ObjectManager::Get().GetObject(h);
            for (auto& elem : external) {
                ptr->push_back(ToInternal(elem));
            }
            return ObjectManager::MakeRef(h);
        }

        // Primitives and form strings pass through
        return external;
    }

} // namespace JsonSerializer