//JsonSerializer.hpp
#pragma once
#include <nlohmann/json.hpp>
#include "ObjectManager.hpp"
#include "FormSerializer.hpp"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>

using json = nlohmann::json;

// lives in JContainersNG_Internal.cpp (which includes this header, so we
// declare instead of include). WalkPath needs the same CI key lookup the
// rest of the plugin uses — OG's istring maps resolve refs case-blind too.
// MUST stay at global scope — inside the namespace it declares a different
// function and the linker goes home empty-handed (LNK2019, ask me how I know)
json::iterator FindKeyCI(json& j, const std::string& key);

namespace JsonSerializer {

    // OG writes "__metaInfo":{"typeName":"JFormMap"|"JIntMap"} so readers can
    // rebuild the right container type. legacy files used a "__formData" key
    // with null value to mean form_map. we infer on write, honor on read
    static constexpr const char* kMetaInfo = "__metaInfo";
    static constexpr const char* kMetaInfoLegacy = "__formData"; // sic — collides with form strings, OG's problem not ours
    static constexpr const char* kTypeName = "typeName";
    static constexpr const char* kRefPrefix = "__reference|";

    // ---------- type inference helpers ----------

    inline bool ParseIntKey(const std::string& key, std::int32_t& out) {
        try {
            size_t used = 0;
            long v = std::stol(key, &used, 0); // base 0 — hex keys like "0x1A" are legal in OG-land
            if (used != key.size()) return false;
            out = static_cast<std::int32_t>(v);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    // duck-type a json object into an OG typeName, or "" for plain JMap.
    // empty objects are untaggable — cant infer from zero keys, leave em alone
    inline std::string InferTypeName(const json& obj) {
        if (!obj.is_object() || obj.empty()) return "";
        bool allForm = true, allInt = true;
        for (auto& [key, val] : obj.items()) {
            if (!FormSerializer::IsFormString(key)) allForm = false;
            std::int32_t dummy;
            if (!ParseIntKey(key, dummy)) allInt = false;
        }
        if (allForm) return "JFormMap";
        if (allInt) return "JIntMap";
        return "";
    }

    // ---------- external write (internal -> plain JSON, for writeToFile etc.) ----------

    class ExternalWriter {
    public:
        json Write(const json& internal) {
            _firstPath.clear();
            return WriteNode(internal, "");
        }

    private:
        // handle -> path where it was first inlined. second sighting of the
        // same handle (shared or cyclic) becomes a "__reference|path" string
        std::unordered_map<ObjectManager::Handle, std::string> _firstPath;

        json WriteNode(const json& node, const std::string& pathHere) {
            ObjectManager::Handle h;
            if (ObjectManager::IsRef(node, &h)) {
                auto it = _firstPath.find(h);
                if (it != _firstPath.end()) {
                    return std::string(kRefPrefix) + it->second; // seen before — reference it
                }
                auto ptr = ObjectManager::Get().GetObject(h);
                if (!ptr) return json::object();
                _firstPath[h] = pathHere;
                return WriteContainer(*ptr, pathHere);
            }
            if (node.is_object() || node.is_array()) {
                return WriteContainer(node, pathHere);
            }
            return node; // primitives & form strings pass through
        }

        json WriteContainer(const json& cnt, const std::string& pathHere) {
            if (cnt.is_array()) {
                json result = json::array();
                int idx = 0;
                for (auto& elem : cnt) {
                    result.push_back(WriteNode(elem, pathHere + "[" + std::to_string(idx++) + "]"));
                }
                return result;
            }

            json result = json::object();
            std::string typeName = InferTypeName(cnt);
            if (!typeName.empty()) {
                result[kMetaInfo] = json::object({ { kTypeName, typeName } });
            }
            for (auto& [key, val] : cnt.items()) {
                // OG path syntax: form-string keys go in brackets, everything
                // else gets a dot. matches their path_appender
                std::string childPath = FormSerializer::IsFormString(key)
                    ? pathHere + "[" + key + "]"
                    : pathHere + "." + key;
                result[key] = WriteNode(val, childPath);
            }
            return result;
        }
    };

    inline json ToExternal(const json& internal) {
        return ExternalWriter().Write(internal);
    }

    // ---------- external read (plain JSON -> internal handle graph) ----------

    class ExternalReader {
    public:
        ObjectManager::Handle Read(const json& external) {
            _pendingRefs.clear();
            ObjectManager::Handle root = BuildNode(external);
            ResolveReferences(root);
            return root;
        }

    private:
        struct PendingRef {
            json* slot;        // where the resolved handle gets embedded
            std::string path;  // "__reference|" already stripped
            ObjectManager::Handle ownerHint; // unused for now, keeping shape simple
        };
        // slots point into objects we own during the build — parents below
        // keep every child json alive via shared_ptr, so pointers stay valid
        std::vector<std::pair<json*, std::string>> _pendingRefs;

        ObjectManager::Handle BuildNode(const json& external) {
            if (external.is_array()) {
                ObjectManager::Handle h = ObjectManager::Get().CreateArray();
                auto ptr = ObjectManager::Get().GetObject(h);
                for (auto& elem : external) {
                    AppendValue(*ptr, elem);
                }
                return h;
            }

            if (external.is_object()) {
                // sniff the type tag before anything else, then drop it —
                // its a serialization detail, not data
                std::string typeName;
                auto meta = external.find(kMetaInfo);
                if (meta != external.end() && meta->is_object()) {
                    auto tn = meta->find(kTypeName);
                    if (tn != meta->end() && tn->is_string()) {
                        typeName = tn->get<std::string>();
                    }
                }
                else if (external.find(kMetaInfoLegacy) != external.end() &&
                    external[kMetaInfoLegacy].is_null()) {
                    typeName = "JFormMap"; // legacy marker: "__formData": null
                }

                ObjectManager::Handle h = ObjectManager::Get().CreateObject();
                auto ptr = ObjectManager::Get().GetObject(h);
                for (auto& [key, val] : external.items()) {
                    if (key == kMetaInfo || (key == kMetaInfoLegacy && val.is_null())) {
                        continue;
                    }
                    // JIntMap keys can be hex ("0x1A") in OG files — normalize
                    // to decimal so JIntMap.getInt(26) actually finds them
                    std::string normKey = key;
                    if (typeName == "JIntMap") {
                        std::int32_t intKey;
                        if (ParseIntKey(key, intKey)) {
                            normKey = std::to_string(intKey);
                        }
                    }
                    SetValue(*ptr, normKey, val);
                }
                return h;
            }

            // top-level primitive — weird but legal
            return ObjectManager::Get().CreateNull();
        }

        // one json value into a parent slot: primitives direct, containers as
        // embedded child objects, "__reference|" strings queued for phase 2
        void PlaceValue(json& slot, const json& val) {
            if (val.is_object() || val.is_array()) {
                ObjectManager::Handle child = BuildNode(val);
                ObjectManager::Get().EmbedEdge(slot, child);
                return;
            }
            if (val.is_string()) {
                const auto& s = val.get_ref<const std::string&>();
                if (s.starts_with(kRefPrefix)) {
                    _pendingRefs.emplace_back(&slot, s.substr(std::string(kRefPrefix).size()));
                    slot = nullptr; // resolved in phase 2 — null till then
                    return;
                }
            }
            slot = val;
        }

        void AppendValue(json& arr, const json& val) {
            arr.push_back(nullptr);
            PlaceValue(arr.back(), val);
        }

        void SetValue(json& obj, const std::string& key, const json& val) {
            obj[key] = nullptr;
            PlaceValue(obj[key], val);
        }

        // phase 2: walk "__reference|" paths against the freshly built tree.
        // path grammar is OG's: ".key" segments, "[N]" array/int indices,
        // "[__formData|...]" form keys. empty path = the root itself
        void ResolveReferences(ObjectManager::Handle root) {
            for (auto& [slot, path] : _pendingRefs) {
                ObjectManager::Handle target = (path.empty()) ? root : WalkPath(root, path);
                if (target != ObjectManager::INVALID_HANDLE && ObjectManager::Get().IsValid(target)) {
                    ObjectManager::Get().EmbedEdge(*slot, target);
                }
                // unresolvable refs stay null — OG skips em quietly too
            }
            _pendingRefs.clear();
        }

        ObjectManager::Handle WalkPath(ObjectManager::Handle root, const std::string& path) {
            ObjectManager::Handle current = root;
            size_t pos = 0;

            while (pos < path.size()) {
                auto ptr = ObjectManager::Get().GetObject(current);
                if (!ptr) return ObjectManager::INVALID_HANDLE;

                if (path[pos] == '.') {
                    size_t end = path.find_first_of(".[", pos + 1);
                    std::string key = path.substr(pos + 1, end == std::string::npos ? end : end - pos - 1);
                    if (!ptr->is_object()) return ObjectManager::INVALID_HANDLE;
                    auto kit = FindKeyCI(*ptr, key);
                    if (kit == ptr->end()) return ObjectManager::INVALID_HANDLE;
                    if (!FollowRef(kit.value(), current)) return ObjectManager::INVALID_HANDLE;
                    pos = (end == std::string::npos) ? path.size() : end;
                }
                else if (path[pos] == '[') {
                    size_t close = path.find(']', pos);
                    if (close == std::string::npos) return ObjectManager::INVALID_HANDLE;
                    std::string inner = path.substr(pos + 1, close - pos - 1);

                    if (FormSerializer::IsFormString(inner)) {
                        if (!ptr->is_object()) return ObjectManager::INVALID_HANDLE;
                        auto kit = FindKeyCI(*ptr, inner);
                        if (kit == ptr->end()) return ObjectManager::INVALID_HANDLE;
                        if (!FollowRef(kit.value(), current)) return ObjectManager::INVALID_HANDLE;
                    }
                    else {
                        std::int32_t idx;
                        if (!ParseIntKey(inner, idx)) return ObjectManager::INVALID_HANDLE;
                        if (ptr->is_array()) {
                            if (idx < 0) idx = static_cast<std::int32_t>(ptr->size()) + idx;
                            if (idx < 0 || idx >= static_cast<std::int32_t>(ptr->size())) return ObjectManager::INVALID_HANDLE;
                            if (!FollowRef((*ptr)[idx], current)) return ObjectManager::INVALID_HANDLE;
                        }
                        else if (ptr->is_object()) {
                            // intmap-style string key
                            auto key = std::to_string(idx);
                            if (!ptr->contains(key)) return ObjectManager::INVALID_HANDLE;
                            if (!FollowRef((*ptr)[key], current)) return ObjectManager::INVALID_HANDLE;
                        }
                        else {
                            return ObjectManager::INVALID_HANDLE;
                        }
                    }
                    pos = close + 1;
                }
                else {
                    return ObjectManager::INVALID_HANDLE; // garbage segment
                }
            }
            return current;
        }

        // step through a slot that should hold an object ref
        bool FollowRef(const json& slot, ObjectManager::Handle& out) {
            ObjectManager::Handle h;
            if (ObjectManager::IsRef(slot, &h)) {
                out = h;
                return true;
            }
            return false; // path walked into a primitive — dead end
        }
    };

    inline ObjectManager::Handle FromExternal(const json& external) {
        return ExternalReader().Read(external);
    }

} // namespace JsonSerializer