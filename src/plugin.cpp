#include <SKSE/SKSE.h>
#include "ObjectManager.hpp"
using Handle = ObjectManager::Handle;

// Forward declare messaging callback
void OnSKSEMessage(SKSE::MessagingInterface::Message* msg);

// Forward declare all the native functions from JContainersNG_Natives.cpp
// JMap
extern Handle JMap_Object(RE::StaticFunctionTag*);
extern void JMap_SetInt(RE::StaticFunctionTag*, Handle, std::string, int32_t);
extern int32_t JMap_GetInt(RE::StaticFunctionTag*, Handle, std::string, int32_t);
extern void JMap_SetFlt(RE::StaticFunctionTag*, Handle, std::string, float);
extern float JMap_GetFlt(RE::StaticFunctionTag*, Handle, std::string, float);
extern void JMap_SetStr(RE::StaticFunctionTag*, Handle, std::string, std::string);
extern std::string JMap_GetStr(RE::StaticFunctionTag*, Handle, std::string, std::string);
extern void JMap_SetObj(RE::StaticFunctionTag*, Handle, std::string, Handle);
extern Handle JMap_GetObj(RE::StaticFunctionTag*, Handle, std::string, Handle);
extern void JMap_SetForm(RE::StaticFunctionTag*, Handle, std::string, RE::TESForm*);
extern RE::TESForm* JMap_GetForm(RE::StaticFunctionTag*, Handle, std::string, RE::TESForm*);
extern bool JMap_HasKey(RE::StaticFunctionTag*, Handle, std::string);
extern int32_t JMap_ValueType(RE::StaticFunctionTag*, Handle, std::string);
extern Handle JMap_AllKeys(RE::StaticFunctionTag*, Handle);
extern std::vector<std::string> JMap_AllKeysPArray(RE::StaticFunctionTag*, Handle);
extern Handle JMap_AllValues(RE::StaticFunctionTag*, Handle);
extern bool JMap_RemoveKey(RE::StaticFunctionTag*, Handle, std::string);
extern int32_t JMap_Count(RE::StaticFunctionTag*, Handle);
extern void JMap_Clear(RE::StaticFunctionTag*, Handle);
extern void JMap_AddPairs(RE::StaticFunctionTag*, Handle, Handle, bool);
extern std::string JMap_NextKey(RE::StaticFunctionTag*, Handle, std::string, std::string);
extern std::string JMap_GetNthKey(RE::StaticFunctionTag*, Handle, int32_t);

// JArray
extern Handle JArray_Object(RE::StaticFunctionTag*);
extern Handle JArray_ObjectWithSize(RE::StaticFunctionTag*, int32_t);
extern Handle JArray_ObjectWithInts(RE::StaticFunctionTag*, std::vector<int32_t>);
extern Handle JArray_ObjectWithStrings(RE::StaticFunctionTag*, std::vector<std::string>);
extern Handle JArray_ObjectWithFloats(RE::StaticFunctionTag*, std::vector<float>);
extern Handle JArray_ObjectWithBooleans(RE::StaticFunctionTag*, std::vector<bool>);
extern Handle JArray_ObjectWithForms(RE::StaticFunctionTag*, std::vector<RE::TESForm*>);
extern Handle JArray_SubArray(RE::StaticFunctionTag*, Handle, int32_t, int32_t);
extern void JArray_AddFromArray(RE::StaticFunctionTag*, Handle, Handle, int32_t);
extern void JArray_AddFromFormList(RE::StaticFunctionTag*, Handle, RE::BGSListForm*, int32_t);
extern int32_t JArray_GetInt(RE::StaticFunctionTag*, Handle, int32_t, int32_t);
extern float JArray_GetFlt(RE::StaticFunctionTag*, Handle, int32_t, float);
extern std::string JArray_GetStr(RE::StaticFunctionTag*, Handle, int32_t, std::string);
extern Handle JArray_GetObj(RE::StaticFunctionTag*, Handle, int32_t, Handle);
extern RE::TESForm* JArray_GetForm(RE::StaticFunctionTag*, Handle, int32_t, RE::TESForm*);
extern std::vector<int32_t> JArray_AsIntArray(RE::StaticFunctionTag*, Handle);
extern std::vector<float> JArray_AsFloatArray(RE::StaticFunctionTag*, Handle);
extern std::vector<std::string> JArray_AsStringArray(RE::StaticFunctionTag*, Handle);
extern std::vector<RE::TESForm*> JArray_AsFormArray(RE::StaticFunctionTag*, Handle);
extern int32_t JArray_FindInt(RE::StaticFunctionTag*, Handle, int32_t, int32_t);
extern int32_t JArray_FindFlt(RE::StaticFunctionTag*, Handle, float, int32_t);
extern int32_t JArray_FindStr(RE::StaticFunctionTag*, Handle, std::string, int32_t);
extern int32_t JArray_FindObj(RE::StaticFunctionTag*, Handle, Handle, int32_t);
extern int32_t JArray_FindForm(RE::StaticFunctionTag*, Handle, RE::TESForm*, int32_t);
extern int32_t JArray_CountInteger(RE::StaticFunctionTag*, Handle, int32_t);
extern int32_t JArray_CountFloat(RE::StaticFunctionTag*, Handle, float);
extern int32_t JArray_CountString(RE::StaticFunctionTag*, Handle, std::string);
extern int32_t JArray_CountObject(RE::StaticFunctionTag*, Handle, Handle);
extern int32_t JArray_CountForm(RE::StaticFunctionTag*, Handle, RE::TESForm*);
extern void JArray_SetInt(RE::StaticFunctionTag*, Handle, int32_t, int32_t);
extern void JArray_SetFlt(RE::StaticFunctionTag*, Handle, int32_t, float);
extern void JArray_SetStr(RE::StaticFunctionTag*, Handle, int32_t, std::string);
extern void JArray_SetObj(RE::StaticFunctionTag*, Handle, int32_t, Handle);
extern void JArray_SetForm(RE::StaticFunctionTag*, Handle, int32_t, RE::TESForm*);
extern void JArray_AddInt(RE::StaticFunctionTag*, Handle, int32_t, int32_t);
extern void JArray_AddFlt(RE::StaticFunctionTag*, Handle, float, int32_t);
extern void JArray_AddStr(RE::StaticFunctionTag*, Handle, std::string, int32_t);
extern void JArray_AddObj(RE::StaticFunctionTag*, Handle, Handle, int32_t);
extern void JArray_AddForm(RE::StaticFunctionTag*, Handle, RE::TESForm*, int32_t);
extern int32_t JArray_Count(RE::StaticFunctionTag*, Handle);
extern void JArray_Clear(RE::StaticFunctionTag*, Handle);
extern void JArray_EraseIndex(RE::StaticFunctionTag*, Handle, int32_t);
extern void JArray_EraseRange(RE::StaticFunctionTag*, Handle, int32_t, int32_t);
extern int32_t JArray_EraseInteger(RE::StaticFunctionTag*, Handle, int32_t);
extern int32_t JArray_EraseFloat(RE::StaticFunctionTag*, Handle, float);
extern int32_t JArray_EraseString(RE::StaticFunctionTag*, Handle, std::string);
extern int32_t JArray_EraseObject(RE::StaticFunctionTag*, Handle, Handle);
extern int32_t JArray_EraseForm(RE::StaticFunctionTag*, Handle, RE::TESForm*);
extern int32_t JArray_ValueType(RE::StaticFunctionTag*, Handle, int32_t);
extern void JArray_SwapItems(RE::StaticFunctionTag*, Handle, int32_t, int32_t);
extern Handle JArray_Sort(RE::StaticFunctionTag*, Handle);
extern Handle JArray_Unique(RE::StaticFunctionTag*, Handle);
extern Handle JArray_Reverse(RE::StaticFunctionTag*, Handle);
extern bool JArray_WriteToIntegerPArray(RE::BSScript::IVirtualMachine*, RE::VMStackID, RE::StaticFunctionTag*, Handle, std::vector<int32_t>, int32_t, int32_t, int32_t, int32_t);
extern bool JArray_WriteToFloatPArray(RE::BSScript::IVirtualMachine*, RE::VMStackID, RE::StaticFunctionTag*, Handle, std::vector<float>, int32_t, int32_t, int32_t, float);
extern bool JArray_WriteToFormPArray(RE::BSScript::IVirtualMachine*, RE::VMStackID, RE::StaticFunctionTag*, Handle, std::vector<RE::TESForm*>, int32_t, int32_t, int32_t, RE::TESForm*);
extern bool JArray_WriteToStringPArray(RE::BSScript::IVirtualMachine*, RE::VMStackID, RE::StaticFunctionTag*, Handle, std::vector<std::string>, int32_t, int32_t, int32_t, std::string);

// JValue
extern void JValue_EnableAPILog(RE::StaticFunctionTag*, bool);
extern Handle JValue_Retain(RE::StaticFunctionTag*, Handle, std::string);
extern Handle JValue_Release(RE::StaticFunctionTag*, Handle);
extern Handle JValue_ReleaseAndRetain(RE::StaticFunctionTag*, Handle, Handle, std::string);
extern void JValue_ReleaseObjectsWithTag(RE::StaticFunctionTag*, std::string);
extern Handle JValue_ZeroLifetime(RE::StaticFunctionTag*, Handle);
extern Handle JValue_AddToPool(RE::StaticFunctionTag*, Handle, std::string);
extern void JValue_CleanPool(RE::StaticFunctionTag*, std::string);
extern Handle JValue_ShallowCopy(RE::StaticFunctionTag*, Handle);
extern Handle JValue_DeepCopy(RE::StaticFunctionTag*, Handle);
extern bool JValue_IsExists(RE::StaticFunctionTag*, Handle);
extern bool JValue_IsArray(RE::StaticFunctionTag*, Handle);
extern bool JValue_IsMap(RE::StaticFunctionTag*, Handle);
extern bool JValue_IsFormMap(RE::StaticFunctionTag*, Handle);
extern bool JValue_IsIntegerMap(RE::StaticFunctionTag*, Handle);
extern bool JValue_Empty(RE::StaticFunctionTag*, Handle);
extern int32_t JValue_Count(RE::StaticFunctionTag*, Handle);
extern void JValue_Clear(RE::StaticFunctionTag*, Handle);
extern Handle JValue_ReadFromFile(RE::StaticFunctionTag*, std::string);
extern Handle JValue_ReadFromDirectory(RE::StaticFunctionTag*, std::string, std::string);
extern Handle JValue_ObjectFromPrototype(RE::StaticFunctionTag*, std::string);
extern void JValue_WriteToFile(RE::StaticFunctionTag*, Handle, std::string);
extern std::string JValue_ToString(RE::StaticFunctionTag*, Handle);
extern int32_t JValue_SolvedValueType(RE::StaticFunctionTag*, Handle, std::string);
extern bool JValue_HasPath(RE::StaticFunctionTag*, Handle, std::string);
extern float JValue_SolveFlt(RE::StaticFunctionTag*, Handle, std::string, float);
extern int32_t JValue_SolveInt(RE::StaticFunctionTag*, Handle, std::string, int32_t);
extern std::string JValue_SolveStr(RE::StaticFunctionTag*, Handle, std::string, std::string);
extern Handle JValue_SolveObj(RE::StaticFunctionTag*, Handle, std::string, Handle);
extern RE::TESForm* JValue_SolveForm(RE::StaticFunctionTag*, Handle, std::string, RE::TESForm*);
extern bool JValue_SolveFltSetter(RE::StaticFunctionTag*, Handle, std::string, float, bool);
extern bool JValue_SolveIntSetter(RE::StaticFunctionTag*, Handle, std::string, int32_t, bool);
extern bool JValue_SolveStrSetter(RE::StaticFunctionTag*, Handle, std::string, std::string, bool);
extern bool JValue_SolveObjSetter(RE::StaticFunctionTag*, Handle, std::string, Handle, bool);
extern bool JValue_SolveFormSetter(RE::StaticFunctionTag*, Handle, std::string, RE::TESForm*, bool);

// JDB
extern float JDB_SolveFlt(RE::StaticFunctionTag*, std::string, float);
extern int32_t JDB_SolveInt(RE::StaticFunctionTag*, std::string, int32_t);
extern std::string JDB_SolveStr(RE::StaticFunctionTag*, std::string, std::string);
extern Handle JDB_SolveObj(RE::StaticFunctionTag*, std::string, Handle);
extern RE::TESForm* JDB_SolveForm(RE::StaticFunctionTag*, std::string, RE::TESForm*);
extern bool JDB_SolveFltSetter(RE::StaticFunctionTag*, std::string, float, bool);
extern bool JDB_SolveIntSetter(RE::StaticFunctionTag*, std::string, int32_t, bool);
extern bool JDB_SolveStrSetter(RE::StaticFunctionTag*, std::string, std::string, bool);
extern bool JDB_SolveObjSetter(RE::StaticFunctionTag*, std::string, Handle, bool);
extern bool JDB_SolveFormSetter(RE::StaticFunctionTag*, std::string, RE::TESForm*, bool);
extern void JDB_SetObj(RE::StaticFunctionTag*, std::string, Handle);
extern bool JDB_HasPath(RE::StaticFunctionTag*, std::string);
extern Handle JDB_AllKeys(RE::StaticFunctionTag*);
extern Handle JDB_AllValues(RE::StaticFunctionTag*);
extern void JDB_WriteToFile(RE::StaticFunctionTag*, std::string);
extern Handle JDB_Root(RE::StaticFunctionTag*);

// JFormDB
extern void JFormDB_SetEntry(RE::StaticFunctionTag*, std::string, RE::TESForm*, Handle);
extern Handle JFormDB_MakeEntry(RE::StaticFunctionTag*, std::string, RE::TESForm*);
extern Handle JFormDB_FindEntry(RE::StaticFunctionTag*, std::string, RE::TESForm*);
extern float JFormDB_SolveFlt(RE::StaticFunctionTag*, RE::TESForm*, std::string, float);
extern int32_t JFormDB_SolveInt(RE::StaticFunctionTag*, RE::TESForm*, std::string, int32_t);
extern std::string JFormDB_SolveStr(RE::StaticFunctionTag*, RE::TESForm*, std::string, std::string);
extern Handle JFormDB_SolveObj(RE::StaticFunctionTag*, RE::TESForm*, std::string, Handle);
extern RE::TESForm* JFormDB_SolveForm(RE::StaticFunctionTag*, RE::TESForm*, std::string, RE::TESForm*);
extern bool JFormDB_SolveFltSetter(RE::StaticFunctionTag*, RE::TESForm*, std::string, float, bool);
extern bool JFormDB_SolveIntSetter(RE::StaticFunctionTag*, RE::TESForm*, std::string, int32_t, bool);
extern bool JFormDB_SolveStrSetter(RE::StaticFunctionTag*, RE::TESForm*, std::string, std::string, bool);
extern bool JFormDB_SolveObjSetter(RE::StaticFunctionTag*, RE::TESForm*, std::string, Handle, bool);
extern bool JFormDB_SolveFormSetter(RE::StaticFunctionTag*, RE::TESForm*, std::string, RE::TESForm*, bool);
extern bool JFormDB_HasPath(RE::StaticFunctionTag*, RE::TESForm*, std::string);
extern Handle JFormDB_AllKeys(RE::StaticFunctionTag*, RE::TESForm*, std::string);
extern Handle JFormDB_AllValues(RE::StaticFunctionTag*, RE::TESForm*, std::string);
extern int32_t JFormDB_GetInt(RE::StaticFunctionTag*, RE::TESForm*, std::string);
extern float JFormDB_GetFlt(RE::StaticFunctionTag*, RE::TESForm*, std::string);
extern std::string JFormDB_GetStr(RE::StaticFunctionTag*, RE::TESForm*, std::string);
extern Handle JFormDB_GetObj(RE::StaticFunctionTag*, RE::TESForm*, std::string);
extern RE::TESForm* JFormDB_GetForm(RE::StaticFunctionTag*, RE::TESForm*, std::string);
extern void JFormDB_SetInt(RE::StaticFunctionTag*, RE::TESForm*, std::string, int32_t);
extern void JFormDB_SetFlt(RE::StaticFunctionTag*, RE::TESForm*, std::string, float);
extern void JFormDB_SetStr(RE::StaticFunctionTag*, RE::TESForm*, std::string, std::string);
extern void JFormDB_SetObj(RE::StaticFunctionTag*, RE::TESForm*, std::string, Handle);
extern void JFormDB_SetForm(RE::StaticFunctionTag*, RE::TESForm*, std::string, RE::TESForm*);

// JFormMap
extern Handle JFormMap_Object(RE::StaticFunctionTag*);
extern int32_t JFormMap_GetInt(RE::StaticFunctionTag*, Handle, RE::TESForm*, int32_t);
extern float JFormMap_GetFlt(RE::StaticFunctionTag*, Handle, RE::TESForm*, float);
extern std::string JFormMap_GetStr(RE::StaticFunctionTag*, Handle, RE::TESForm*, std::string);
extern Handle JFormMap_GetObj(RE::StaticFunctionTag*, Handle, RE::TESForm*, Handle);
extern RE::TESForm* JFormMap_GetForm(RE::StaticFunctionTag*, Handle, RE::TESForm*, RE::TESForm*);
extern void JFormMap_SetInt(RE::StaticFunctionTag*, Handle, RE::TESForm*, int32_t);
extern void JFormMap_SetFlt(RE::StaticFunctionTag*, Handle, RE::TESForm*, float);
extern void JFormMap_SetStr(RE::StaticFunctionTag*, Handle, RE::TESForm*, std::string);
extern void JFormMap_SetObj(RE::StaticFunctionTag*, Handle, RE::TESForm*, Handle);
extern void JFormMap_SetForm(RE::StaticFunctionTag*, Handle, RE::TESForm*, RE::TESForm*);
extern bool JFormMap_HasKey(RE::StaticFunctionTag*, Handle, RE::TESForm*);
extern int32_t JFormMap_ValueType(RE::StaticFunctionTag*, Handle, RE::TESForm*);
extern Handle JFormMap_AllKeys(RE::StaticFunctionTag*, Handle);
extern std::vector<RE::TESForm*> JFormMap_AllKeysPArray(RE::StaticFunctionTag*, Handle);
extern Handle JFormMap_AllValues(RE::StaticFunctionTag*, Handle);
extern bool JFormMap_RemoveKey(RE::StaticFunctionTag*, Handle, RE::TESForm*);
extern int32_t JFormMap_Count(RE::StaticFunctionTag*, Handle);
extern void JFormMap_Clear(RE::StaticFunctionTag*, Handle);
extern void JFormMap_AddPairs(RE::StaticFunctionTag*, Handle, Handle, bool);
extern RE::TESForm* JFormMap_NextKey(RE::StaticFunctionTag*, Handle, RE::TESForm*, RE::TESForm*);
extern RE::TESForm* JFormMap_GetNthKey(RE::StaticFunctionTag*, Handle, int32_t);

// JIntMap
extern Handle JIntMap_Object(RE::StaticFunctionTag*);
extern int32_t JIntMap_GetInt(RE::StaticFunctionTag*, Handle, int32_t, int32_t);
extern float JIntMap_GetFlt(RE::StaticFunctionTag*, Handle, int32_t, float);
extern std::string JIntMap_GetStr(RE::StaticFunctionTag*, Handle, int32_t, std::string);
extern Handle JIntMap_GetObj(RE::StaticFunctionTag*, Handle, int32_t, Handle);
extern RE::TESForm* JIntMap_GetForm(RE::StaticFunctionTag*, Handle, int32_t, RE::TESForm*);
extern void JIntMap_SetInt(RE::StaticFunctionTag*, Handle, int32_t, int32_t);
extern void JIntMap_SetFlt(RE::StaticFunctionTag*, Handle, int32_t, float);
extern void JIntMap_SetStr(RE::StaticFunctionTag*, Handle, int32_t, std::string);
extern void JIntMap_SetObj(RE::StaticFunctionTag*, Handle, int32_t, Handle);
extern void JIntMap_SetForm(RE::StaticFunctionTag*, Handle, int32_t, RE::TESForm*);
extern bool JIntMap_HasKey(RE::StaticFunctionTag*, Handle, int32_t);
extern int32_t JIntMap_ValueType(RE::StaticFunctionTag*, Handle, int32_t);
extern Handle JIntMap_AllKeys(RE::StaticFunctionTag*, Handle);
extern std::vector<int32_t> JIntMap_AllKeysPArray(RE::StaticFunctionTag*, Handle);
extern Handle JIntMap_AllValues(RE::StaticFunctionTag*, Handle);
extern bool JIntMap_RemoveKey(RE::StaticFunctionTag*, Handle, int32_t);
extern int32_t JIntMap_Count(RE::StaticFunctionTag*, Handle);
extern void JIntMap_Clear(RE::StaticFunctionTag*, Handle);
extern void JIntMap_AddPairs(RE::StaticFunctionTag*, Handle, Handle, bool);
extern int32_t JIntMap_NextKey(RE::StaticFunctionTag*, Handle, int32_t, int32_t);
extern int32_t JIntMap_GetNthKey(RE::StaticFunctionTag*, Handle, int32_t);

// JString
extern int32_t JString_Wrap(RE::StaticFunctionTag*, std::string, int32_t);
extern int32_t JString_DecodeFormStringToFormId(RE::StaticFunctionTag*, std::string);
extern RE::TESForm* JString_DecodeFormStringToForm(RE::StaticFunctionTag*, std::string);
extern std::string JString_EncodeFormToString(RE::StaticFunctionTag*, RE::TESForm*);
extern std::string JString_EncodeFormIdToString(RE::StaticFunctionTag*, int32_t);
extern std::string JString_GenerateUUID(RE::StaticFunctionTag*);

// JContainers
extern bool JContainers_IsInstalled(RE::StaticFunctionTag*);
extern int32_t JContainers_APIVersion(RE::StaticFunctionTag*);
extern int32_t JContainers_FeatureVersion(RE::StaticFunctionTag*);
extern bool JContainers_FileExistsAtPath(RE::StaticFunctionTag*, std::string);
extern std::vector<std::string> JContainers_ContentsOfDirectoryAtPath(RE::StaticFunctionTag*, std::string, std::string);
extern void JContainers_RemoveFileAtPath(RE::StaticFunctionTag*, std::string);
extern std::string JContainers_UserDirectory(RE::StaticFunctionTag*);

// Serialization callbacks
extern void SaveCallback(SKSE::SerializationInterface*);
extern void LoadCallback(SKSE::SerializationInterface*);
extern void RevertCallback(SKSE::SerializationInterface*);

bool RegisterJContainersFunctions(RE::BSScript::IVirtualMachine* vm) {
    if (!vm) return false;

    // JMap
    vm->RegisterFunction("object", "JMap", JMap_Object, true);
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

    // JArray
    vm->RegisterFunction("object", "JArray", JArray_Object, true);
    vm->RegisterFunction("objectWithSize", "JArray", JArray_ObjectWithSize, true);
    vm->RegisterFunction("objectWithInts", "JArray", JArray_ObjectWithInts, true);
    vm->RegisterFunction("objectWithStrings", "JArray", JArray_ObjectWithStrings, true);
    vm->RegisterFunction("objectWithFloats", "JArray", JArray_ObjectWithFloats, true);
    vm->RegisterFunction("objectWithBooleans", "JArray", JArray_ObjectWithBooleans, true);
    vm->RegisterFunction("objectWithForms", "JArray", JArray_ObjectWithForms, true);
    vm->RegisterFunction("subArray", "JArray", JArray_SubArray, true);
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
    vm->RegisterFunction("shallowCopy", "JValue", JValue_ShallowCopy, true);
    vm->RegisterFunction("deepCopy", "JValue", JValue_DeepCopy, true);
    vm->RegisterFunction("isExists", "JValue", JValue_IsExists, false);
    vm->RegisterFunction("isArray", "JValue", JValue_IsArray, false);
    vm->RegisterFunction("isMap", "JValue", JValue_IsMap, false);
    vm->RegisterFunction("isFormMap", "JValue", JValue_IsFormMap, false);
    vm->RegisterFunction("isIntegerMap", "JValue", JValue_IsIntegerMap, false);
    vm->RegisterFunction("empty", "JValue", JValue_Empty, false);
    vm->RegisterFunction("count", "JValue", JValue_Count, false);
    vm->RegisterFunction("clear", "JValue", JValue_Clear, false);
    vm->RegisterFunction("readFromFile", "JValue", JValue_ReadFromFile, true);
    vm->RegisterFunction("readFromDirectory", "JValue", JValue_ReadFromDirectory, true);
    vm->RegisterFunction("objectFromPrototype", "JValue", JValue_ObjectFromPrototype, true);
    vm->RegisterFunction("writeToFile", "JValue", JValue_WriteToFile, false);
    vm->RegisterFunction("toString", "JValue", JValue_ToString, false);
    vm->RegisterFunction("solvedValueType", "JValue", JValue_SolvedValueType, false);
    vm->RegisterFunction("hasPath", "JValue", JValue_HasPath, false);
    vm->RegisterFunction("solveFlt", "JValue", JValue_SolveFlt, false);
    vm->RegisterFunction("solveInt", "JValue", JValue_SolveInt, false);
    vm->RegisterFunction("solveStr", "JValue", JValue_SolveStr, false);
    vm->RegisterFunction("solveObj", "JValue", JValue_SolveObj, true);
    vm->RegisterFunction("solveForm", "JValue", JValue_SolveForm, false);
    vm->RegisterFunction("solveFltSetter", "JValue", JValue_SolveFltSetter, false);
    vm->RegisterFunction("solveIntSetter", "JValue", JValue_SolveIntSetter, false);
    vm->RegisterFunction("solveStrSetter", "JValue", JValue_SolveStrSetter, false);
    vm->RegisterFunction("solveObjSetter", "JValue", JValue_SolveObjSetter, false);
    vm->RegisterFunction("solveFormSetter", "JValue", JValue_SolveFormSetter, false);

    // JDB
    vm->RegisterFunction("solveFlt", "JDB", JDB_SolveFlt, false);
    vm->RegisterFunction("solveInt", "JDB", JDB_SolveInt, false);
    vm->RegisterFunction("solveStr", "JDB", JDB_SolveStr, false);
    vm->RegisterFunction("solveObj", "JDB", JDB_SolveObj, true);
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
    vm->RegisterFunction("root", "JDB", JDB_Root, true);

    // JFormDB
    vm->RegisterFunction("setEntry", "JFormDB", JFormDB_SetEntry, false);
    vm->RegisterFunction("makeEntry", "JFormDB", JFormDB_MakeEntry, true);
    vm->RegisterFunction("findEntry", "JFormDB", JFormDB_FindEntry, true);
    vm->RegisterFunction("solveFlt", "JFormDB", JFormDB_SolveFlt, false);
    vm->RegisterFunction("solveInt", "JFormDB", JFormDB_SolveInt, false);
    vm->RegisterFunction("solveStr", "JFormDB", JFormDB_SolveStr, false);
    vm->RegisterFunction("solveObj", "JFormDB", JFormDB_SolveObj, true);
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
    vm->RegisterFunction("object", "JFormMap", JFormMap_Object, true);
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

    // JIntMap
    vm->RegisterFunction("object", "JIntMap", JIntMap_Object, true);
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

    // JString
    vm->RegisterFunction("wrap", "JString", JString_Wrap, true);
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

    SKSE::log::info("JContainersNG: all natives registered");
    return true;
}

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
}

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
    }

    auto papyrus = SKSE::GetPapyrusInterface();
    if (!papyrus || !papyrus->Register(RegisterJContainersFunctions)) {
        SKSE::log::error("JContainersNG: Papyrus registration failed");
        return false;
    }

    SKSE::log::info("JContainersNG: ready with serialization");
    return true;
}