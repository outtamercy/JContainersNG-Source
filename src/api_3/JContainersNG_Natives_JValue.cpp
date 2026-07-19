//JContainersNG_Natives_JValue.cpp
#include "JContainersNG_Natives.hpp"
#include <fstream>
#include <filesystem>
#include <shlobj.h>

// Standalone helper to resolve the JCUser path, eliminating external link dependencies
std::string LocalGetJCUserDirectory() {
    wchar_t* wpath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &wpath))) {
        std::filesystem::path docPath(wpath);
        CoTaskMemFree(wpath);
        auto jcUserPath = docPath / "My Games" / "Skyrim Special Edition" / "JCUser" / "";
        std::filesystem::create_directories(jcUserPath);
        return jcUserPath.string();
    }
    return "Data/SKSE/Plugins/JContainersNG/";
}

// Resolves relative paths to the actual Data directory or JCUser
std::string ResolveJCPath(const std::string& inputPath) {
    std::string filePath = inputPath;

    // 1. Resolve user:/ prefix
    if (filePath.length() >= 6 && (_strnicmp(filePath.c_str(), "user:/", 6) == 0)) {
        return LocalGetJCUserDirectory() + filePath.substr(6);
    }

    // 2. Resolve native/relative path relative to Skyrim/Data/
    std::filesystem::path p(filePath);
    if (p.is_relative()) {
        // If it already starts with "data/" or "DATA/", don't double-prefix it
        if (filePath.length() >= 5 && (_strnicmp(filePath.c_str(), "data/", 5) == 0)) {
            return filePath;
        }
        return "Data/" + filePath;
    }

    return filePath;
}

void JValue_EnableAPILog(RE::StaticFunctionTag*, bool arg0) {
    g_jcApiLog = arg0;
}

Handle JValue_Retain(RE::StaticFunctionTag*, Handle obj, std::string tag) {
    if (!ObjectManager::Get().IsValid(obj)) return 0;
    ObjectManager::Get().Retain(obj, RefDomain::TES);
    if (!tag.empty()) {
        std::lock_guard<std::mutex> lock(g_tagMutex);
        g_tags[tag].insert(obj);
    }
    return obj;
}

Handle JValue_Release(RE::StaticFunctionTag*, Handle obj) {
    ObjectManager::Get().Release(obj, RefDomain::TES);
    return 0;
}

Handle JValue_ReleaseAndRetain(RE::StaticFunctionTag*, Handle previous, Handle newObj, std::string tag) {
    if (previous != 0) ObjectManager::Get().Release(previous, RefDomain::TES);
    if (newObj != 0 && ObjectManager::Get().IsValid(newObj)) {
        ObjectManager::Get().Retain(newObj, RefDomain::TES);
        if (!tag.empty()) {
            std::lock_guard<std::mutex> lock(g_tagMutex);
            g_tags[tag].insert(newObj);
        }
    }
    return newObj;
}

void JValue_ReleaseObjectsWithTag(RE::StaticFunctionTag*, std::string tag) {
    std::lock_guard<std::mutex> lock(g_tagMutex);
    auto it = g_tags.find(tag);
    if (it == g_tags.end()) return;
    for (auto h : it->second) {
        // drain every TES ref the tag piled up — Internal edges and root pins
        // aint ours, they die on their own schedule
        while (ObjectManager::Get().GetRefCount(h, RefDomain::TES) > 0) {
            ObjectManager::Get().Release(h, RefDomain::TES);
        }
    }
    g_tags.erase(it);
}

Handle JValue_ZeroLifetime(RE::StaticFunctionTag*, Handle obj) {
    if (ObjectManager::Get().IsValid(obj)) {
        ObjectManager::Get().SetZeroLifetime(obj);
    }
    return obj;
}

Handle JValue_AddToPool(RE::StaticFunctionTag*, Handle obj, std::string poolName) {
    if (!ObjectManager::Get().IsValid(obj)) return obj;
    std::lock_guard<std::mutex> lock(g_poolMutex);
    g_pools[poolName].push_back(obj);
    // pool membership is a GC root — otherwise the sweeper eats pooled objects
    // that aint reachable from JDB, which defeats the whole point of a pool
    ObjectManager::Get().Retain(obj, RefDomain::Internal);
    ObjectManager::Get().AddRootPin(obj);
    return obj;
}

void JValue_CleanPool(RE::StaticFunctionTag*, std::string poolName) {
    std::lock_guard<std::mutex> lock(g_poolMutex);
    auto it = g_pools.find(poolName);
    if (it == g_pools.end()) return;
    for (auto h : it->second) {
        ObjectManager::Get().RemoveRootPin(h);
        ObjectManager::Get().Release(h, RefDomain::Internal);
    }
    g_pools.erase(it);
}

Handle JValue_ShallowCopy(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return 0;

    if (ptr->is_object()) {
        Handle h = ObjectManager::Get().CreateObject();
        auto newPtr = ObjectManager::Get().GetObject(h);
        for (auto& [key, val] : ptr->items()) {
            Handle child;
            if (ObjectManager::IsRef(val, &child)) {
                ObjectManager::Get().EmbedEdge((*newPtr)[key], child);
            }
            else {
                (*newPtr)[key] = val;
            }
        }
        return h;
    }

    if (ptr->is_array()) {
        Handle h = ObjectManager::Get().CreateArray();
        auto newPtr = ObjectManager::Get().GetObject(h);
        for (auto& elem : *ptr) {
            Handle child;
            if (ObjectManager::IsRef(elem, &child)) {
                json slot;
                ObjectManager::Get().EmbedEdge(slot, child);
                newPtr->push_back(std::move(slot));
            }
            else {
                newPtr->push_back(elem);
            }
        }
        return h;
    }

    return 0;
}

Handle JValue_DeepCopy(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return 0;
    json external = JsonSerializer::ToExternal(*ptr);
    return JsonSerializer::FromExternal(external);
}

bool JValue_IsExists(RE::StaticFunctionTag*, Handle obj) {
    return ObjectManager::Get().IsValid(obj);
}

bool JValue_IsArray(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return false;
    return ptr->is_array();
}

bool JValue_IsMap(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return false;
    return ptr->is_object();
}

bool JValue_IsFormMap(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return false;
    return ptr->is_object();
}

bool JValue_IsIntegerMap(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return false;
    return ptr->is_object();
}

bool JValue_Empty(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return true;
    if (ptr->is_object() || ptr->is_array()) return ptr->empty();
    return true;
}

int32_t JValue_Count(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return 0;
    if (ptr->is_object() || ptr->is_array()) return static_cast<int32_t>(ptr->size());
    return 0;
}

void JValue_Clear(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return;
    if (ptr->is_object() || ptr->is_array()) {
        ObjectManager::Get().ReleaseAllEdges(*ptr);
        ptr->clear();
    }
}

Handle JValue_ReadFromFile(RE::StaticFunctionTag*, std::string filePath) {
    SKSE::log::info("JContainersNG: readFromFile RAW path = '{}'", filePath);
    filePath = ResolveJCPath(filePath);
    SKSE::log::info("JContainersNG: readFromFile RESOLVED to = '{}'", filePath);

    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            SKSE::log::warn("JContainersNG: readFromFile FAILED to open '{}'", filePath);
            return 0;
        }
        json j;
        file >> j;
        Handle h = JsonSerializer::FromExternal(j);
        SKSE::log::info("JContainersNG: readFromFile '{}' -> handle={}", filePath, h);
        return h;
    }
    catch (const std::exception& e) {
        SKSE::log::error("JContainersNG: readFromFile exception for '{}': {}", filePath, e.what());
        return 0;
    }
}

Handle JValue_ReadFromDirectory(RE::StaticFunctionTag*, std::string directoryPath, std::string extension) {
    SKSE::log::info("JContainersNG: readFromDirectory RAW path = '{}'", directoryPath);
    directoryPath = ResolveJCPath(directoryPath);
    SKSE::log::info("JContainersNG: readFromDirectory RESOLVED to = '{}'", directoryPath);

    try {
        Handle result = ObjectManager::Get().CreateObject();
        auto ptr = ObjectManager::Get().GetObject(result);
        if (!ptr) return 0;

        if (!std::filesystem::exists(directoryPath)) {
            SKSE::log::warn("JContainersNG: readFromDirectory folder does not exist: '{}'", directoryPath);
            return result;
        }

        int successCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (!entry.is_regular_file()) continue;

            // OG compares extension WITH the dot — mods pass ".json", and
            // stripping it here meant those filters matched literally nothing
            if (!extension.empty() && entry.path().extension().string() != extension) continue;

            std::string filename = entry.path().filename().string();
            try {
                std::ifstream file(entry.path());
                json j;
                file >> j;
                Handle obj = JsonSerializer::FromExternal(j);
                // real edge — parent map owns an Internal ref on each file's tree
                ObjectManager::Get().EmbedEdge((*ptr)[filename], obj);
                successCount++;
            }
            catch (const std::exception& e) {
                SKSE::log::error("JContainersNG: Failed parsing JSON file '{}': {}", filename, e.what());
            }
        }
        SKSE::log::info("JContainersNG: readFromDirectory successfully loaded {} files from '{}'", successCount, directoryPath);
        return result;
    }
    catch (const std::exception& e) {
        SKSE::log::error("JContainersNG: readFromDirectory critical exception for '{}': {}", directoryPath, e.what());
        return 0;
    }
}

Handle JValue_ObjectFromPrototype(RE::StaticFunctionTag*, std::string prototype) {
    try {
        json j = json::parse(prototype);
        return JsonSerializer::FromExternal(j);
    }
    catch (...) {
        return 0;
    }
}

void JValue_WriteToFile(RE::StaticFunctionTag*, Handle obj, std::string filePath) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return;

    SKSE::log::info("JContainersNG: writeToFile RAW path = '{}'", filePath);
    filePath = ResolveJCPath(filePath);
    SKSE::log::info("JContainersNG: writeToFile RESOLVED to = '{}'", filePath);

    try {
        json external = JsonSerializer::ToExternal(*ptr);
        std::filesystem::path p(filePath);
        std::filesystem::create_directories(p.parent_path());
        std::ofstream file(filePath);
        file << external.dump(2);
        SKSE::log::info("JContainersNG: writeToFile successfully saved to '{}'", filePath);
    }
    catch (const std::exception& e) {
        SKSE::log::error("JContainersNG: writeToFile failed for '{}': {}", filePath, e.what());
    }
}

std::string JValue_ToString(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return "{}";
    // OG dumps with JSON_INDENT(2) — compact output breaks string compares
    return JsonSerializer::ToExternal(*ptr).dump(2);
}

int32_t JValue_SolvedValueType(RE::StaticFunctionTag*, Handle obj, std::string path) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return 0;
    auto* target = ResolvePath(*ptr, path);
    if (!target) return 0;
    target = DerefFinal(target);
    if (!target) return 0;
    return GetValueType(*target);
}

bool JValue_HasPath(RE::StaticFunctionTag*, Handle obj, std::string path) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return false;
    return ResolvePath(*ptr, path) != nullptr;
}

float JValue_SolveFlt(RE::StaticFunctionTag*, Handle obj, std::string path, float defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return defaultVal;
    auto* target = ResolvePath(*ptr, path);
    target = DerefFinal(target);
    if (!target || !target->is_number()) return defaultVal;
    return target->get<float>();
}

int32_t JValue_SolveInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return defaultVal;
    auto* target = ResolvePath(*ptr, path);
    target = DerefFinal(target);
    if (!target || !target->is_number()) return defaultVal;
    if (target->is_number_integer()) return target->get<int32_t>();
    return static_cast<int32_t>(target->get<double>());
}

std::string JValue_SolveStr(RE::StaticFunctionTag*, Handle obj, std::string path, std::string defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return defaultVal;
    auto* target = ResolvePath(*ptr, path);
    target = DerefFinal(target);
    if (!target || !target->is_string()) return defaultVal;
    return target->get<std::string>();
}

Handle JValue_SolveObj(RE::StaticFunctionTag*, Handle obj, std::string path, Handle defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return defaultVal;
    auto* target = ResolvePath(*ptr, path);
    if (!target) return defaultVal;

    Handle refHandle;
    if (ObjectManager::IsRef(*target, &refHandle)) {
        return refHandle;
    }

    if (target->is_object() || target->is_array()) {
        Handle newHandle = ObjectManager::Get().RegisterObject(std::move(*target));
        ObjectManager::Get().EmbedEdge(*target, newHandle);
        return newHandle;
    }

    return defaultVal;
}

RE::TESForm* JValue_SolveForm(RE::StaticFunctionTag*, Handle obj, std::string path, RE::TESForm* defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return defaultVal;
    auto* target = ResolvePath(*ptr, path);
    target = DerefFinal(target);
    if (!target || !target->is_string()) return defaultVal;
    std::string str = target->get<std::string>();
    if (FormSerializer::IsFormString(str)) {
        return FormSerializer::DecodeForm(str);
    }
    return defaultVal;
}

bool JValue_SolveFltSetter(RE::StaticFunctionTag*, Handle obj, std::string path, float value, bool createMissing) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return false;
    auto* target = ResolvePath(*ptr, path, createMissing);
    if (!target) return false;
    ObjectManager::Get().ReleaseEdge(*target); // old value might be a ref
    *target = value;
    return true;
}

bool JValue_SolveIntSetter(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, bool createMissing) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return false;
    auto* target = ResolvePath(*ptr, path, createMissing);
    if (!target) return false;
    ObjectManager::Get().ReleaseEdge(*target);
    *target = value;
    return true;
}

bool JValue_SolveStrSetter(RE::StaticFunctionTag*, Handle obj, std::string path, std::string value, bool createMissing) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return false;
    auto* target = ResolvePath(*ptr, path, createMissing);
    if (!target) return false;
    ObjectManager::Get().ReleaseEdge(*target);
    *target = value;
    return true;
}

bool JValue_SolveObjSetter(RE::StaticFunctionTag*, Handle obj, std::string path, Handle value, bool createMissing) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return false;
    auto* target = ResolvePath(*ptr, path, createMissing);
    if (!target) return false;

    // EmbedEdge covers both cases: 0 clears the slot (and releases the old
    // edge), anything else releases old + retains new
    ObjectManager::Get().EmbedEdge(*target, value);
    return true;
}

bool JValue_SolveFormSetter(RE::StaticFunctionTag*, Handle obj, std::string path, RE::TESForm* value, bool createMissing) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return false;
    auto* target = ResolvePath(*ptr, path, createMissing);
    if (!target) return false;

    ObjectManager::Get().ReleaseEdge(*target);
    if (!value) {
        *target = nullptr;
        return true;
    }
    *target = FormSerializer::EncodeForm(value);
    return true;
}