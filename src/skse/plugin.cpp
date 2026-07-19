//Plugin.cpp
#include <SKSE/SKSE.h>
#include "ObjectManager.hpp"
#include "JContainersNG_Natives.hpp"

using Handle = ObjectManager::Handle;

// Serialization callbacks
extern void SaveCallback(SKSE::SerializationInterface*);
extern void LoadCallback(SKSE::SerializationInterface*);
extern void RevertCallback(SKSE::SerializationInterface*);

bool RegisterJContainersFunctions(RE::BSScript::IVirtualMachine* vm) {
    if (!vm) return false;

// JMap
    vm->RegisterFunction("object", "JMap", JMap_Object, false);
    vm->RegisterFunction("getInt", "JMap", JMap_GetInt, false);
    vm->RegisterFunction("getFlt", "JMap", JMap_GetFlt, false);
    vm->RegisterFunction("getStr", "JMap", JMap_GetStr, false);
    vm->RegisterFunction("getObj", "JMap", JMap_GetObj, false);
    vm->RegisterFunction("getForm", "JMap", JMap_GetForm, false);
    vm->RegisterFunction("setInt", "JMap", JMap_SetInt, false);
    vm->RegisterFunction("setFlt", "JMap", JMap_SetFlt, false);
    vm->RegisterFunction("setStr", "JMap", JMap_SetStr, false);
    vm->RegisterFunction("setObj", "JMap", JMap_SetObj, false);
    vm->RegisterFunction("setForm", "JMap", JMap_SetForm, false);
    vm->RegisterFunction("hasKey", "JMap", JMap_HasKey, false);
    vm->RegisterFunction("valueType", "JMap", JMap_ValueType, false);
    vm->RegisterFunction("allKeys", "JMap", JMap_AllKeys, false);
    vm->RegisterFunction("allKeysPArray", "JMap", JMap_AllKeysPArray, false);
    vm->RegisterFunction("allValues", "JMap", JMap_AllValues, false);
    vm->RegisterFunction("removeKey", "JMap", JMap_RemoveKey, false);
    vm->RegisterFunction("count", "JMap", JMap_Count, false);
    vm->RegisterFunction("clear", "JMap", JMap_Clear, false);
    vm->RegisterFunction("addPairs", "JMap", JMap_AddPairs, false);
    vm->RegisterFunction("nextKey", "JMap", JMap_NextKey, false);
    vm->RegisterFunction("getNthKey", "JMap", JMap_GetNthKey, false);
    vm->RegisterFunction("insertInt", "JMap", JMap_InsertInt, false);
    vm->RegisterFunction("insertFlt", "JMap", JMap_InsertFlt, false);
    vm->RegisterFunction("insertStr", "JMap", JMap_InsertStr, false);
    vm->RegisterFunction("insertObj", "JMap", JMap_InsertObj, false);
    vm->RegisterFunction("insertForm", "JMap", JMap_InsertForm, false);

    // JAtomic
    vm->RegisterFunction("fetchAddInt", "JAtomic", JAtomic_fetchAddInt, false);
    vm->RegisterFunction("fetchAddFlt", "JAtomic", JAtomic_fetchAddFlt, false);
    vm->RegisterFunction("fetchMultInt", "JAtomic", JAtomic_fetchMultInt, false);
    vm->RegisterFunction("fetchMultFlt", "JAtomic", JAtomic_fetchMultFlt, false);
    vm->RegisterFunction("fetchModInt", "JAtomic", JAtomic_fetchModInt, false);
    vm->RegisterFunction("fetchDivInt", "JAtomic", JAtomic_fetchDivInt, false);
    vm->RegisterFunction("fetchDivFlt", "JAtomic", JAtomic_fetchDivFlt, false);
    vm->RegisterFunction("fetchAndInt", "JAtomic", JAtomic_fetchAndInt, false);
    vm->RegisterFunction("fetchXorInt", "JAtomic", JAtomic_fetchXorInt, false);
    vm->RegisterFunction("fetchOrInt", "JAtomic", JAtomic_fetchOrInt, false);
    vm->RegisterFunction("exchangeInt", "JAtomic", JAtomic_exchangeInt, false);
    vm->RegisterFunction("exchangeFlt", "JAtomic", JAtomic_exchangeFlt, false);
    vm->RegisterFunction("exchangeStr", "JAtomic", JAtomic_exchangeStr, false);
    vm->RegisterFunction("exchangeForm", "JAtomic", JAtomic_exchangeForm, false);
    vm->RegisterFunction("exchangeObj", "JAtomic", JAtomic_exchangeObj, false);
    vm->RegisterFunction("compareExchangeInt", "JAtomic", JAtomic_compareExchangeInt, false);
    vm->RegisterFunction("compareExchangeFlt", "JAtomic", JAtomic_compareExchangeFlt, false);
    vm->RegisterFunction("compareExchangeStr", "JAtomic", JAtomic_compareExchangeStr, false);
    vm->RegisterFunction("compareExchangeForm", "JAtomic", JAtomic_compareExchangeForm, false);
    vm->RegisterFunction("compareExchangeObj", "JAtomic", JAtomic_compareExchangeObj, false);

    // JArray
    vm->RegisterFunction("object", "JArray", JArray_Object, false);
    vm->RegisterFunction("objectWithSize", "JArray", JArray_ObjectWithSize, false);
    vm->RegisterFunction("objectWithInts", "JArray", JArray_ObjectWithInts, false);
    vm->RegisterFunction("objectWithStrings", "JArray", JArray_ObjectWithStrings, false);
    vm->RegisterFunction("objectWithFloats", "JArray", JArray_ObjectWithFloats, false);
    vm->RegisterFunction("objectWithBooleans", "JArray", JArray_ObjectWithBooleans, false);
    vm->RegisterFunction("objectWithForms", "JArray", JArray_ObjectWithForms, false);
    vm->RegisterFunction("subArray", "JArray", JArray_SubArray, false);
    vm->RegisterFunction("addFromArray", "JArray", JArray_AddFromArray, false);
    vm->RegisterFunction("addFromFormList", "JArray", JArray_AddFromFormList, false);
    vm->RegisterFunction("getInt", "JArray", JArray_GetInt, false);
    vm->RegisterFunction("getFlt", "JArray", JArray_GetFlt, false);
    vm->RegisterFunction("getStr", "JArray", JArray_GetStr, false);
    vm->RegisterFunction("getObj", "JArray", JArray_GetObj, false);
    vm->RegisterFunction("getForm", "JArray", JArray_GetForm, false);
    vm->RegisterFunction("asIntArray", "JArray", JArray_AsIntArray, false);
    vm->RegisterFunction("asFloatArray", "JArray", JArray_AsFloatArray, false);
    vm->RegisterFunction("asStringArray", "JArray", JArray_AsStringArray, false);
    vm->RegisterFunction("asFormArray", "JArray", JArray_AsFormArray, false);
    vm->RegisterFunction("findInt", "JArray", JArray_FindInt, false);
    vm->RegisterFunction("findFlt", "JArray", JArray_FindFlt, false);
    vm->RegisterFunction("findStr", "JArray", JArray_FindStr, false);
    vm->RegisterFunction("findObj", "JArray", JArray_FindObj, false);
    vm->RegisterFunction("findForm", "JArray", JArray_FindForm, false);
    vm->RegisterFunction("countInteger", "JArray", JArray_CountInteger, false);
    vm->RegisterFunction("countFloat", "JArray", JArray_CountFloat, false);
    vm->RegisterFunction("countString", "JArray", JArray_CountString, false);
    vm->RegisterFunction("countObject", "JArray", JArray_CountObject, false);
    vm->RegisterFunction("countForm", "JArray", JArray_CountForm, false);
    vm->RegisterFunction("setInt", "JArray", JArray_SetInt, false);
    vm->RegisterFunction("setFlt", "JArray", JArray_SetFlt, false);
    vm->RegisterFunction("setStr", "JArray", JArray_SetStr, false);
    vm->RegisterFunction("setObj", "JArray", JArray_SetObj, false);
    vm->RegisterFunction("setForm", "JArray", JArray_SetForm, false);
    vm->RegisterFunction("addInt", "JArray", JArray_AddInt, false);
    vm->RegisterFunction("addFlt", "JArray", JArray_AddFlt, false);
    vm->RegisterFunction("addStr", "JArray", JArray_AddStr, false);
    vm->RegisterFunction("addObj", "JArray", JArray_AddObj, false);
    vm->RegisterFunction("addForm", "JArray", JArray_AddForm, false);
    vm->RegisterFunction("count", "JArray", JArray_Count, false);
    vm->RegisterFunction("clear", "JArray", JArray_Clear, false);
    vm->RegisterFunction("eraseIndex", "JArray", JArray_EraseIndex, false);
    vm->RegisterFunction("eraseRange", "JArray", JArray_EraseRange, false);
    vm->RegisterFunction("eraseInteger", "JArray", JArray_EraseInteger, false);
    vm->RegisterFunction("eraseFloat", "JArray", JArray_EraseFloat, false);
    vm->RegisterFunction("eraseString", "JArray", JArray_EraseString, false);
    vm->RegisterFunction("eraseObject", "JArray", JArray_EraseObject, false);
    vm->RegisterFunction("eraseForm", "JArray", JArray_EraseForm, false);
    vm->RegisterFunction("valueType", "JArray", JArray_ValueType, false);
    vm->RegisterFunction("swapItems", "JArray", JArray_SwapItems, false);
    vm->RegisterFunction("sort", "JArray", JArray_Sort, false);
    vm->RegisterFunction("unique", "JArray", JArray_Unique, false);
    vm->RegisterFunction("reverse", "JArray", JArray_Reverse, false);
    vm->RegisterFunction("writeToIntegerPArray", "JArray", JArray_WriteToIntegerPArray, false);
    vm->RegisterFunction("writeToFloatPArray", "JArray", JArray_WriteToFloatPArray, false);
    vm->RegisterFunction("writeToFormPArray", "JArray", JArray_WriteToFormPArray, false);
    vm->RegisterFunction("writeToStringPArray", "JArray", JArray_WriteToStringPArray, false);

    // JValue
    vm->RegisterFunction("enableAPILog", "JValue", JValue_EnableAPILog, false);
    vm->RegisterFunction("retain", "JValue", JValue_Retain, false);
    vm->RegisterFunction("release", "JValue", JValue_Release, false);
    vm->RegisterFunction("releaseAndRetain", "JValue", JValue_ReleaseAndRetain, false);
    vm->RegisterFunction("releaseObjectsWithTag", "JValue", JValue_ReleaseObjectsWithTag, false);
    vm->RegisterFunction("zeroLifetime", "JValue", JValue_ZeroLifetime, false);
    vm->RegisterFunction("addToPool", "JValue", JValue_AddToPool, false);
    vm->RegisterFunction("cleanPool", "JValue", JValue_CleanPool, false);
    vm->RegisterFunction("shallowCopy", "JValue", JValue_ShallowCopy, false);
    vm->RegisterFunction("deepCopy", "JValue", JValue_DeepCopy, false);
    vm->RegisterFunction("isExists", "JValue", JValue_IsExists, false);
    vm->RegisterFunction("isArray", "JValue", JValue_IsArray, false);
    vm->RegisterFunction("isMap", "JValue", JValue_IsMap, false);
    vm->RegisterFunction("isFormMap", "JValue", JValue_IsFormMap, false);
    vm->RegisterFunction("isIntegerMap", "JValue", JValue_IsIntegerMap, false);
    vm->RegisterFunction("empty", "JValue", JValue_Empty, false);
    vm->RegisterFunction("count", "JValue", JValue_Count, false);
    vm->RegisterFunction("clear", "JValue", JValue_Clear, false);
    vm->RegisterFunction("readFromFile", "JValue", JValue_ReadFromFile, false);
    vm->RegisterFunction("readFromDirectory", "JValue", JValue_ReadFromDirectory, false);
    vm->RegisterFunction("objectFromPrototype", "JValue", JValue_ObjectFromPrototype, false);
    vm->RegisterFunction("writeToFile", "JValue", JValue_WriteToFile, false);
    vm->RegisterFunction("toString", "JValue", JValue_ToString, false);
    vm->RegisterFunction("toJsonString", "JValue", JValue_ToString, false);
    vm->RegisterFunction("solvedValueType", "JValue", JValue_SolvedValueType, false);
    vm->RegisterFunction("hasPath", "JValue", JValue_HasPath, false);
    vm->RegisterFunction("solveFlt", "JValue", JValue_SolveFlt, false);
    vm->RegisterFunction("solveInt", "JValue", JValue_SolveInt, false);
    vm->RegisterFunction("solveStr", "JValue", JValue_SolveStr, false);
    vm->RegisterFunction("solveObj", "JValue", JValue_SolveObj, false);
    vm->RegisterFunction("solveForm", "JValue", JValue_SolveForm, false);
    vm->RegisterFunction("solveFltSetter", "JValue", JValue_SolveFltSetter, false);
    vm->RegisterFunction("solveIntSetter", "JValue", JValue_SolveIntSetter, false);
    vm->RegisterFunction("solveStrSetter", "JValue", JValue_SolveStrSetter, false);
    vm->RegisterFunction("solveObjSetter", "JValue", JValue_SolveObjSetter, false);
    vm->RegisterFunction("solveFormSetter", "JValue", JValue_SolveFormSetter, false);
    vm->RegisterFunction("evalLuaFlt", "JValue", JValue_EvalLuaFlt, false);
    vm->RegisterFunction("evalLuaInt", "JValue", JValue_EvalLuaInt, false);
    vm->RegisterFunction("evalLuaStr", "JValue", JValue_EvalLuaStr, false);
    vm->RegisterFunction("evalLuaObj", "JValue", JValue_EvalLuaObj, false);
    vm->RegisterFunction("evalLuaForm", "JValue", JValue_EvalLuaForm, false);

    // JDB
    vm->RegisterFunction("solveFlt", "JDB", JDB_SolveFlt, false);
    vm->RegisterFunction("solveInt", "JDB", JDB_SolveInt, false);
    vm->RegisterFunction("solveStr", "JDB", JDB_SolveStr, false);
    vm->RegisterFunction("solveObj", "JDB", JDB_SolveObj, false);
    vm->RegisterFunction("solveForm", "JDB", JDB_SolveForm, false);
    vm->RegisterFunction("solveFltSetter", "JDB", JDB_SolveFltSetter, false);
    vm->RegisterFunction("solveIntSetter", "JDB", JDB_SolveIntSetter, false);
    vm->RegisterFunction("solveStrSetter", "JDB", JDB_SolveStrSetter, false);
    vm->RegisterFunction("solveObjSetter", "JDB", JDB_SolveObjSetter, false);
    vm->RegisterFunction("solveFormSetter", "JDB", JDB_SolveFormSetter, false);
    vm->RegisterFunction("setObj", "JDB", JDB_SetObj, false);
    vm->RegisterFunction("hasPath", "JDB", JDB_HasPath, false);
    vm->RegisterFunction("allKeys", "JDB", JDB_AllKeys, false);
    vm->RegisterFunction("allValues", "JDB", JDB_AllValues, false);
    vm->RegisterFunction("writeToFile", "JDB", JDB_WriteToFile, false);
    vm->RegisterFunction("root", "JDB", JDB_Root, false);

    // JFormDB
    vm->RegisterFunction("setEntry", "JFormDB", JFormDB_SetEntry, false);
    vm->RegisterFunction("makeEntry", "JFormDB", JFormDB_MakeEntry, false);
    vm->RegisterFunction("findEntry", "JFormDB", JFormDB_FindEntry, false);
    vm->RegisterFunction("solveFlt", "JFormDB", JFormDB_SolveFlt, false);
    vm->RegisterFunction("solveInt", "JFormDB", JFormDB_SolveInt, false);
    vm->RegisterFunction("solveStr", "JFormDB", JFormDB_SolveStr, false);
    vm->RegisterFunction("solveObj", "JFormDB", JFormDB_SolveObj, false);
    vm->RegisterFunction("solveForm", "JFormDB", JFormDB_SolveForm, false);
    vm->RegisterFunction("solveFltSetter", "JFormDB", JFormDB_SolveFltSetter, false);
    vm->RegisterFunction("solveIntSetter", "JFormDB", JFormDB_SolveIntSetter, false);
    vm->RegisterFunction("solveStrSetter", "JFormDB", JFormDB_SolveStrSetter, false);
    vm->RegisterFunction("solveObjSetter", "JFormDB", JFormDB_SolveObjSetter, false);
    vm->RegisterFunction("solveFormSetter", "JFormDB", JFormDB_SolveFormSetter, false);
    vm->RegisterFunction("hasPath", "JFormDB", JFormDB_HasPath, false);
    vm->RegisterFunction("allKeys", "JFormDB", JFormDB_AllKeys, false);
    vm->RegisterFunction("allValues", "JFormDB", JFormDB_AllValues, false);
    vm->RegisterFunction("getInt", "JFormDB", JFormDB_GetInt, false);
    vm->RegisterFunction("getFlt", "JFormDB", JFormDB_GetFlt, false);
    vm->RegisterFunction("getStr", "JFormDB", JFormDB_GetStr, false);
    vm->RegisterFunction("getObj", "JFormDB", JFormDB_GetObj, false);
    vm->RegisterFunction("getForm", "JFormDB", JFormDB_GetForm, false);
    vm->RegisterFunction("setInt", "JFormDB", JFormDB_SetInt, false);
    vm->RegisterFunction("setFlt", "JFormDB", JFormDB_SetFlt, false);
    vm->RegisterFunction("setStr", "JFormDB", JFormDB_SetStr, false);
    vm->RegisterFunction("setObj", "JFormDB", JFormDB_SetObj, false);
    vm->RegisterFunction("setForm", "JFormDB", JFormDB_SetForm, false);

    // JFormMap
    vm->RegisterFunction("object", "JFormMap", JFormMap_Object, false);
    vm->RegisterFunction("getInt", "JFormMap", JFormMap_GetInt, false);
    vm->RegisterFunction("getFlt", "JFormMap", JFormMap_GetFlt, false);
    vm->RegisterFunction("getStr", "JFormMap", JFormMap_GetStr, false);
    vm->RegisterFunction("getObj", "JFormMap", JFormMap_GetObj, false);
    vm->RegisterFunction("getForm", "JFormMap", JFormMap_GetForm, false);
    vm->RegisterFunction("setInt", "JFormMap", JFormMap_SetInt, false);
    vm->RegisterFunction("setFlt", "JFormMap", JFormMap_SetFlt, false);
    vm->RegisterFunction("setStr", "JFormMap", JFormMap_SetStr, false);
    vm->RegisterFunction("setObj", "JFormMap", JFormMap_SetObj, false);
    vm->RegisterFunction("setForm", "JFormMap", JFormMap_SetForm, false);
    vm->RegisterFunction("hasKey", "JFormMap", JFormMap_HasKey, false);
    vm->RegisterFunction("valueType", "JFormMap", JFormMap_ValueType, false);
    vm->RegisterFunction("allKeys", "JFormMap", JFormMap_AllKeys, false);
    vm->RegisterFunction("allKeysPArray", "JFormMap", JFormMap_AllKeysPArray, false);
    vm->RegisterFunction("allValues", "JFormMap", JFormMap_AllValues, false);
    vm->RegisterFunction("removeKey", "JFormMap", JFormMap_RemoveKey, false);
    vm->RegisterFunction("count", "JFormMap", JFormMap_Count, false);
    vm->RegisterFunction("clear", "JFormMap", JFormMap_Clear, false);
    vm->RegisterFunction("addPairs", "JFormMap", JFormMap_AddPairs, false);
    vm->RegisterFunction("nextKey", "JFormMap", JFormMap_NextKey, false);
    vm->RegisterFunction("getNthKey", "JFormMap", JFormMap_GetNthKey, false);
    vm->RegisterFunction("insertInt", "JFormMap", JFormMap_InsertInt, false);
    vm->RegisterFunction("insertFlt", "JFormMap", JFormMap_InsertFlt, false);
    vm->RegisterFunction("insertStr", "JFormMap", JFormMap_InsertStr, false);
    vm->RegisterFunction("insertObj", "JFormMap", JFormMap_InsertObj, false);
    vm->RegisterFunction("insertForm", "JFormMap", JFormMap_InsertForm, false);

    // JIntMap
    vm->RegisterFunction("object", "JIntMap", JIntMap_Object, false);
    vm->RegisterFunction("getInt", "JIntMap", JIntMap_GetInt, false);
    vm->RegisterFunction("getFlt", "JIntMap", JIntMap_GetFlt, false);
    vm->RegisterFunction("getStr", "JIntMap", JIntMap_GetStr, false);
    vm->RegisterFunction("getObj", "JIntMap", JIntMap_GetObj, false);
    vm->RegisterFunction("getForm", "JIntMap", JIntMap_GetForm, false);
    vm->RegisterFunction("setInt", "JIntMap", JIntMap_SetInt, false);
    vm->RegisterFunction("setFlt", "JIntMap", JIntMap_SetFlt, false);
    vm->RegisterFunction("setStr", "JIntMap", JIntMap_SetStr, false);
    vm->RegisterFunction("setObj", "JIntMap", JIntMap_SetObj, false);
    vm->RegisterFunction("setForm", "JIntMap", JIntMap_SetForm, false);
    vm->RegisterFunction("hasKey", "JIntMap", JIntMap_HasKey, false);
    vm->RegisterFunction("valueType", "JIntMap", JIntMap_ValueType, false);
    vm->RegisterFunction("allKeys", "JIntMap", JIntMap_AllKeys, false);
    vm->RegisterFunction("allKeysPArray", "JIntMap", JIntMap_AllKeysPArray, false);
    vm->RegisterFunction("allValues", "JIntMap", JIntMap_AllValues, false);
    vm->RegisterFunction("removeKey", "JIntMap", JIntMap_RemoveKey, false);
    vm->RegisterFunction("count", "JIntMap", JIntMap_Count, false);
    vm->RegisterFunction("clear", "JIntMap", JIntMap_Clear, false);
    vm->RegisterFunction("addPairs", "JIntMap", JIntMap_AddPairs, false);
    vm->RegisterFunction("nextKey", "JIntMap", JIntMap_NextKey, false);
    vm->RegisterFunction("getNthKey", "JIntMap", JIntMap_GetNthKey, false);
    vm->RegisterFunction("insertInt", "JIntMap", JIntMap_InsertInt, false);
    vm->RegisterFunction("insertFlt", "JIntMap", JIntMap_InsertFlt, false);
    vm->RegisterFunction("insertStr", "JIntMap", JIntMap_InsertStr, false);
    vm->RegisterFunction("insertObj", "JIntMap", JIntMap_InsertObj, false);
    vm->RegisterFunction("insertForm", "JIntMap", JIntMap_InsertForm, false);

    // JString
    vm->RegisterFunction("wrap", "JString", JString_Wrap, false);
    vm->RegisterFunction("decodeFormStringToFormId", "JString", JString_DecodeFormStringToFormId, false);
    vm->RegisterFunction("decodeFormStringToForm", "JString", JString_DecodeFormStringToForm, false);
    vm->RegisterFunction("encodeFormToString", "JString", JString_EncodeFormToString, false);
    vm->RegisterFunction("encodeFormIdToString", "JString", JString_EncodeFormIdToString, false);
    vm->RegisterFunction("generateUUID", "JString", JString_GenerateUUID, false);

    // JContainers
    vm->RegisterFunction("__isInstalled", "JContainers", JContainers_IsInstalled, false);
    vm->RegisterFunction("APIVersion", "JContainers", JContainers_APIVersion, false);
    vm->RegisterFunction("featureVersion", "JContainers", JContainers_FeatureVersion, false);
    vm->RegisterFunction("fileExistsAtPath", "JContainers", JContainers_FileExistsAtPath, false);
    vm->RegisterFunction("contentsOfDirectoryAtPath", "JContainers", JContainers_ContentsOfDirectoryAtPath, false);
    vm->RegisterFunction("removeFileAtPath", "JContainers", JContainers_RemoveFileAtPath, false);
    vm->RegisterFunction("userDirectory", "JContainers", JContainers_UserDirectory, false);

    // JLua
    vm->RegisterFunction("evalLuaFlt", "JLua", JLua_EvalLuaFlt, false);
    vm->RegisterFunction("evalLuaInt", "JLua", JLua_EvalLuaInt, false);
    vm->RegisterFunction("evalLuaStr", "JLua", JLua_EvalLuaStr, false);
    vm->RegisterFunction("evalLuaObj", "JLua", JLua_EvalLuaObj, false);
    vm->RegisterFunction("evalLuaForm", "JLua", JLua_EvalLuaForm, false);
    vm->RegisterFunction("setStr", "JLua", JLua_SetStr, false);
    vm->RegisterFunction("setFlt", "JLua", JLua_SetFlt, false);
    vm->RegisterFunction("setInt", "JLua", JLua_SetInt, false);
    vm->RegisterFunction("setForm", "JLua", JLua_SetForm, false);
    vm->RegisterFunction("setObj", "JLua", JLua_SetObj, false);

    SKSE::log::info("JContainersNG: all natives registered");
    return true;
}

// --- API export for DLL-to-DLL communication ---

struct JCNG_API_V1 {
    int version;
    Handle(*jmap_object)(RE::StaticFunctionTag*);
    void (*jmap_set_int)(RE::StaticFunctionTag*, Handle, std::string, int32_t);
    int32_t(*jmap_get_int)(RE::StaticFunctionTag*, Handle, std::string, int32_t);
    void (*jmap_set_str)(RE::StaticFunctionTag*, Handle, std::string, std::string);
    std::string(*jmap_get_str)(RE::StaticFunctionTag*, Handle, std::string, std::string);
    void (*jmap_set_obj)(RE::StaticFunctionTag*, Handle, std::string, Handle);
    Handle(*jmap_get_obj)(RE::StaticFunctionTag*, Handle, std::string, Handle);
    void (*jmap_set_form)(RE::StaticFunctionTag*, Handle, std::string, RE::TESForm*);
    RE::TESForm* (*jmap_get_form)(RE::StaticFunctionTag*, Handle, std::string, RE::TESForm*);
    bool (*jmap_remove_key)(RE::StaticFunctionTag*, Handle, std::string);
    void (*jvalue_write_to_file)(RE::StaticFunctionTag*, Handle, std::string);
    Handle(*jvalue_read_from_file)(RE::StaticFunctionTag*, std::string);
    Handle(*jdb_root)(RE::StaticFunctionTag*);
};

static JCNG_API_V1 g_JContainersNGAPI_V1_Instance = {
    .version = 1,
    .jmap_object = JMap_Object,
    .jmap_set_int = JMap_SetInt,
    .jmap_get_int = JMap_GetInt,
    .jmap_set_str = JMap_SetStr,
    .jmap_get_str = JMap_GetStr,
    .jmap_set_obj = JMap_SetObj,
    .jmap_get_obj = JMap_GetObj,
    .jmap_set_form = JMap_SetForm,
    .jmap_get_form = JMap_GetForm,
    .jmap_remove_key = JMap_RemoveKey,
    .jvalue_write_to_file = JValue_WriteToFile,
    .jvalue_read_from_file = JValue_ReadFromFile,
    .jdb_root = JDB_Root,
};

extern "C" __declspec(dllexport) void* GetJContainersNGAPI() {
    return &g_JContainersNGAPI_V1_Instance;
}

// --- Event sinks ---

class FormDeleteObserver : public RE::BSTEventSink<RE::TESFormDeleteEvent> {
public:
    static FormDeleteObserver* GetSingleton() {
        static FormDeleteObserver instance;
        return &instance;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESFormDeleteEvent* a_event, RE::BSTEventSource<RE::TESFormDeleteEvent>*) override {
        if (a_event) {
            ObjectManager::Get().MarkFormsDirty();
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};

// --- Messaging ---

void OnSKSEMessage(SKSE::MessagingInterface::Message* msg) {
    if (msg->type == SKSE::MessagingInterface::kPreLoadGame) {
        SKSE::log::info("JContainersNG: kPreLoadGame fired");
        ObjectManager::Get().Clear();
        Handle root = ObjectManager::Get().CreateObject();
        ObjectManager::Get().SetJDBRoot(root);
    }
    else if (msg->type == SKSE::MessagingInterface::kNewGame) {
        SKSE::log::info("JContainersNG: kNewGame fired");
        ObjectManager::Get().Clear();
        Handle root = ObjectManager::Get().CreateObject();
        ObjectManager::Get().SetJDBRoot(root);
    }
    else if (msg->type == SKSE::MessagingInterface::kPostPostLoad) {
        auto messaging = SKSE::GetMessagingInterface();
        if (messaging) {
            // was dispatching on type 1 — that IS kPostPostLoad, so every mod got
            // our api struct as a fake kPostPostLoad event. 'JCNG' is ours alone
            messaging->Dispatch('JCNG', &g_JContainersNGAPI_V1_Instance, sizeof(void*), nullptr);
            SKSE::log::info("JContainersNG: root_interface broadcast on 'JCNG'");
        }
    }
    else if (msg->type == 'JCNG') {
        auto** reply = static_cast<JCNG_API_V1**>(msg->data);
        if (reply) {
            *reply = &g_JContainersNGAPI_V1_Instance;
            SKSE::log::info("JContainersNG: API v1 exported to {}", msg->sender);
        }
    }
}

// --- Plugin entry ---

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);

    auto serial = SKSE::GetSerializationInterface();
    serial->SetUniqueID('JCON');
    serial->SetSaveCallback(SaveCallback);
    serial->SetLoadCallback(LoadCallback);
    serial->SetRevertCallback(RevertCallback);

    auto messaging = SKSE::GetMessagingInterface();
    if (messaging) {
        messaging->RegisterListener(OnSKSEMessage);
        auto* sourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
        if (sourceHolder) {
            sourceHolder->AddEventSink(FormDeleteObserver::GetSingleton());
            SKSE::log::info("JContainersNG: form delete observer registered");
        }
    }

    auto papyrus = SKSE::GetPapyrusInterface();
    if (!papyrus || !papyrus->Register(RegisterJContainersFunctions)) {
        SKSE::log::error("JContainersNG: Papyrus registration failed");
        return false;
    }

    SKSE::log::info("JContainersNG: ready with serialization");
    return true;
}
