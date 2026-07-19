//JContainersNG_Natives_JDB.cpp
#include "JContainersNG_Natives.hpp"

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

void JDB_SetObj(RE::StaticFunctionTag*, std::string path, Handle obj) {
    auto root = ObjectManager::Get().GetJDBRoot();
    auto ptr = ObjectManager::Get().GetObject(root);
    if (!ptr || !ptr->is_object()) return;

    if (obj == 0) {
        ErasePath(*ptr, path); // releases the edge internally now
        return;
    }

    auto* target = ResolvePath(*ptr, path, true);
    if (!target) return;
    ObjectManager::Get().EmbedEdge(*target, obj);
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