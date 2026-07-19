//JContainersNG_Internal.cpp
#include "JContainersNG_Internal.hpp"
#include "FormSerializer.hpp"

bool g_jcApiLog = false;
std::mutex g_poolMutex;
std::unordered_map<std::string, std::vector<Handle>> g_pools;
std::mutex g_tagMutex;
std::unordered_map<std::string, std::unordered_set<Handle>> g_tags;

json::iterator FindKeyCI(json& j, const std::string& key) {
    if (!j.is_object()) return j.end();
    auto fast = j.find(key);
    if (fast != j.end()) return fast;
    for (auto it = j.begin(); it != j.end(); ++it) {
        const auto& k = it.key();
        if (k.size() != key.size()) continue;
        if (std::equal(k.begin(), k.end(), key.begin(), key.end(),
            [](char a, char b) {
                return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
            })) {
            return it;
        }
    }
    return j.end();
}

json::iterator FindKeyCI(json& j, std::string_view key) {
    if (!j.is_object()) return j.end();
    auto fast = j.find(std::string(key));
    if (fast != j.end()) return fast;
    for (auto it = j.begin(); it != j.end(); ++it) {
        const auto& k = it.key();
        if (k.size() != key.size()) continue;
        if (std::equal(k.begin(), k.end(), key.begin(), key.end(),
            [](char a, char b) {
                return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
            })) {
            return it;
        }
    }
    return j.end();
}

// --- path segment helpers --------------------------------------------------

namespace JCPathDetail {

    // base 0 — hex segments like "[0x1A]" are legal in OG-land
    inline bool ParseSegmentIndex(const std::string& s, int32_t& out) {
        try {
            size_t used = 0;
            long v = std::stol(s, &used, 0);
            if (used != s.size()) return false;
            out = static_cast<int32_t>(v);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    // arrays: [N] indexes, negatives wrap. objects: [N] hits the decimal
    // string key — thats JIntMap duck-typing doing its thing
    inline json* AccessIndex(json* current, int32_t idx) {
        if (current->is_array()) {
            int32_t count = static_cast<int32_t>(current->size());
            if (idx < 0) idx = count + idx;
            if (idx < 0 || idx >= count) return nullptr;
            return &(*current)[idx];
        }
        if (current->is_object()) {
            auto key = std::to_string(idx);
            auto it = FindKeyCI(*current, key);
            if (it == current->end()) return nullptr;
            return &it.value();
        }
        return nullptr;
    }

    inline json* AccessKey(json* current, const std::string& key) {
        if (!current->is_object()) return nullptr;
        auto it = FindKeyCI(*current, key);
        if (it == current->end()) return nullptr;
        return &it.value();
    }

} // namespace JCPathDetail

// path grammar (matches OG): ".key" map segments, "[N]" array/intmap segments,
// "[__formData|...]" formmap segments. createMissing spawns plain objects for
// any missing key segment — OG never creative-makes arrays either, and in our
// duck-typed world map/intmap/formmap intermediates are all the same json object
json* ResolvePath(json& root, const std::string& path, bool createMissing) {
    if (path.empty()) return &root;

    std::vector<std::shared_ptr<json>> keepAlive;
    json* current = &root;
    size_t pos = 0;
    const size_t pathSize = path.size();

    while (pos < pathSize) {
        // deref any __jc_ref we're standing on before stepping
        while (current->is_object() && current->contains(ObjectManager::REF_KEY)) {
            Handle h = (*current)[ObjectManager::REF_KEY].get<Handle>();
            auto ptr = ObjectManager::Get().GetObject(h);
            if (!ptr) return nullptr;
            keepAlive.push_back(ptr);
            current = ptr.get();
        }

        if (path[pos] == '.') {
            size_t end = path.find_first_of(".[", pos + 1);
            size_t keyLen = (end == std::string::npos) ? end : end - pos - 1;
            if (keyLen == 0) return nullptr; // ".." — garbage
            std::string key = path.substr(pos + 1, keyLen);

            json* next = JCPathDetail::AccessKey(current, key);
            if (!next) {
                if (!createMissing || !current->is_object()) return nullptr;
                Handle newObj = ObjectManager::Get().CreateObject();
                // real edge — parent holds an Internal ref on the intermediate
                ObjectManager::Get().EmbedEdge((*current)[key], newObj);
                next = JCPathDetail::AccessKey(current, key);
                if (!next) return nullptr;
            }
            current = next;
            pos = (end == std::string::npos) ? pathSize : end;
        }
        else if (path[pos] == '[') {
            size_t close = path.find(']', pos);
            if (close == std::string::npos || close == pos + 1) return nullptr;
            std::string inner = path.substr(pos + 1, close - pos - 1);

            json* next = nullptr;
            if (FormSerializer::IsFormString(inner)) {
                next = JCPathDetail::AccessKey(current, inner);
                if (!next && createMissing && current->is_object()) {
                    Handle newObj = ObjectManager::Get().CreateObject();
                    ObjectManager::Get().EmbedEdge((*current)[inner], newObj);
                    next = JCPathDetail::AccessKey(current, inner);
                }
            }
            else {
                int32_t idx;
                if (!JCPathDetail::ParseSegmentIndex(inner, idx)) return nullptr;
                next = JCPathDetail::AccessIndex(current, idx);
                // no createMissing for indices — OG doesnt stretch arrays or
                // invent intmap slots mid-path either
            }
            if (!next) return nullptr;
            current = next;
            pos = close + 1;
        }
        else if (pos == 0) {
            // bare leading key, no dot — old code allowed it, keep allowing it
            size_t end = path.find_first_of(".[", pos);
            size_t keyLen = (end == std::string::npos) ? end : end - pos;
            std::string key = path.substr(pos, keyLen);

            json* next = JCPathDetail::AccessKey(current, key);
            if (!next) {
                if (!createMissing || !current->is_object()) return nullptr;
                Handle newObj = ObjectManager::Get().CreateObject();
                ObjectManager::Get().EmbedEdge((*current)[key], newObj);
                next = JCPathDetail::AccessKey(current, key);
                if (!next) return nullptr;
            }
            current = next;
            pos = (end == std::string::npos) ? pathSize : end;
        }
        else {
            return nullptr; // mid-path garbage
        }
    }

    return current;
}

json* DerefFinal(json* raw) {
    if (!raw) return nullptr;
    while (raw->is_object() && raw->contains(ObjectManager::REF_KEY)) {
        Handle h = (*raw)[ObjectManager::REF_KEY].get<Handle>();
        auto ptr = ObjectManager::Get().GetObject(h);
        if (!ptr) return nullptr;
        raw = ptr.get();
    }
    return raw;
}

int32_t GetValueType(const json& val) {
    Handle h;
    if (ObjectManager::IsRef(val, &h)) {
        auto ptr = ObjectManager::Get().GetObject(h);
        if (!ptr) return 0;
        if (ptr->is_null()) return 1;
        if (ptr->is_number_integer() || ptr->is_boolean()) return 2;
        if (ptr->is_number_float()) return 3;
        if (ptr->is_string()) {
            if (FormSerializer::IsFormString(ptr->get<std::string>())) return 4;
            return 6;
        }
        if (ptr->is_object() || ptr->is_array()) return 5;
        return 0;
    }

    if (val.is_null()) return 1;
    if (val.is_number_integer() || val.is_boolean()) return 2;
    if (val.is_number_float()) return 3;
    if (val.is_string()) {
        if (FormSerializer::IsFormString(val.get<std::string>())) return 4;
        return 6;
    }
    if (val.is_object() || val.is_array()) return 5;
    return 0;
}

bool ErasePath(json& root, const std::string& path) {
    if (path.empty()) return false;

    // find where the final segment starts — could be ".key" or "[...]"
    size_t segStart = std::string::npos;
    bool finalIsBracket = false;
    for (size_t i = path.size(); i-- > 0;) {
        if (path[i] == ']') { finalIsBracket = true; }
        if (path[i] == '[' && finalIsBracket) { segStart = i; break; }
        if (path[i] == '.' && !finalIsBracket) { segStart = i; break; }
        if (path[i] == '.' || path[i] == '[') break; // boundary of an earlier segment
    }

    if (segStart == std::string::npos) {
        // single bare key, no separators
        if (!root.is_object()) return false;
        auto it = FindKeyCI(root, path);
        if (it != root.end()) {
            ObjectManager::Get().ReleaseEdge(it.value());
            root.erase(it);
            return true;
        }
        return false;
    }

    std::string parentPath = path.substr(0, segStart);
    std::string finalSeg = path.substr(segStart);

    auto* parent = ResolvePath(root, parentPath);
    parent = DerefFinal(parent);
    if (!parent) return false;

    if (finalIsBracket) {
        std::string inner = finalSeg.substr(1, finalSeg.size() - 2);

        if (FormSerializer::IsFormString(inner)) {
            if (!parent->is_object()) return false;
            auto it = FindKeyCI(*parent, inner);
            if (it == parent->end()) return false;
            ObjectManager::Get().ReleaseEdge(it.value());
            parent->erase(it);
            return true;
        }

        int32_t idx;
        if (!JCPathDetail::ParseSegmentIndex(inner, idx)) return false;

        if (parent->is_array()) {
            int32_t count = static_cast<int32_t>(parent->size());
            if (idx < 0) idx = count + idx;
            if (idx < 0 || idx >= count) return false;
            ObjectManager::Get().ReleaseEdge((*parent)[idx]);
            parent->erase(parent->begin() + idx);
            return true;
        }
        if (parent->is_object()) {
            // JIntMap duck-typing — decimal string key
            auto it = FindKeyCI(*parent, std::to_string(idx));
            if (it == parent->end()) return false;
            ObjectManager::Get().ReleaseEdge(it.value());
            parent->erase(it);
            return true;
        }
        return false;
    }

    // plain ".key" finale
    if (!parent->is_object()) return false;
    auto it = FindKeyCI(*parent, finalSeg.substr(1));
    if (it == parent->end()) return false;
    ObjectManager::Get().ReleaseEdge(it.value());
    parent->erase(it);
    return true;
}

int GetSortOrder(const json& j) {
    if (j.is_null()) return 0;
    if (j.is_number_integer() || j.is_boolean()) return 1;
    if (j.is_number_float()) return 2;
    if (j.is_string()) {
        if (FormSerializer::IsFormString(j.get<std::string>())) return 3;
        return 5;
    }
    if (j.is_object() || j.is_array()) return 4;
    return 6;
}