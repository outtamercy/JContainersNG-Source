#include <SKSE/SKSE.h>
#include <RE/P/PackUnpack.h>
#include <RE/V/VirtualMachine.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include "ObjectManager.hpp"
#include "FormSerializer.hpp"
#include "JsonSerializer.hpp"
#include <string_view>

using json = nlohmann::json;
namespace {
    // Case-insensitive key finder for nlohmann json objects.
    // Original JC used util::istring keys — this brings back that behavior.
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
}
using Handle = ObjectManager::Handle;

static bool g_jcApiLog = false;

// ------------------------------------------------------------------
// Path resolution helpers
// ------------------------------------------------------------------

namespace {
    // Resolve a dot-path on a json tree. Creates missing objects as __jc_ref handles if asked.
    // Intermediate __jc_ref are transparently dereferenced. Final value is returned raw.
    json* ResolvePath(json& root, const std::string& path, bool createMissing = false) {
        if (path.empty()) return &root;

        std::vector<std::shared_ptr<json>> keepAlive;
        json* current = &root;
        size_t start = (path[0] == '.') ? 1 : 0;
        size_t pathSize = path.size();

        while (start < pathSize) {
            while (current->is_object() && current->contains(ObjectManager::REF_KEY)) {
                Handle h = (*current)[ObjectManager::REF_KEY].get<Handle>();
                auto ptr = ObjectManager::Get().GetObject(h);
                if (!ptr) return nullptr;
                keepAlive.push_back(ptr);
                current = ptr.get();
            }

            size_t dot = path.find('.', start);
            std::string_view segment = (dot == std::string::npos)
                ? std::string_view(path).substr(start)
                : std::string_view(path).substr(start, dot - start);

            if (current->is_object()) {
                auto it = FindKeyCI(*current, segment);
                if (it == current->end()) {
                    if (!createMissing) return nullptr;
                    Handle newObj = ObjectManager::Get().CreateObject();
                    (*current)[std::string(segment)] = ObjectManager::MakeRef(newObj);
                    auto ptr = ObjectManager::Get().GetObject(newObj);
                    if (!ptr) return nullptr;
                    keepAlive.push_back(ptr);
                    current = ptr.get();
                }
                else {
                    current = &it.value();
                }
            }
            else {
                return nullptr;
            }

            if (dot == std::string::npos) break;
            start = dot + 1;
        }

        return current;
    }
    // Dereference a final __jc_ref for value reads
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

    // Type enum for JC: 0=no value, 1=none, 2=int, 3=float, 4=form, 5=object, 6=string
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

    // Erase a key at a dot-path (needs parent access)
    bool ErasePath(json& root, const std::string& path) {
        if (path.empty()) return false;

        size_t lastDot = path.find_last_of('.');
        if (lastDot == std::string::npos || lastDot == 0) {
            // Top-level key on root object
            std::string key = (path[0] == '.') ? path.substr(1) : path;
            if (!root.is_object()) return false;
            auto it = FindKeyCI(root, key);
            if (it != root.end()) {
                root.erase(it);
                return true;
            }
            return false;
        }

        std::string parentPath = path.substr(0, lastDot);
        std::string key = path.substr(lastDot + 1);

        auto* parent = ResolvePath(root, parentPath);
        parent = DerefFinal(parent);
        if (!parent || !parent->is_object()) return false;
        auto it = FindKeyCI(*parent, key);
        if (it != parent->end()) {
            parent->erase(it);
            return true;
        }
        return false;
    }

    // Pool storage for JValue.addToPool / cleanPool
    std::mutex g_poolMutex;
    std::unordered_map<std::string, std::vector<Handle>> g_pools;

    // Tag tracking for releaseObjectsWithTag
    std::mutex g_tagMutex;
    std::unordered_map<std::string, std::unordered_set<Handle>> g_tags;

    // Sort order helper for JArray.sort
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

    // PArray helper — decoy vector keeps RegisterFunction happy, we punch through to the VM stack directly.
    // Stack layout: base[2] is the targetArray param (reference_array<T> from Papyrus).
    template <typename T, typename JExtractor>
    bool WritePArray(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackID, Handle obj, std::vector<T> decoy, int32_t writeAtIdx, int32_t stopWriteAtIdx, int32_t readIdx, T defaultRead, JExtractor extract) {
        (void)decoy; // RegisterFunction unpacks the Papyrus array into this — we ignore it and write straight to the VM array.

        auto ptr = ObjectManager::Get().GetObject(obj);
        if (!ptr || !ptr->is_array()) return false;

        // grab the exact stack frame the VM is running right now
        auto* internalVM = static_cast<RE::BSScript::Internal::VirtualMachine*>(vm);
        auto it = internalVM->allRunningStacks.find(stackID);
        if (it == internalVM->allRunningStacks.end()) return false;

        auto* stack = it->second.get();
        if (!stack || !stack->top) return false;

        // StackFrame ends at 0x40, args are laid out inline right after it.
        // base[2] is the targetArray param passed from Papyrus.
        auto* args = reinterpret_cast<RE::BSScript::Variable*>(stack->top + 1);
        auto& targetVar = args[2];
        auto* pArray = targetVar.GetArray().get();
        if (!pArray) return false;

        std::uint32_t pArraySize = pArray->size();
        if (writeAtIdx < 0) writeAtIdx = 0;
        if (stopWriteAtIdx < 0 || stopWriteAtIdx > static_cast<int32_t>(pArraySize)) stopWriteAtIdx = static_cast<int32_t>(pArraySize);
        if (readIdx < 0) readIdx = 0;

        for (int32_t w = writeAtIdx, r = readIdx; w < stopWriteAtIdx; ++w, ++r) {
            T val = defaultRead;
            if (r >= 0 && r < static_cast<int32_t>(ptr->size())) {
                val = extract((*ptr)[r], defaultRead);
            }
            RE::BSScript::PackValue(&pArray->data()[w], val);
        }
        return true;
    }
}

// ------------------------------------------------------------------
// JValue
// ------------------------------------------------------------------

void JValue_EnableAPILog(RE::StaticFunctionTag*, bool arg0) {
    g_jcApiLog = arg0;
}

Handle JValue_Retain(RE::StaticFunctionTag*, Handle obj, std::string tag) {
    if (!ObjectManager::Get().IsValid(obj)) return 0;
    ObjectManager::Get().Retain(obj);
    if (!tag.empty()) {
        std::lock_guard<std::mutex> lock(g_tagMutex);
        g_tags[tag].insert(obj);
    }
    return obj;
}

Handle JValue_Release(RE::StaticFunctionTag*, Handle obj) {
    ObjectManager::Get().Release(obj);
    return 0;
}

Handle JValue_ReleaseAndRetain(RE::StaticFunctionTag*, Handle previous, Handle newObj, std::string tag) {
    if (previous != 0) ObjectManager::Get().Release(previous);
    if (newObj != 0 && ObjectManager::Get().IsValid(newObj)) {
        ObjectManager::Get().Retain(newObj);
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
        if (ObjectManager::Get().IsValid(h)) {
            ObjectManager::Get().Release(h);
        }
    }
    g_tags.erase(it);
}

Handle JValue_ZeroLifetime(RE::StaticFunctionTag*, Handle obj) {
    // Can't hook Papyrus variable destruction to auto-release.
    // Calling Release here immediately deletes the object while the Papyrus variable
    // still holds the handle. This breaks any mod that calls zeroLifetime then uses
    // the object later (FF does this). No-op to prevent premature deletion.
    return obj;
}

Handle JValue_AddToPool(RE::StaticFunctionTag*, Handle obj, std::string poolName) {
    if (!ObjectManager::Get().IsValid(obj)) return obj;
    std::lock_guard<std::mutex> lock(g_poolMutex);
    g_pools[poolName].push_back(obj);
    ObjectManager::Get().Retain(obj);
    return obj;
}

void JValue_CleanPool(RE::StaticFunctionTag*, std::string poolName) {
    std::lock_guard<std::mutex> lock(g_poolMutex);
    auto it = g_pools.find(poolName);
    if (it == g_pools.end()) return;
    for (auto h : it->second) {
        ObjectManager::Get().Release(h);
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
            (*newPtr)[key] = val; // copies __jc_ref markers, not deep cloning
        }
        return h;
    }

    if (ptr->is_array()) {
        Handle h = ObjectManager::Get().CreateArray();
        auto newPtr = ObjectManager::Get().GetObject(h);
        for (auto& elem : *ptr) {
            newPtr->push_back(elem);
        }
        return h;
    }

    return 0;
}

Handle JValue_DeepCopy(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return 0;
    // Inline everything to external JSON, then rebuild fresh handles
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
    // FormMap is just a JMap with Form keys — same underlying type
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
    if (ptr->is_object() || ptr->is_array()) ptr->clear();
}

Handle JValue_ReadFromFile(RE::StaticFunctionTag*, std::string filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            SKSE::log::info("JContainersNG: ReadFromFile FAILED to open '{}'", filePath);
            return 0;
        }
        json j;
        file >> j;
        Handle h = JsonSerializer::FromExternal(j);
        SKSE::log::info("JContainersNG: ReadFromFile '{}' -> handle={}", filePath, h);
        return h;
    }
    catch (const std::exception& e) {
        SKSE::log::error("JContainersNG: ReadFromFile exception for '{}': {}", filePath, e.what());
        return 0;
    }
}

Handle JValue_ReadFromDirectory(RE::StaticFunctionTag*, std::string directoryPath, std::string extension) {
    try {
        Handle result = ObjectManager::Get().CreateObject();
        auto ptr = ObjectManager::Get().GetObject(result);
        if (!ptr) return 0;

        if (!std::filesystem::exists(directoryPath)) return result;

        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (!entry.is_regular_file()) continue;

            std::string ext = entry.path().extension().string();
            if (!ext.empty() && ext[0] == '.') ext = ext.substr(1);
            if (!extension.empty() && ext != extension) continue;

            std::string filename = entry.path().filename().string();
            try {
                std::ifstream file(entry.path());
                json j;
                file >> j;
                Handle obj = JsonSerializer::FromExternal(j);
                (*ptr)[filename] = ObjectManager::MakeRef(obj);
            }
            catch (...) {
                // Bad file? Skip it, not our problem
            }
        }
        return result;
    }
    catch (...) {
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

    try {
        json external = JsonSerializer::ToExternal(*ptr);
        std::filesystem::path p(filePath);
        std::filesystem::create_directories(p.parent_path());
        std::ofstream file(filePath);
        file << external.dump(2);
    }
    catch (const std::exception& e) {
        SKSE::log::error("JValue.writeToFile failed: {}", e.what());
    }
}

std::string JValue_ToString(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return "{}";
    return JsonSerializer::ToExternal(*ptr).dump();
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
        // Promote inline object to a handle — this shouldn't happen often with our createMissing,
        // but handles cases where raw JSON was injected
        Handle newHandle = ObjectManager::Get().RegisterObject(std::move(*target));
        *target = ObjectManager::MakeRef(newHandle);
        return newHandle;
    }

    return defaultVal;
}

RE::TESForm* JValue_SolveForm(RE::StaticFunctionTag*, Handle obj, std::string path, RE::TESForm* defaultVal) {
    SKSE::log::info("JContainersNG: SolveForm ENTER obj={} path='{}'", obj, path);
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) {
        SKSE::log::info("JContainersNG: SolveForm obj={} path='{}' - NULL obj", obj, path);
        return defaultVal;
    }
    auto* target = ResolvePath(*ptr, path);
    target = DerefFinal(target);
    if (!target) {
        SKSE::log::info("JContainersNG: SolveForm obj={} path='{}' - path not found", obj, path);
        return defaultVal;
    }
    if (!target->is_string()) {
        SKSE::log::info("JContainersNG: SolveForm obj={} path='{}' - not string, type={}", obj, path, target->type_name());
        return defaultVal;
    }
    std::string str = target->get<std::string>();
    SKSE::log::info("JContainersNG: SolveForm obj={} path='{}' - string='{}'", obj, path, str);
    if (FormSerializer::IsFormString(str)) {
        auto* form = FormSerializer::DecodeForm(str);
        SKSE::log::info("JContainersNG: SolveForm obj={} path='{}' - DecodeForm result={}", obj, path, form ? "FOUND" : "NULL");
        return form;
    }
    SKSE::log::info("JContainersNG: SolveForm obj={} path='{}' - not a form string: '{}'", obj, path, str);
    return defaultVal;
}

bool JValue_SolveFltSetter(RE::StaticFunctionTag*, Handle obj, std::string path, float value, bool createMissing) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return false;
    auto* target = ResolvePath(*ptr, path, createMissing);
    if (!target) return false;
    *target = value;
    return true;
}

bool JValue_SolveIntSetter(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, bool createMissing) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return false;
    auto* target = ResolvePath(*ptr, path, createMissing);
    if (!target) return false;
    *target = value;
    return true;
}

bool JValue_SolveStrSetter(RE::StaticFunctionTag*, Handle obj, std::string path, std::string value, bool createMissing) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return false;
    auto* target = ResolvePath(*ptr, path, createMissing);
    if (!target) return false;
    *target = value;
    return true;
}

bool JValue_SolveObjSetter(RE::StaticFunctionTag*, Handle obj, std::string path, Handle value, bool createMissing) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return false;
    auto* target = ResolvePath(*ptr, path, createMissing);
    if (!target) return false;

    if (value == 0) {
        *target = nullptr;
        return true;
    }
    *target = ObjectManager::MakeRef(value);
    return true;
}

bool JValue_SolveFormSetter(RE::StaticFunctionTag*, Handle obj, std::string path, RE::TESForm* value, bool createMissing) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return false;
    auto* target = ResolvePath(*ptr, path, createMissing);
    if (!target) return false;

    if (!value) {
        *target = nullptr;
        return true;
    }
    *target = FormSerializer::EncodeForm(value);
    return true;
}

// ------------------------------------------------------------------
// JMap
// ------------------------------------------------------------------

Handle JMap_Object(RE::StaticFunctionTag*) {
    return ObjectManager::Get().CreateObject();
}

int32_t JMap_GetInt(RE::StaticFunctionTag*, Handle obj, std::string key, int32_t defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return defaultVal;
    auto it = FindKeyCI(*ptr, key);
    if (it == ptr->end()) return defaultVal;
    auto& val = it.value();
    if (val.is_number_integer()) return val.get<int32_t>();
    if (val.is_number_float()) return static_cast<int32_t>(val.get<double>());
    return defaultVal;
}

float JMap_GetFlt(RE::StaticFunctionTag*, Handle obj, std::string key, float defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return defaultVal;
    auto it = FindKeyCI(*ptr, key);
    if (it == ptr->end()) return defaultVal;
    auto& val = it.value();
    if (val.is_number()) return val.get<float>();
    return defaultVal;
}

std::string JMap_GetStr(RE::StaticFunctionTag*, Handle obj, std::string key, std::string defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return defaultVal;
    auto it = FindKeyCI(*ptr, key);
    if (it == ptr->end()) return defaultVal;
    auto& val = it.value();
    if (val.is_string()) return val.get<std::string>();
    return defaultVal;
}

Handle JMap_GetObj(RE::StaticFunctionTag*, Handle obj, std::string key, Handle defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return defaultVal;
    auto it = FindKeyCI(*ptr, key);
    if (it == ptr->end()) return defaultVal;

    auto& val = it.value();
    Handle refHandle;
    if (ObjectManager::IsRef(val, &refHandle)) return refHandle;

    if (val.is_object() || val.is_array()) {
        Handle newHandle = ObjectManager::Get().RegisterObject(std::move(val));
        val = ObjectManager::MakeRef(newHandle);
        return newHandle;
    }
    return defaultVal;
}

RE::TESForm* JMap_GetForm(RE::StaticFunctionTag*, Handle obj, std::string key, RE::TESForm* defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return defaultVal;
    auto it = FindKeyCI(*ptr, key);
    if (it == ptr->end()) return defaultVal;
    auto& val = it.value();
    if (val.is_string()) {
        std::string str = val.get<std::string>();
        if (FormSerializer::IsFormString(str)) return FormSerializer::DecodeForm(str);
    }
    return defaultVal;
}

void JMap_SetInt(RE::StaticFunctionTag*, Handle obj, std::string key, int32_t value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    auto it = FindKeyCI(*ptr, key);
    if (it != ptr->end()) {
        it.value() = value;
    }
    else {
        (*ptr)[key] = value;
    }
}

void JMap_SetFlt(RE::StaticFunctionTag*, Handle obj, std::string key, float value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    auto it = FindKeyCI(*ptr, key);
    if (it != ptr->end()) {
        it.value() = value;
    }
    else {
        (*ptr)[key] = value;
    }
}

void JMap_SetStr(RE::StaticFunctionTag*, Handle obj, std::string key, std::string value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    auto it = FindKeyCI(*ptr, key);
    if (it != ptr->end()) {
        it.value() = value;
    }
    else {
        (*ptr)[key] = value;
    }
}

void JMap_SetObj(RE::StaticFunctionTag*, Handle obj, std::string key, Handle value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    if (value == 0) {
        auto it = FindKeyCI(*ptr, key);
        if (it != ptr->end()) ptr->erase(it);
        return;
    }
    auto it = FindKeyCI(*ptr, key);
    if (it != ptr->end()) {
        it.value() = ObjectManager::MakeRef(value);
    }
    else {
        (*ptr)[key] = ObjectManager::MakeRef(value);
    }
}

void JMap_SetForm(RE::StaticFunctionTag*, Handle obj, std::string key, RE::TESForm* value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    if (!value) {
        auto it = FindKeyCI(*ptr, key);
        if (it != ptr->end()) ptr->erase(it);
        return;
    }
    auto it = FindKeyCI(*ptr, key);
    if (it != ptr->end()) {
        it.value() = FormSerializer::EncodeForm(value);
    }
    else {
        (*ptr)[key] = FormSerializer::EncodeForm(value);
    }
}

bool JMap_HasKey(RE::StaticFunctionTag*, Handle obj, std::string key) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return false;
    return FindKeyCI(*ptr, key) != ptr->end();
}

int32_t JMap_ValueType(RE::StaticFunctionTag*, Handle obj, std::string key) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    auto it = FindKeyCI(*ptr, key);
    if (it == ptr->end()) return 0;
    return GetValueType(it.value());
}

Handle JMap_AllKeys(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    Handle arr = ObjectManager::Get().CreateArray();
    auto arrPtr = ObjectManager::Get().GetObject(arr);
    for (auto& [key, val] : ptr->items()) {
        arrPtr->push_back(key);
    }
    return arr;
}

std::vector<std::string> JMap_AllKeysPArray(RE::StaticFunctionTag*, Handle obj) {
    std::vector<std::string> result;
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return result;
    for (auto& [key, val] : ptr->items()) {
        result.push_back(key);
    }
    return result;
}

Handle JMap_AllValues(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    Handle arr = ObjectManager::Get().CreateArray();
    auto arrPtr = ObjectManager::Get().GetObject(arr);
    for (auto& [key, val] : ptr->items()) {
        arrPtr->push_back(val);
    }
    return arr;
}

bool JMap_RemoveKey(RE::StaticFunctionTag*, Handle obj, std::string key) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return false;
    auto it = FindKeyCI(*ptr, key);
    if (it != ptr->end()) {
        ptr->erase(it);
        return true;
    }
    return false;
}

int32_t JMap_Count(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    return static_cast<int32_t>(ptr->size());
}

void JMap_Clear(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    ptr->clear();
}

void JMap_AddPairs(RE::StaticFunctionTag*, Handle obj, Handle source, bool overrideDuplicates) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    auto srcPtr = ObjectManager::Get().GetObject(source);
    if (!ptr || !ptr->is_object() || !srcPtr || !srcPtr->is_object()) return;
    for (auto& [key, val] : srcPtr->items()) {
        auto it = FindKeyCI(*ptr, key);
        if (!overrideDuplicates && it != ptr->end()) continue;
        if (it != ptr->end()) {
            it.value() = val;
        }
        else {
            (*ptr)[key] = val;
        }
    }
}

std::string JMap_NextKey(RE::StaticFunctionTag*, Handle obj, std::string previousKey, std::string endKey) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return endKey;
    if (previousKey == endKey) {
        if (ptr->empty()) return endKey;
        return ptr->begin().key();
    }
    bool found = false;
    for (auto& [k, val] : ptr->items()) {
        if (found) return k;
        if (std::equal(k.begin(), k.end(), previousKey.begin(), previousKey.end(),
            [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); })) {
            found = true;
        }
    }
    return endKey;
}

std::string JMap_GetNthKey(RE::StaticFunctionTag*, Handle obj, int32_t keyIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return "";
    if (keyIndex < 0) keyIndex = static_cast<int32_t>(ptr->size()) + keyIndex;
    if (keyIndex < 0 || keyIndex >= static_cast<int32_t>(ptr->size())) return "";
    auto it = ptr->begin();
    std::advance(it, keyIndex);
    return it.key();
}

// ------------------------------------------------------------------
// JArray
// ------------------------------------------------------------------

Handle JArray_Object(RE::StaticFunctionTag*) {
    return ObjectManager::Get().CreateArray();
}

Handle JArray_ObjectWithSize(RE::StaticFunctionTag*, int32_t size) {
    if (size < 0) size = 0;
    Handle h = ObjectManager::Get().CreateArray();
    auto ptr = ObjectManager::Get().GetObject(h);
    for (int32_t i = 0; i < size; ++i) ptr->push_back(json(nullptr));
    return h;
}

Handle JArray_ObjectWithInts(RE::StaticFunctionTag*, std::vector<int32_t> values) {
    Handle h = ObjectManager::Get().CreateArray();
    auto ptr = ObjectManager::Get().GetObject(h);
    for (auto v : values) ptr->push_back(v);
    return h;
}

Handle JArray_ObjectWithStrings(RE::StaticFunctionTag*, std::vector<std::string> values) {
    Handle h = ObjectManager::Get().CreateArray();
    auto ptr = ObjectManager::Get().GetObject(h);
    for (auto& v : values) ptr->push_back(v);
    return h;
}

Handle JArray_ObjectWithFloats(RE::StaticFunctionTag*, std::vector<float> values) {
    Handle h = ObjectManager::Get().CreateArray();
    auto ptr = ObjectManager::Get().GetObject(h);
    for (auto v : values) ptr->push_back(v);
    return h;
}

Handle JArray_ObjectWithBooleans(RE::StaticFunctionTag*, std::vector<bool> values) {
    Handle h = ObjectManager::Get().CreateArray();
    auto ptr = ObjectManager::Get().GetObject(h);
    for (auto v : values) ptr->push_back(v ? 1 : 0);
    return h;
}

Handle JArray_ObjectWithForms(RE::StaticFunctionTag*, std::vector<RE::TESForm*> values) {
    Handle h = ObjectManager::Get().CreateArray();
    auto ptr = ObjectManager::Get().GetObject(h);
    for (auto v : values) {
        ptr->push_back(v ? json(FormSerializer::EncodeForm(v)) : json(nullptr));
    }
    return h;
}

Handle JArray_SubArray(RE::StaticFunctionTag*, Handle obj, int32_t startIndex, int32_t endIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return 0;
    if (startIndex < 0) startIndex = static_cast<int32_t>(ptr->size()) + startIndex;
    if (endIndex < 0) endIndex = static_cast<int32_t>(ptr->size()) + endIndex;
    if (startIndex < 0) startIndex = 0;
    if (endIndex > static_cast<int32_t>(ptr->size())) endIndex = static_cast<int32_t>(ptr->size());
    if (startIndex >= endIndex) return JArray_Object(nullptr);

    Handle result = ObjectManager::Get().CreateArray();
    auto newPtr = ObjectManager::Get().GetObject(result);
    for (int32_t i = startIndex; i < endIndex; ++i) {
        newPtr->push_back((*ptr)[i]);
    }
    return result;
}

void JArray_AddFromArray(RE::StaticFunctionTag*, Handle obj, Handle source, int32_t insertAtIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    auto srcPtr = ObjectManager::Get().GetObject(source);
    if (!ptr || !ptr->is_array() || !srcPtr || !srcPtr->is_array()) return;

    if (insertAtIndex < 0 || insertAtIndex >= static_cast<int32_t>(ptr->size())) {
        for (auto& elem : *srcPtr) ptr->push_back(elem);
    }
    else {
        auto it = ptr->begin() + insertAtIndex;
        for (auto& elem : *srcPtr) {
            it = ptr->insert(it, elem);
            ++it;
        }
    }
}

void JArray_AddFromFormList(RE::StaticFunctionTag*, Handle obj, RE::BGSListForm* source, int32_t insertAtIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array() || !source) return;

    if (insertAtIndex < 0 || insertAtIndex >= static_cast<int32_t>(ptr->size())) {
        for (auto& form : source->forms) {
            ptr->push_back(FormSerializer::EncodeForm(form));
        }
    }
    else {
        auto it = ptr->begin() + insertAtIndex;
        for (auto& form : source->forms) {
            it = ptr->insert(it, FormSerializer::EncodeForm(form));
            ++it;
        }
    }
}

int32_t JArray_GetInt(RE::StaticFunctionTag*, Handle obj, int32_t index, int32_t defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return defaultVal;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return defaultVal;
    auto& val = (*ptr)[index];
    if (val.is_number_integer()) return val.get<int32_t>();
    if (val.is_number_float()) return static_cast<int32_t>(val.get<double>());
    return defaultVal;
}

float JArray_GetFlt(RE::StaticFunctionTag*, Handle obj, int32_t index, float defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return defaultVal;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return defaultVal;
    auto& val = (*ptr)[index];
    if (val.is_number()) return val.get<float>();
    return defaultVal;
}

std::string JArray_GetStr(RE::StaticFunctionTag*, Handle obj, int32_t index, std::string defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return defaultVal;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return defaultVal;
    auto& val = (*ptr)[index];
    if (val.is_string()) return val.get<std::string>();
    return defaultVal;
}

Handle JArray_GetObj(RE::StaticFunctionTag*, Handle obj, int32_t index, Handle defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return defaultVal;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return defaultVal;

    auto& val = (*ptr)[index];
    Handle refHandle;
    if (ObjectManager::IsRef(val, &refHandle)) return refHandle;

    if (val.is_object() || val.is_array()) {
        Handle newHandle = ObjectManager::Get().RegisterObject(std::move(val));
        val = ObjectManager::MakeRef(newHandle);
        return newHandle;
    }
    return defaultVal;
}

RE::TESForm* JArray_GetForm(RE::StaticFunctionTag*, Handle obj, int32_t index, RE::TESForm* defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return defaultVal;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return defaultVal;
    auto& val = (*ptr)[index];
    if (val.is_string()) {
        std::string str = val.get<std::string>();
        if (FormSerializer::IsFormString(str)) return FormSerializer::DecodeForm(str);
    }
    return defaultVal;
}

std::vector<int32_t> JArray_AsIntArray(RE::StaticFunctionTag*, Handle obj) {
    std::vector<int32_t> result;
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return result;
    for (auto& elem : *ptr) {
        if (elem.is_number_integer()) result.push_back(elem.get<int32_t>());
        else if (elem.is_number()) result.push_back(static_cast<int32_t>(elem.get<double>()));
        else result.push_back(0);
    }
    return result;
}

std::vector<float> JArray_AsFloatArray(RE::StaticFunctionTag*, Handle obj) {
    std::vector<float> result;
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return result;
    for (auto& elem : *ptr) {
        if (elem.is_number()) result.push_back(elem.get<float>());
        else result.push_back(0.0f);
    }
    return result;
}

std::vector<std::string> JArray_AsStringArray(RE::StaticFunctionTag*, Handle obj) {
    std::vector<std::string> result;
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return result;
    for (auto& elem : *ptr) {
        if (elem.is_string()) result.push_back(elem.get<std::string>());
        else result.push_back("");
    }
    return result;
}

std::vector<RE::TESForm*> JArray_AsFormArray(RE::StaticFunctionTag*, Handle obj) {
    std::vector<RE::TESForm*> result;
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return result;
    for (auto& elem : *ptr) {
        if (elem.is_string()) {
            std::string str = elem.get<std::string>();
            if (FormSerializer::IsFormString(str)) result.push_back(FormSerializer::DecodeForm(str));
            else result.push_back(nullptr);
        }
        else {
            result.push_back(nullptr);
        }
    }
    return result;
}

int32_t JArray_FindInt(RE::StaticFunctionTag*, Handle obj, int32_t value, int32_t searchStartIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return -1;
    if (searchStartIndex < 0) searchStartIndex = static_cast<int32_t>(ptr->size()) + searchStartIndex;
    if (searchStartIndex < 0) searchStartIndex = 0;
    for (int32_t i = searchStartIndex; i < static_cast<int32_t>(ptr->size()); ++i) {
        auto& val = (*ptr)[i];
        if (val.is_number_integer() && val.get<int32_t>() == value) return i;
    }
    return -1;
}

int32_t JArray_FindFlt(RE::StaticFunctionTag*, Handle obj, float value, int32_t searchStartIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return -1;
    if (searchStartIndex < 0) searchStartIndex = static_cast<int32_t>(ptr->size()) + searchStartIndex;
    if (searchStartIndex < 0) searchStartIndex = 0;
    for (int32_t i = searchStartIndex; i < static_cast<int32_t>(ptr->size()); ++i) {
        auto& val = (*ptr)[i];
        if (val.is_number() && val.get<float>() == value) return i;
    }
    return -1;
}

int32_t JArray_FindStr(RE::StaticFunctionTag*, Handle obj, std::string value, int32_t searchStartIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return -1;
    if (searchStartIndex < 0) searchStartIndex = static_cast<int32_t>(ptr->size()) + searchStartIndex;
    if (searchStartIndex < 0) searchStartIndex = 0;
    for (int32_t i = searchStartIndex; i < static_cast<int32_t>(ptr->size()); ++i) {
        auto& val = (*ptr)[i];
        if (val.is_string() && val.get<std::string>() == value) return i;
    }
    return -1;
}

int32_t JArray_FindObj(RE::StaticFunctionTag*, Handle obj, Handle container, int32_t searchStartIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return -1;
    if (searchStartIndex < 0) searchStartIndex = static_cast<int32_t>(ptr->size()) + searchStartIndex;
    if (searchStartIndex < 0) searchStartIndex = 0;
    for (int32_t i = searchStartIndex; i < static_cast<int32_t>(ptr->size()); ++i) {
        auto& val = (*ptr)[i];
        Handle h;
        if (ObjectManager::IsRef(val, &h) && h == container) return i;
    }
    return -1;
}

int32_t JArray_FindForm(RE::StaticFunctionTag*, Handle obj, RE::TESForm* value, int32_t searchStartIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array() || !value) return -1;
    std::string formStr = FormSerializer::EncodeForm(value);
    if (searchStartIndex < 0) searchStartIndex = static_cast<int32_t>(ptr->size()) + searchStartIndex;
    if (searchStartIndex < 0) searchStartIndex = 0;
    for (int32_t i = searchStartIndex; i < static_cast<int32_t>(ptr->size()); ++i) {
        auto& val = (*ptr)[i];
        if (val.is_string() && val.get<std::string>() == formStr) return i;
    }
    return -1;
}

int32_t JArray_CountInteger(RE::StaticFunctionTag*, Handle obj, int32_t value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return 0;
    int32_t count = 0;
    for (auto& elem : *ptr) {
        if (elem.is_number_integer() && elem.get<int32_t>() == value) ++count;
    }
    return count;
}

int32_t JArray_CountFloat(RE::StaticFunctionTag*, Handle obj, float value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return 0;
    int32_t count = 0;
    for (auto& elem : *ptr) {
        if (elem.is_number() && elem.get<float>() == value) ++count;
    }
    return count;
}

int32_t JArray_CountString(RE::StaticFunctionTag*, Handle obj, std::string value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return 0;
    int32_t count = 0;
    for (auto& elem : *ptr) {
        if (elem.is_string() && elem.get<std::string>() == value) ++count;
    }
    return count;
}

int32_t JArray_CountObject(RE::StaticFunctionTag*, Handle obj, Handle container) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return 0;
    int32_t count = 0;
    for (auto& elem : *ptr) {
        Handle h;
        if (ObjectManager::IsRef(elem, &h) && h == container) ++count;
    }
    return count;
}

int32_t JArray_CountForm(RE::StaticFunctionTag*, Handle obj, RE::TESForm* value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array() || !value) return 0;
    std::string formStr = FormSerializer::EncodeForm(value);
    int32_t count = 0;
    for (auto& elem : *ptr) {
        if (elem.is_string() && elem.get<std::string>() == formStr) ++count;
    }
    return count;
}

void JArray_SetInt(RE::StaticFunctionTag*, Handle obj, int32_t index, int32_t value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return;
    (*ptr)[index] = value;
}

void JArray_SetFlt(RE::StaticFunctionTag*, Handle obj, int32_t index, float value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return;
    (*ptr)[index] = value;
}

void JArray_SetStr(RE::StaticFunctionTag*, Handle obj, int32_t index, std::string value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return;
    (*ptr)[index] = value;
}

void JArray_SetObj(RE::StaticFunctionTag*, Handle obj, int32_t index, Handle container) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return;
    if (container == 0) {
        (*ptr)[index] = nullptr;
    }
    else {
        (*ptr)[index] = ObjectManager::MakeRef(container);
    }
}

void JArray_SetForm(RE::StaticFunctionTag*, Handle obj, int32_t index, RE::TESForm* value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return;
    if (!value) {
        (*ptr)[index] = nullptr;
    }
    else {
        (*ptr)[index] = FormSerializer::EncodeForm(value);
    }
}

void JArray_AddInt(RE::StaticFunctionTag*, Handle obj, int32_t value, int32_t addToIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (addToIndex < 0 || addToIndex >= static_cast<int32_t>(ptr->size())) {
        ptr->push_back(value);
    }
    else {
        ptr->insert(ptr->begin() + addToIndex, value);
    }
}

void JArray_AddFlt(RE::StaticFunctionTag*, Handle obj, float value, int32_t addToIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (addToIndex < 0 || addToIndex >= static_cast<int32_t>(ptr->size())) {
        ptr->push_back(value);
    }
    else {
        ptr->insert(ptr->begin() + addToIndex, value);
    }
}

void JArray_AddStr(RE::StaticFunctionTag*, Handle obj, std::string value, int32_t addToIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (addToIndex < 0 || addToIndex >= static_cast<int32_t>(ptr->size())) {
        ptr->push_back(value);
    }
    else {
        ptr->insert(ptr->begin() + addToIndex, value);
    }
}

void JArray_AddObj(RE::StaticFunctionTag*, Handle obj, Handle container, int32_t addToIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    json val = (container == 0) ? json(nullptr) : ObjectManager::MakeRef(container);
    if (addToIndex < 0 || addToIndex >= static_cast<int32_t>(ptr->size())) {
        ptr->push_back(val);
    }
    else {
        ptr->insert(ptr->begin() + addToIndex, val);
    }
}

void JArray_AddForm(RE::StaticFunctionTag*, Handle obj, RE::TESForm* value, int32_t addToIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    json formJson = value ? json(FormSerializer::EncodeForm(value)) : json(nullptr);
    if (addToIndex < 0 || addToIndex >= static_cast<int32_t>(ptr->size())) {
        ptr->push_back(formJson);
    }
    else {
        ptr->insert(ptr->begin() + addToIndex, formJson);
    }
}

int32_t JArray_Count(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return 0;
    return static_cast<int32_t>(ptr->size());
}

void JArray_Clear(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    ptr->clear();
}

void JArray_EraseIndex(RE::StaticFunctionTag*, Handle obj, int32_t index) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return;
    ptr->erase(ptr->begin() + index);
}

void JArray_EraseRange(RE::StaticFunctionTag*, Handle obj, int32_t first, int32_t last) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (first < 0) first = static_cast<int32_t>(ptr->size()) + first;
    if (last < 0) last = static_cast<int32_t>(ptr->size()) + last;
    if (first < 0) first = 0;
    if (last >= static_cast<int32_t>(ptr->size())) last = static_cast<int32_t>(ptr->size()) - 1;
    if (first > last) return;
    ptr->erase(ptr->begin() + first, ptr->begin() + last + 1);
}

int32_t JArray_EraseInteger(RE::StaticFunctionTag*, Handle obj, int32_t value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return 0;
    int32_t count = 0;
    for (auto it = ptr->begin(); it != ptr->end(); ) {
        if (it->is_number_integer() && it->get<int32_t>() == value) {
            it = ptr->erase(it);
            ++count;
        }
        else {
            ++it;
        }
    }
    return count;
}

int32_t JArray_EraseFloat(RE::StaticFunctionTag*, Handle obj, float value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return 0;
    int32_t count = 0;
    for (auto it = ptr->begin(); it != ptr->end(); ) {
        if (it->is_number() && it->get<float>() == value) {
            it = ptr->erase(it);
            ++count;
        }
        else {
            ++it;
        }
    }
    return count;
}

int32_t JArray_EraseString(RE::StaticFunctionTag*, Handle obj, std::string value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return 0;
    int32_t count = 0;
    for (auto it = ptr->begin(); it != ptr->end(); ) {
        if (it->is_string() && it->get<std::string>() == value) {
            it = ptr->erase(it);
            ++count;
        }
        else {
            ++it;
        }
    }
    return count;
}

int32_t JArray_EraseObject(RE::StaticFunctionTag*, Handle obj, Handle container) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return 0;
    int32_t count = 0;
    for (auto it = ptr->begin(); it != ptr->end(); ) {
        Handle h;
        if (ObjectManager::IsRef(*it, &h) && h == container) {
            it = ptr->erase(it);
            ++count;
        }
        else {
            ++it;
        }
    }
    return count;
}

int32_t JArray_EraseForm(RE::StaticFunctionTag*, Handle obj, RE::TESForm* value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array() || !value) return 0;
    std::string formStr = FormSerializer::EncodeForm(value);
    int32_t count = 0;
    for (auto it = ptr->begin(); it != ptr->end(); ) {
        if (it->is_string() && it->get<std::string>() == formStr) {
            it = ptr->erase(it);
            ++count;
        }
        else {
            ++it;
        }
    }
    return count;
}

int32_t JArray_ValueType(RE::StaticFunctionTag*, Handle obj, int32_t index) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return 0;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return 0;
    return GetValueType((*ptr)[index]);
}

void JArray_SwapItems(RE::StaticFunctionTag*, Handle obj, int32_t index1, int32_t index2) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (index1 < 0) index1 = static_cast<int32_t>(ptr->size()) + index1;
    if (index2 < 0) index2 = static_cast<int32_t>(ptr->size()) + index2;
    if (index1 < 0 || index1 >= static_cast<int32_t>(ptr->size())) return;
    if (index2 < 0 || index2 >= static_cast<int32_t>(ptr->size())) return;
    std::swap((*ptr)[index1], (*ptr)[index2]);
}

Handle JArray_Sort(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return obj;
    std::sort(ptr->begin(), ptr->end(), [](const json& a, const json& b) {
        int orderA = GetSortOrder(a);
        int orderB = GetSortOrder(b);
        if (orderA != orderB) return orderA < orderB;
        return a < b;
        });
    return obj;
}

Handle JArray_Unique(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return obj;
    JArray_Sort(nullptr, obj);
    auto it = std::unique(ptr->begin(), ptr->end(), [](const json& a, const json& b) {
        return a == b;
        });
    ptr->erase(it, ptr->end());
    return obj;
}

Handle JArray_Reverse(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return obj;
    std::reverse(ptr->begin(), ptr->end());
    return obj;
}

// PArray writes — reference_array auto-flushes changes back to the VM array
bool JArray_WriteToIntegerPArray(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackID, RE::StaticFunctionTag*, Handle obj, std::vector<int32_t> decoy, int32_t writeAtIdx, int32_t stopWriteAtIdx, int32_t readIdx, int32_t defaultRead) {
    return WritePArray(vm, stackID, obj, std::move(decoy), writeAtIdx, stopWriteAtIdx, readIdx, defaultRead, [](const json& j, int32_t def) {
        return j.is_number() ? static_cast<int32_t>(j.get<double>()) : def;
        });
}

bool JArray_WriteToFloatPArray(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackID, RE::StaticFunctionTag*, Handle obj, std::vector<float> decoy, int32_t writeAtIdx, int32_t stopWriteAtIdx, int32_t readIdx, float defaultRead) {
    return WritePArray(vm, stackID, obj, std::move(decoy), writeAtIdx, stopWriteAtIdx, readIdx, defaultRead, [](const json& j, float def) {
        return j.is_number() ? j.get<float>() : def;
        });
}

bool JArray_WriteToFormPArray(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackID, RE::StaticFunctionTag*, Handle obj, std::vector<RE::TESForm*> decoy, int32_t writeAtIdx, int32_t stopWriteAtIdx, int32_t readIdx, RE::TESForm* defaultRead) {
    return WritePArray(vm, stackID, obj, std::move(decoy), writeAtIdx, stopWriteAtIdx, readIdx, defaultRead, [](const json& j, RE::TESForm* def) {
        return j.is_string() ? FormSerializer::DecodeForm(j.get<std::string>()) : def;
        });
}

bool JArray_WriteToStringPArray(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackID, RE::StaticFunctionTag*, Handle obj, std::vector<std::string> decoy, int32_t writeAtIdx, int32_t stopWriteAtIdx, int32_t readIdx, std::string defaultRead) {
    return WritePArray(vm, stackID, obj, std::move(decoy), writeAtIdx, stopWriteAtIdx, readIdx, defaultRead, [](const json& j, const std::string& def) {
        return j.is_string() ? j.get<std::string>() : def;
        });
}

// ------------------------------------------------------------------
// JDB
// ------------------------------------------------------------------

float JDB_SolveFlt(RE::StaticFunctionTag*, std::string path, float defaultVal) {
    auto root = ObjectManager::Get().GetJDBRoot();
    return JValue_SolveFlt(nullptr, root, path, defaultVal);
}

int32_t JDB_SolveInt(RE::StaticFunctionTag*, std::string path, int32_t defaultVal) {
    auto root = ObjectManager::Get().GetJDBRoot();
    return JValue_SolveInt(nullptr, root, path, defaultVal);
}

std::string JDB_SolveStr(RE::StaticFunctionTag*, std::string path, std::string defaultVal) {
    auto root = ObjectManager::Get().GetJDBRoot();
    return JValue_SolveStr(nullptr, root, path, defaultVal);
}

Handle JDB_SolveObj(RE::StaticFunctionTag*, std::string path, Handle defaultVal) {
    auto root = ObjectManager::Get().GetJDBRoot();
    return JValue_SolveObj(nullptr, root, path, defaultVal);
}

RE::TESForm* JDB_SolveForm(RE::StaticFunctionTag*, std::string path, RE::TESForm* defaultVal) {
    auto root = ObjectManager::Get().GetJDBRoot();
    return JValue_SolveForm(nullptr, root, path, defaultVal);
}

bool JDB_SolveFltSetter(RE::StaticFunctionTag*, std::string path, float value, bool createMissing) {
    auto root = ObjectManager::Get().GetJDBRoot();
    return JValue_SolveFltSetter(nullptr, root, path, value, createMissing);
}

bool JDB_SolveIntSetter(RE::StaticFunctionTag*, std::string path, int32_t value, bool createMissing) {
    auto root = ObjectManager::Get().GetJDBRoot();
    return JValue_SolveIntSetter(nullptr, root, path, value, createMissing);
}

bool JDB_SolveStrSetter(RE::StaticFunctionTag*, std::string path, std::string value, bool createMissing) {
    auto root = ObjectManager::Get().GetJDBRoot();
    return JValue_SolveStrSetter(nullptr, root, path, value, createMissing);
}

bool JDB_SolveObjSetter(RE::StaticFunctionTag*, std::string path, Handle value, bool createMissing) {
    auto root = ObjectManager::Get().GetJDBRoot();
    return JValue_SolveObjSetter(nullptr, root, path, value, createMissing);
}

bool JDB_SolveFormSetter(RE::StaticFunctionTag*, std::string path, RE::TESForm* value, bool createMissing) {
    auto root = ObjectManager::Get().GetJDBRoot();
    return JValue_SolveFormSetter(nullptr, root, path, value, createMissing);
}

void JDB_SetObj(RE::StaticFunctionTag*, std::string key, Handle obj) {
    auto root = ObjectManager::Get().GetJDBRoot();
    auto ptr = ObjectManager::Get().GetObject(root);
    if (!ptr || !ptr->is_object()) return;
    if (obj == 0) {
        ptr->erase(key);
        return;
    }
    (*ptr)[key] = ObjectManager::MakeRef(obj);
}

bool JDB_HasPath(RE::StaticFunctionTag*, std::string path) {
    auto root = ObjectManager::Get().GetJDBRoot();
    return JValue_HasPath(nullptr, root, path);
}

Handle JDB_AllKeys(RE::StaticFunctionTag*) {
    auto root = ObjectManager::Get().GetJDBRoot();
    return JMap_AllKeys(nullptr, root);
}

Handle JDB_AllValues(RE::StaticFunctionTag*) {
    auto root = ObjectManager::Get().GetJDBRoot();
    return JMap_AllValues(nullptr, root);
}

void JDB_WriteToFile(RE::StaticFunctionTag*, std::string path) {
    auto root = ObjectManager::Get().GetJDBRoot();
    JValue_WriteToFile(nullptr, root, path);
}

Handle JDB_Root(RE::StaticFunctionTag*) {
    return ObjectManager::Get().GetJDBRoot();
}

// ------------------------------------------------------------------
// JFormDB
// ------------------------------------------------------------------

namespace {
    Handle GetJFormDBEntryRaw(RE::TESForm* fKey, const std::string& storageName) {
        if (!fKey) return 0;
        auto root = ObjectManager::Get().GetJDBRoot();
        auto ptr = ObjectManager::Get().GetObject(root);
        if (!ptr) return 0;

        auto path = std::format(".JFormDB.{}.{}", storageName, FormSerializer::EncodeForm(fKey));
        auto* target = ResolvePath(*ptr, path);
        if (!target) return 0;

        Handle h;
        if (ObjectManager::IsRef(*target, &h)) return h;

        if (target->is_object()) {
            Handle newHandle = ObjectManager::Get().RegisterObject(std::move(*target));
            *target = ObjectManager::MakeRef(newHandle);
            return newHandle;
        }
        return 0;
    }
}

void JFormDB_SetEntry(RE::StaticFunctionTag*, std::string storageName, RE::TESForm* fKey, Handle entry) {
    if (!fKey) return;
    auto root = ObjectManager::Get().GetJDBRoot();
    auto ptr = ObjectManager::Get().GetObject(root);
    if (!ptr) return;

    auto formStr = FormSerializer::EncodeForm(fKey);
    auto path = std::format(".JFormDB.{}.{}", storageName, formStr);

    if (entry == 0) {
        ErasePath(*ptr, path);
        return;
    }

    auto* target = ResolvePath(*ptr, path, true);
    if (!target) return;
    *target = ObjectManager::MakeRef(entry);
}

Handle JFormDB_MakeEntry(RE::StaticFunctionTag*, std::string storageName, RE::TESForm* fKey) {
    if (!fKey) return 0;
    auto root = ObjectManager::Get().GetJDBRoot();
    auto ptr = ObjectManager::Get().GetObject(root);
    if (!ptr) return 0;

    auto formStr = FormSerializer::EncodeForm(fKey);
    auto path = std::format(".JFormDB.{}.{}", storageName, formStr);

    auto* target = ResolvePath(*ptr, path);
    if (target) {
        Handle h;
        if (ObjectManager::IsRef(*target, &h)) return h;
        if (target->is_object()) {
            Handle newHandle = ObjectManager::Get().RegisterObject(std::move(*target));
            *target = ObjectManager::MakeRef(newHandle);
            return newHandle;
        }
    }

    Handle entry = ObjectManager::Get().CreateObject();
    auto* writeTarget = ResolvePath(*ptr, path, true);
    if (!writeTarget) return 0;
    *writeTarget = ObjectManager::MakeRef(entry);
    return entry;
}

Handle JFormDB_FindEntry(RE::StaticFunctionTag*, std::string storageName, RE::TESForm* fKey) {
    return GetJFormDBEntryRaw(fKey, storageName);
}

float JFormDB_SolveFlt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, float defaultVal) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return defaultVal;
    return JValue_SolveFlt(nullptr, entry, path, defaultVal);
}

int32_t JFormDB_SolveInt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, int32_t defaultVal) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return defaultVal;
    return JValue_SolveInt(nullptr, entry, path, defaultVal);
}

std::string JFormDB_SolveStr(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, std::string defaultVal) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return defaultVal;
    return JValue_SolveStr(nullptr, entry, path, defaultVal);
}

Handle JFormDB_SolveObj(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, Handle defaultVal) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return defaultVal;
    return JValue_SolveObj(nullptr, entry, path, defaultVal);
}

RE::TESForm* JFormDB_SolveForm(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, RE::TESForm* defaultVal) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return defaultVal;
    return JValue_SolveForm(nullptr, entry, path, defaultVal);
}

bool JFormDB_SolveFltSetter(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, float value, bool createMissing) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return false;
    return JValue_SolveFltSetter(nullptr, entry, path, value, createMissing);
}

bool JFormDB_SolveIntSetter(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, int32_t value, bool createMissing) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return false;
    return JValue_SolveIntSetter(nullptr, entry, path, value, createMissing);
}

bool JFormDB_SolveStrSetter(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, std::string value, bool createMissing) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return false;
    return JValue_SolveStrSetter(nullptr, entry, path, value, createMissing);
}

bool JFormDB_SolveObjSetter(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, Handle value, bool createMissing) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return false;
    return JValue_SolveObjSetter(nullptr, entry, path, value, createMissing);
}

bool JFormDB_SolveFormSetter(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, RE::TESForm* value, bool createMissing) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return false;
    return JValue_SolveFormSetter(nullptr, entry, path, value, createMissing);
}

bool JFormDB_HasPath(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return false;
    return JValue_HasPath(nullptr, entry, path);
}

Handle JFormDB_AllKeys(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string storageName) {
    auto entry = GetJFormDBEntryRaw(fKey, storageName);
    if (!entry) return 0;
    return JMap_AllKeys(nullptr, entry);
}

Handle JFormDB_AllValues(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string storageName) {
    auto entry = GetJFormDBEntryRaw(fKey, storageName);
    if (!entry) return 0;
    return JMap_AllValues(nullptr, entry);
}

int32_t JFormDB_GetInt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string key) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return 0;
    return JMap_GetInt(nullptr, entry, key, 0);
}

float JFormDB_GetFlt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string key) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return 0.0f;
    return JMap_GetFlt(nullptr, entry, key, 0.0f);
}

std::string JFormDB_GetStr(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string key) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return "";
    return JMap_GetStr(nullptr, entry, key, "");
}

Handle JFormDB_GetObj(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string key) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return 0;
    return JMap_GetObj(nullptr, entry, key, 0);
}

RE::TESForm* JFormDB_GetForm(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string key) {
    auto entry = GetJFormDBEntryRaw(fKey, "default");
    if (!entry) return nullptr;
    return JMap_GetForm(nullptr, entry, key, nullptr);
}

void JFormDB_SetInt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string key, int32_t value) {
    auto entry = JFormDB_MakeEntry(nullptr, "default", fKey);
    if (!entry) return;
    JMap_SetInt(nullptr, entry, key, value);
}

void JFormDB_SetFlt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string key, float value) {
    auto entry = JFormDB_MakeEntry(nullptr, "default", fKey);
    if (!entry) return;
    JMap_SetFlt(nullptr, entry, key, value);
}

void JFormDB_SetStr(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string key, std::string value) {
    auto entry = JFormDB_MakeEntry(nullptr, "default", fKey);
    if (!entry) return;
    JMap_SetStr(nullptr, entry, key, value);
}

void JFormDB_SetObj(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string key, Handle value) {
    auto entry = JFormDB_MakeEntry(nullptr, "default", fKey);
    if (!entry) return;
    JMap_SetObj(nullptr, entry, key, value);
}

void JFormDB_SetForm(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string key, RE::TESForm* value) {
    auto entry = JFormDB_MakeEntry(nullptr, "default", fKey);
    if (!entry) return;
    JMap_SetForm(nullptr, entry, key, value);
}

// ------------------------------------------------------------------
// JFormMap
// ------------------------------------------------------------------

Handle JFormMap_Object(RE::StaticFunctionTag*) {
    return ObjectManager::Get().CreateObject();
}

int32_t JFormMap_GetInt(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, int32_t defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return defaultVal;
    auto formStr = FormSerializer::EncodeForm(key);
    if (!ptr->contains(formStr)) return defaultVal;
    auto& val = (*ptr)[formStr];
    if (val.is_number_integer()) return val.get<int32_t>();
    if (val.is_number_float()) return static_cast<int32_t>(val.get<double>());
    return defaultVal;
}

float JFormMap_GetFlt(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, float defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return defaultVal;
    auto formStr = FormSerializer::EncodeForm(key);
    if (!ptr->contains(formStr)) return defaultVal;
    auto& val = (*ptr)[formStr];
    if (val.is_number()) return val.get<float>();
    return defaultVal;
}

std::string JFormMap_GetStr(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, std::string defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return defaultVal;
    auto formStr = FormSerializer::EncodeForm(key);
    if (!ptr->contains(formStr)) return defaultVal;
    auto& val = (*ptr)[formStr];
    if (val.is_string()) return val.get<std::string>();
    return defaultVal;
}

Handle JFormMap_GetObj(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, Handle defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return defaultVal;
    auto formStr = FormSerializer::EncodeForm(key);
    if (!ptr->contains(formStr)) return defaultVal;

    auto& val = (*ptr)[formStr];
    Handle refHandle;
    if (ObjectManager::IsRef(val, &refHandle)) return refHandle;

    if (val.is_object() || val.is_array()) {
        Handle newHandle = ObjectManager::Get().RegisterObject(std::move(val));
        val = ObjectManager::MakeRef(newHandle);
        return newHandle;
    }
    return defaultVal;
}

RE::TESForm* JFormMap_GetForm(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, RE::TESForm* defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return defaultVal;
    auto formStr = FormSerializer::EncodeForm(key);
    if (!ptr->contains(formStr)) return defaultVal;
    auto& val = (*ptr)[formStr];
    if (val.is_string()) {
        std::string str = val.get<std::string>();
        if (FormSerializer::IsFormString(str)) return FormSerializer::DecodeForm(str);
    }
    return defaultVal;
}

void JFormMap_SetInt(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, int32_t value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return;
    (*ptr)[FormSerializer::EncodeForm(key)] = value;
}

void JFormMap_SetFlt(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, float value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return;
    (*ptr)[FormSerializer::EncodeForm(key)] = value;
}

void JFormMap_SetStr(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, std::string value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return;
    (*ptr)[FormSerializer::EncodeForm(key)] = value;
}

void JFormMap_SetObj(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, Handle container) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return;
    if (container == 0) {
        ptr->erase(FormSerializer::EncodeForm(key));
        return;
    }
    (*ptr)[FormSerializer::EncodeForm(key)] = ObjectManager::MakeRef(container);
}

void JFormMap_SetForm(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, RE::TESForm* value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return;
    if (!value) {
        ptr->erase(FormSerializer::EncodeForm(key));
        return;
    }
    (*ptr)[FormSerializer::EncodeForm(key)] = value ? json(FormSerializer::EncodeForm(value)) : json(nullptr);
}

bool JFormMap_HasKey(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return false;
    return ptr->contains(FormSerializer::EncodeForm(key));
}

int32_t JFormMap_ValueType(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return 0;
    auto formStr = FormSerializer::EncodeForm(key);
    if (!ptr->contains(formStr)) return 0;
    return GetValueType((*ptr)[formStr]);
}

Handle JFormMap_AllKeys(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    Handle arr = ObjectManager::Get().CreateArray();
    auto arrPtr = ObjectManager::Get().GetObject(arr);
    for (auto& [key, val] : ptr->items()) {
        auto form = FormSerializer::DecodeForm(key);
        if (form) arrPtr->push_back(FormSerializer::EncodeForm(form));
    }
    return arr;
}

std::vector<RE::TESForm*> JFormMap_AllKeysPArray(RE::StaticFunctionTag*, Handle obj) {
    std::vector<RE::TESForm*> result;
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return result;
    for (auto& [key, val] : ptr->items()) {
        auto form = FormSerializer::DecodeForm(key);
        if (form) result.push_back(form);
    }
    return result;
}

Handle JFormMap_AllValues(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    Handle arr = ObjectManager::Get().CreateArray();
    auto arrPtr = ObjectManager::Get().GetObject(arr);
    for (auto& [key, val] : ptr->items()) {
        arrPtr->push_back(val);
    }
    return arr;
}

bool JFormMap_RemoveKey(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return false;
    return ptr->erase(FormSerializer::EncodeForm(key)) > 0;
}

int32_t JFormMap_Count(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    return static_cast<int32_t>(ptr->size());
}

void JFormMap_Clear(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    ptr->clear();
}

void JFormMap_AddPairs(RE::StaticFunctionTag*, Handle obj, Handle source, bool overrideDuplicates) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    auto srcPtr = ObjectManager::Get().GetObject(source);
    if (!ptr || !ptr->is_object() || !srcPtr || !srcPtr->is_object()) return;
    for (auto& [key, val] : srcPtr->items()) {
        if (!overrideDuplicates && ptr->contains(key)) continue;
        (*ptr)[key] = val;
    }
}

RE::TESForm* JFormMap_NextKey(RE::StaticFunctionTag*, Handle obj, RE::TESForm* previousKey, RE::TESForm* endKey) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return endKey;

    std::string prevStr = previousKey ? FormSerializer::EncodeForm(previousKey) : "";
    std::string endStr = endKey ? FormSerializer::EncodeForm(endKey) : "";

    if (prevStr == endStr) {
        if (ptr->empty()) return endKey;
        return FormSerializer::DecodeForm(ptr->begin().key());
    }

    bool found = false;
    for (auto& [key, val] : ptr->items()) {
        if (found) return FormSerializer::DecodeForm(key);
        if (key == prevStr) found = true;
    }
    return endKey;
}

RE::TESForm* JFormMap_GetNthKey(RE::StaticFunctionTag*, Handle obj, int32_t keyIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return nullptr;
    if (keyIndex < 0) keyIndex = static_cast<int32_t>(ptr->size()) + keyIndex;
    if (keyIndex < 0 || keyIndex >= static_cast<int32_t>(ptr->size())) return nullptr;
    auto it = ptr->begin();
    std::advance(it, keyIndex);
    return FormSerializer::DecodeForm(it.key());
}

// ------------------------------------------------------------------
// JIntMap
// ------------------------------------------------------------------

Handle JIntMap_Object(RE::StaticFunctionTag*) {
    return ObjectManager::Get().CreateObject();
}

int32_t JIntMap_GetInt(RE::StaticFunctionTag*, Handle obj, int32_t key, int32_t defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return defaultVal;
    auto keyStr = std::to_string(key);
    if (!ptr->contains(keyStr)) return defaultVal;
    auto& val = (*ptr)[keyStr];
    if (val.is_number_integer()) return val.get<int32_t>();
    if (val.is_number_float()) return static_cast<int32_t>(val.get<double>());
    return defaultVal;
}

float JIntMap_GetFlt(RE::StaticFunctionTag*, Handle obj, int32_t key, float defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return defaultVal;
    auto keyStr = std::to_string(key);
    if (!ptr->contains(keyStr)) return defaultVal;
    auto& val = (*ptr)[keyStr];
    if (val.is_number()) return val.get<float>();
    return defaultVal;
}

std::string JIntMap_GetStr(RE::StaticFunctionTag*, Handle obj, int32_t key, std::string defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return defaultVal;
    auto keyStr = std::to_string(key);
    if (!ptr->contains(keyStr)) return defaultVal;
    auto& val = (*ptr)[keyStr];
    if (val.is_string()) return val.get<std::string>();
    return defaultVal;
}

Handle JIntMap_GetObj(RE::StaticFunctionTag*, Handle obj, int32_t key, Handle defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return defaultVal;
    auto keyStr = std::to_string(key);
    if (!ptr->contains(keyStr)) return defaultVal;

    auto& val = (*ptr)[keyStr];
    Handle refHandle;
    if (ObjectManager::IsRef(val, &refHandle)) return refHandle;

    if (val.is_object() || val.is_array()) {
        Handle newHandle = ObjectManager::Get().RegisterObject(std::move(val));
        val = ObjectManager::MakeRef(newHandle);
        return newHandle;
    }
    return defaultVal;
}

RE::TESForm* JIntMap_GetForm(RE::StaticFunctionTag*, Handle obj, int32_t key, RE::TESForm* defaultVal) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return defaultVal;
    auto keyStr = std::to_string(key);
    if (!ptr->contains(keyStr)) return defaultVal;
    auto& val = (*ptr)[keyStr];
    if (val.is_string()) {
        std::string str = val.get<std::string>();
        if (FormSerializer::IsFormString(str)) return FormSerializer::DecodeForm(str);
    }
    return defaultVal;
}

void JIntMap_SetInt(RE::StaticFunctionTag*, Handle obj, int32_t key, int32_t value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    (*ptr)[std::to_string(key)] = value;
}

void JIntMap_SetFlt(RE::StaticFunctionTag*, Handle obj, int32_t key, float value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    (*ptr)[std::to_string(key)] = value;
}

void JIntMap_SetStr(RE::StaticFunctionTag*, Handle obj, int32_t key, std::string value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    (*ptr)[std::to_string(key)] = value;
}

void JIntMap_SetObj(RE::StaticFunctionTag*, Handle obj, int32_t key, Handle container) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    if (container == 0) {
        ptr->erase(std::to_string(key));
        return;
    }
    (*ptr)[std::to_string(key)] = ObjectManager::MakeRef(container);
}

void JIntMap_SetForm(RE::StaticFunctionTag*, Handle obj, int32_t key, RE::TESForm* value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    if (!value) {
        ptr->erase(std::to_string(key));
        return;
    }
    (*ptr)[std::to_string(key)] = FormSerializer::EncodeForm(value);
}

bool JIntMap_HasKey(RE::StaticFunctionTag*, Handle obj, int32_t key) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return false;
    return ptr->contains(std::to_string(key));
}

int32_t JIntMap_ValueType(RE::StaticFunctionTag*, Handle obj, int32_t key) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    auto keyStr = std::to_string(key);
    if (!ptr->contains(keyStr)) return 0;
    return GetValueType((*ptr)[keyStr]);
}

Handle JIntMap_AllKeys(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    Handle arr = ObjectManager::Get().CreateArray();
    auto arrPtr = ObjectManager::Get().GetObject(arr);
    for (auto& [key, val] : ptr->items()) {
        arrPtr->push_back(std::stoi(key));
    }
    return arr;
}

std::vector<int32_t> JIntMap_AllKeysPArray(RE::StaticFunctionTag*, Handle obj) {
    std::vector<int32_t> result;
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return result;
    for (auto& [key, val] : ptr->items()) {
        result.push_back(std::stoi(key));
    }
    return result;
}

Handle JIntMap_AllValues(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    Handle arr = ObjectManager::Get().CreateArray();
    auto arrPtr = ObjectManager::Get().GetObject(arr);
    for (auto& [key, val] : ptr->items()) {
        arrPtr->push_back(val);
    }
    return arr;
}

bool JIntMap_RemoveKey(RE::StaticFunctionTag*, Handle obj, int32_t key) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return false;
    return ptr->erase(std::to_string(key)) > 0;
}

int32_t JIntMap_Count(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    return static_cast<int32_t>(ptr->size());
}

void JIntMap_Clear(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    ptr->clear();
}

void JIntMap_AddPairs(RE::StaticFunctionTag*, Handle obj, Handle source, bool overrideDuplicates) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    auto srcPtr = ObjectManager::Get().GetObject(source);
    if (!ptr || !ptr->is_object() || !srcPtr || !srcPtr->is_object()) return;
    for (auto& [key, val] : srcPtr->items()) {
        if (!overrideDuplicates && ptr->contains(key)) continue;
        (*ptr)[key] = val;
    }
}

int32_t JIntMap_NextKey(RE::StaticFunctionTag*, Handle obj, int32_t previousKey, int32_t endKey) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return endKey;
    if (previousKey == endKey) {
        if (ptr->empty()) return endKey;
        return std::stoi(ptr->begin().key());
    }
    bool found = false;
    for (auto& [key, val] : ptr->items()) {
        int32_t k = std::stoi(key);
        if (found) return k;
        if (k == previousKey) found = true;
    }
    return endKey;
}

int32_t JIntMap_GetNthKey(RE::StaticFunctionTag*, Handle obj, int32_t keyIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    if (keyIndex < 0) keyIndex = static_cast<int32_t>(ptr->size()) + keyIndex;
    if (keyIndex < 0 || keyIndex >= static_cast<int32_t>(ptr->size())) return 0;
    auto it = ptr->begin();
    std::advance(it, keyIndex);
    return std::stoi(it.key());
}

// ------------------------------------------------------------------
// JString
// ------------------------------------------------------------------

int32_t JString_Wrap(RE::StaticFunctionTag*, std::string sourceText, int32_t charactersPerLine) {
    Handle arr = ObjectManager::Get().CreateArray();
    auto ptr = ObjectManager::Get().GetObject(arr);
    if (!ptr) return 0;

    std::string currentLine;
    std::istringstream iss(sourceText);
    std::string word;

    while (iss >> word) {
        if (currentLine.empty()) {
            currentLine = word;
        }
        else if (currentLine.length() + 1 + word.length() <= static_cast<size_t>(charactersPerLine)) {
            currentLine += ' ' + word;
        }
        else {
            ptr->push_back(currentLine);
            currentLine = word;
        }
    }
    if (!currentLine.empty()) ptr->push_back(currentLine);
    return arr;
}

int32_t JString_DecodeFormStringToFormId(RE::StaticFunctionTag*, std::string formString) {
    auto form = FormSerializer::DecodeForm(formString);
    return form ? static_cast<int32_t>(form->GetFormID()) : 0;
}

RE::TESForm* JString_DecodeFormStringToForm(RE::StaticFunctionTag*, std::string formString) {
    return FormSerializer::DecodeForm(formString);
}

std::string JString_EncodeFormToString(RE::StaticFunctionTag*, RE::TESForm* value) {
    if (!value) return "";
    return FormSerializer::EncodeForm(value);
}

std::string JString_EncodeFormIdToString(RE::StaticFunctionTag*, int32_t formId) {
    auto form = RE::TESForm::LookupByID(static_cast<RE::FormID>(formId));
    if (!form) return "";
    return FormSerializer::EncodeForm(form);
}

std::string JString_GenerateUUID(RE::StaticFunctionTag*) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    const char* hex = "0123456789abcdef";
    std::string uuid;
    uuid.reserve(36);

    for (int i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            uuid += '-';
        }
        else if (i == 14) {
            uuid += '4';
        }
        else if (i == 19) {
            uuid += hex[dis2(gen)];
        }
        else {
            uuid += hex[dis(gen)];
        }
    }
    return uuid;
}

// ------------------------------------------------------------------
// JContainers utility
// ------------------------------------------------------------------

bool JContainers_IsInstalled(RE::StaticFunctionTag*) {
    return true;
}

int32_t JContainers_APIVersion(RE::StaticFunctionTag*) {
    return 4;
}

int32_t JContainers_FeatureVersion(RE::StaticFunctionTag*) {
    return 2;
}

bool JContainers_FileExistsAtPath(RE::StaticFunctionTag*, std::string path) {
    return std::filesystem::exists(path);
}

std::vector<std::string> JContainers_ContentsOfDirectoryAtPath(RE::StaticFunctionTag*, std::string directoryPath, std::string extension) {
    std::vector<std::string> result;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            if (!ext.empty() && ext[0] == '.') ext = ext.substr(1);
            if (!extension.empty() && ext != extension) continue;
            result.push_back(entry.path().string());
        }
    }
    catch (...) {}
    return result;
}

void JContainers_RemoveFileAtPath(RE::StaticFunctionTag*, std::string path) {
    try {
        std::filesystem::remove_all(path);
    }
    catch (...) {}
}

std::string JContainers_UserDirectory(RE::StaticFunctionTag*) {
    return "Data/SKSE/Plugins/JContainersNG/";
}