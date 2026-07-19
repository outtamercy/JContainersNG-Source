//JContainersNG_Natives_JFormDB.cpp
#include "JContainersNG_Natives.hpp"

namespace {
    struct JFormDBPath {
        std::string storage;
        std::string rest;
    };

    JFormDBPath ParseJFormDBPath(const std::string& path, bool isPath) {
        JFormDBPath result;
        result.storage = "default";
        result.rest = path;

        if (path.empty() || path[0] != '.') return result;

        size_t nextDot = path.find('.', 1);
        if (nextDot == std::string::npos) {
            result.storage = path.substr(1);
            result.rest = "";
            return result;
        }

        result.storage = path.substr(1, nextDot - 1);
        result.rest = isPath ? path.substr(nextDot) : path.substr(nextDot + 1);
        return result;
    }

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
            // promoted inline json becomes a real edge on the parent
            ObjectManager::Get().EmbedEdge(*target, newHandle);
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
        ErasePath(*ptr, path); // releases the edge internally now
        return;
    }

    auto* target = ResolvePath(*ptr, path, true);
    if (!target) return;
    ObjectManager::Get().EmbedEdge(*target, entry);
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
            ObjectManager::Get().EmbedEdge(*target, newHandle);
            return newHandle;
        }
    }

    Handle entry = ObjectManager::Get().CreateObject();
    auto* writeTarget = ResolvePath(*ptr, path, true);
    if (!writeTarget) return 0;
    ObjectManager::Get().EmbedEdge(*writeTarget, entry);
    return entry;
}

Handle JFormDB_FindEntry(RE::StaticFunctionTag*, std::string storageName, RE::TESForm* fKey) {
    return GetJFormDBEntryRaw(fKey, storageName);
}

float JFormDB_SolveFlt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, float defaultVal) {
    auto parsed = ParseJFormDBPath(path, true);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return defaultVal;
    return JValue_SolveFlt(nullptr, entry, parsed.rest, defaultVal);
}

int32_t JFormDB_SolveInt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, int32_t defaultVal) {
    auto parsed = ParseJFormDBPath(path, true);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return defaultVal;
    return JValue_SolveInt(nullptr, entry, parsed.rest, defaultVal);
}

std::string JFormDB_SolveStr(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, std::string defaultVal) {
    auto parsed = ParseJFormDBPath(path, true);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return defaultVal;
    return JValue_SolveStr(nullptr, entry, parsed.rest, defaultVal);
}

Handle JFormDB_SolveObj(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, Handle defaultVal) {
    auto parsed = ParseJFormDBPath(path, true);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return defaultVal;
    return JValue_SolveObj(nullptr, entry, parsed.rest, defaultVal);
}

RE::TESForm* JFormDB_SolveForm(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, RE::TESForm* defaultVal) {
    auto parsed = ParseJFormDBPath(path, true);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return defaultVal;
    return JValue_SolveForm(nullptr, entry, parsed.rest, defaultVal);
}

bool JFormDB_SolveFltSetter(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, float value, bool createMissing) {
    auto parsed = ParseJFormDBPath(path, true);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return false;
    return JValue_SolveFltSetter(nullptr, entry, parsed.rest, value, createMissing);
}

bool JFormDB_SolveIntSetter(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, int32_t value, bool createMissing) {
    auto parsed = ParseJFormDBPath(path, true);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return false;
    return JValue_SolveIntSetter(nullptr, entry, parsed.rest, value, createMissing);
}

bool JFormDB_SolveStrSetter(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, std::string value, bool createMissing) {
    auto parsed = ParseJFormDBPath(path, true);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return false;
    return JValue_SolveStrSetter(nullptr, entry, parsed.rest, value, createMissing);
}

bool JFormDB_SolveObjSetter(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, Handle value, bool createMissing) {
    auto parsed = ParseJFormDBPath(path, true);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return false;
    return JValue_SolveObjSetter(nullptr, entry, parsed.rest, value, createMissing);
}

bool JFormDB_SolveFormSetter(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, RE::TESForm* value, bool createMissing) {
    auto parsed = ParseJFormDBPath(path, true);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return false;
    return JValue_SolveFormSetter(nullptr, entry, parsed.rest, value, createMissing);
}

bool JFormDB_HasPath(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path) {
    auto parsed = ParseJFormDBPath(path, true);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return false;
    return JValue_HasPath(nullptr, entry, parsed.rest);
}

Handle JFormDB_AllKeys(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path) {
    auto parsed = ParseJFormDBPath(path, false);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return 0;
    return JMap_AllKeys(nullptr, entry);
}

Handle JFormDB_AllValues(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path) {
    auto parsed = ParseJFormDBPath(path, false);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return 0;
    return JMap_AllValues(nullptr, entry);
}

int32_t JFormDB_GetInt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path) {
    auto parsed = ParseJFormDBPath(path, false);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return 0;
    return JMap_GetInt(nullptr, entry, parsed.rest, 0);
}

float JFormDB_GetFlt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path) {
    auto parsed = ParseJFormDBPath(path, false);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return 0.0f;
    return JMap_GetFlt(nullptr, entry, parsed.rest, 0.0f);
}

std::string JFormDB_GetStr(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path) {
    auto parsed = ParseJFormDBPath(path, false);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return "";
    return JMap_GetStr(nullptr, entry, parsed.rest, "");
}

Handle JFormDB_GetObj(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path) {
    auto parsed = ParseJFormDBPath(path, false);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return 0;
    return JMap_GetObj(nullptr, entry, parsed.rest, 0);
}

RE::TESForm* JFormDB_GetForm(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path) {
    auto parsed = ParseJFormDBPath(path, false);
    auto entry = GetJFormDBEntryRaw(fKey, parsed.storage);
    if (!entry) return nullptr;
    return JMap_GetForm(nullptr, entry, parsed.rest, nullptr);
}

void JFormDB_SetInt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, int32_t value) {
    auto parsed = ParseJFormDBPath(path, false);
    auto entry = JFormDB_MakeEntry(nullptr, parsed.storage, fKey);
    if (!entry) return;
    JMap_SetInt(nullptr, entry, parsed.rest, value);
}

void JFormDB_SetFlt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, float value) {
    auto parsed = ParseJFormDBPath(path, false);
    auto entry = JFormDB_MakeEntry(nullptr, parsed.storage, fKey);
    if (!entry) return;
    JMap_SetFlt(nullptr, entry, parsed.rest, value);
}

void JFormDB_SetStr(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, std::string value) {
    auto parsed = ParseJFormDBPath(path, false);
    auto entry = JFormDB_MakeEntry(nullptr, parsed.storage, fKey);
    if (!entry) return;
    JMap_SetStr(nullptr, entry, parsed.rest, value);
}

void JFormDB_SetObj(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, Handle value) {
    auto parsed = ParseJFormDBPath(path, false);
    auto entry = JFormDB_MakeEntry(nullptr, parsed.storage, fKey);
    if (!entry) return;
    JMap_SetObj(nullptr, entry, parsed.rest, value);
}

void JFormDB_SetForm(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, RE::TESForm* value) {
    auto parsed = ParseJFormDBPath(path, false);
    auto entry = JFormDB_MakeEntry(nullptr, parsed.storage, fKey);
    if (!entry) return;
    JMap_SetForm(nullptr, entry, parsed.rest, value);
}