//JContainersNG_Internal.hpp
#pragma once

#include <SKSE/SKSE.h>
#include <RE/P/PackUnpack.h>
#include <RE/V/VirtualMachine.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <string_view>
#include <functional>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <random>
#include <shlobj.h>
#include <combaseapi.h>

#include "ObjectManager.hpp"
#include "FormSerializer.hpp"
#include "JsonSerializer.hpp"

using json = nlohmann::json;
using Handle = ObjectManager::Handle;

// Lua backend forward declarations
struct lua_State;
namespace JContainersNG::Lua {
    json PullJsonFromLua(lua_State* L, int idx);
    bool EvaluateLuaExpression(const std::string& luaCode, ObjectManager::Handle transportHandle, std::function<void(lua_State*)> extractResult);
}

// --- shared state ---
extern bool g_jcApiLog;
extern std::mutex g_poolMutex;
extern std::unordered_map<std::string, std::vector<Handle>> g_pools;
extern std::mutex g_tagMutex;
extern std::unordered_map<std::string, std::unordered_set<Handle>> g_tags;

// --- case-insensitive key finders ---
json::iterator FindKeyCI(json& j, const std::string& key);
json::iterator FindKeyCI(json& j, std::string_view key);

// --- path resolution ---
json* ResolvePath(json& root, const std::string& path, bool createMissing = false);
json* DerefFinal(json* raw);
int32_t GetValueType(const json& val);
bool ErasePath(json& root, const std::string& path);

// --- sort ---
int GetSortOrder(const json& j);

// --- PArray helpers ---
template <typename T>
void NativeWriteElement(RE::BSScript::Variable& target, T val, RE::BSScript::Internal::VirtualMachine*) {
    RE::BSScript::PackValue(&target, val);
}

inline void NativeWriteElement(RE::BSScript::Variable& target, RE::TESForm* form, RE::BSScript::Internal::VirtualMachine*) {
    if (form) {
        RE::BSScript::PackValue(&target, form);
    }
    else {
        target.SetNone();
    }
}

template <typename T, typename JExtractor>
bool WritePArray(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackID, Handle obj, std::vector<T> decoy, int32_t writeAtIdx, int32_t stopWriteAtIdx, int32_t readIdx, T defaultRead, JExtractor extract) {
    (void)decoy;
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return false;

    auto* internalVM = static_cast<RE::BSScript::Internal::VirtualMachine*>(vm);
    auto it = internalVM->allRunningStacks.find(stackID);
    if (it == internalVM->allRunningStacks.end()) return false;

    auto* stack = it->second.get();
    if (!stack || !stack->top) return false;

    auto page = stack->top->GetPageForFrame();
    auto& targetVar = stack->top->GetStackFrameVariable(0, page);
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
        NativeWriteElement(pArray->data()[w], val, internalVM);
    }
    return true;
}
// --- OG item semantics -------------------------------------------------

// OG reads coerce numbers: getInt on a float truncates, getFlt on an int
// converts. anything else (or non-number) hands back the default
inline int32_t JCGetInt(const json& val, int32_t defaultVal = 0) {
    if (val.is_number_integer()) return val.get<int32_t>();
    if (val.is_number_float()) return static_cast<int32_t>(val.get<double>());
    return defaultVal;
}

inline float JCGetFlt(const json& val, float defaultVal = 0.0f) {
    if (val.is_number_float()) return val.get<float>();
    if (val.is_number_integer()) return static_cast<float>(val.get<int32_t>());
    return defaultVal;
}

// OG string items compare case-INSENSITIVE (_stricmp) — equality, sort, unique
inline bool JCStrEqualsCI(const std::string& a, const std::string& b) {
    return _stricmp(a.c_str(), b.c_str()) == 0;
}

// OG item equality: same types only, strings case-insensitive, refs by handle.
// numbers do NOT cross-compare (int 3 != float 3.0 in strict equality land)
inline bool JCItemEquals(const json& a, const json& b) {
    if (a.is_string() && b.is_string()) {
        return JCStrEqualsCI(a.get_ref<const std::string&>(), b.get_ref<const std::string&>());
    }
    ObjectManager::Handle ha, hb;
    const bool aRef = ObjectManager::IsRef(a, &ha);
    const bool bRef = ObjectManager::IsRef(b, &hb);
    if (aRef || bRef) {
        return aRef && bRef && ha == hb;
    }
    if (a.is_number_integer() && b.is_number_integer()) return a.get<int32_t>() == b.get<int32_t>();
    if (a.is_number_float() && b.is_number_float()) return a.get<double>() == b.get<double>();
    return a == b; // null/bool/whatever — exact match
}

// OG sort ordering: by type rank first (none < int < real < form < object <
// string), then by value inside the rank. strings case-insensitive. forms are
// strings in our world, so we sniff the prefix to give em their own rank
inline int JCItemTypeRank(const json& val) {
    ObjectManager::Handle h;
    if (ObjectManager::IsRef(val, &h)) return 4; // object
    if (val.is_null()) return 0;
    if (val.is_number_integer()) return 1;
    if (val.is_number_float()) return 2;
    if (val.is_string()) {
        return FormSerializer::IsFormString(val.get_ref<const std::string&>()) ? 3 : 5;
    }
    return 0; // arrays/objects inline — shouldnt happen, treat as none
}

inline bool JCItemLessOG(const json& a, const json& b) {
    const int ra = JCItemTypeRank(a);
    const int rb = JCItemTypeRank(b);
    if (ra != rb) return ra < rb;

    if (a.is_number_integer() && b.is_number_integer()) return a.get<int32_t>() < b.get<int32_t>();
    if (a.is_number_float() && b.is_number_float()) return a.get<double>() < b.get<double>();
    if (a.is_string() && b.is_string()) {
        return _stricmp(a.get_ref<const std::string&>().c_str(),
            b.get_ref<const std::string&>().c_str()) < 0;
    }
    ObjectManager::Handle ha, hb;
    if (ObjectManager::IsRef(a, &ha) && ObjectManager::IsRef(b, &hb)) return ha < hb;
    return false; // equal ranks, equal-ish — stable sort keeps em put
}