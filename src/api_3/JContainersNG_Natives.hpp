//JContainersNG_Natives.hpp
#pragma once
#include "JContainersNG_Internal.hpp"

// ============================================================================
// JValue
// ============================================================================
extern void JValue_EnableAPILog(RE::StaticFunctionTag*, bool arg0);
extern Handle JValue_Retain(RE::StaticFunctionTag*, Handle obj, std::string tag);
extern Handle JValue_Release(RE::StaticFunctionTag*, Handle obj);
extern Handle JValue_ReleaseAndRetain(RE::StaticFunctionTag*, Handle previous, Handle newObj, std::string tag);
extern void JValue_ReleaseObjectsWithTag(RE::StaticFunctionTag*, std::string tag);
extern Handle JValue_ZeroLifetime(RE::StaticFunctionTag*, Handle obj);
extern Handle JValue_AddToPool(RE::StaticFunctionTag*, Handle obj, std::string poolName);
extern void JValue_CleanPool(RE::StaticFunctionTag*, std::string poolName);
extern Handle JValue_ShallowCopy(RE::StaticFunctionTag*, Handle obj);
extern Handle JValue_DeepCopy(RE::StaticFunctionTag*, Handle obj);
extern bool JValue_IsExists(RE::StaticFunctionTag*, Handle obj);
extern bool JValue_IsArray(RE::StaticFunctionTag*, Handle obj);
extern bool JValue_IsMap(RE::StaticFunctionTag*, Handle obj);
extern bool JValue_IsFormMap(RE::StaticFunctionTag*, Handle obj);
extern bool JValue_IsIntegerMap(RE::StaticFunctionTag*, Handle obj);
extern bool JValue_Empty(RE::StaticFunctionTag*, Handle obj);
extern int32_t JValue_Count(RE::StaticFunctionTag*, Handle obj);
extern void JValue_Clear(RE::StaticFunctionTag*, Handle obj);
extern Handle JValue_ReadFromFile(RE::StaticFunctionTag*, std::string filePath);
extern Handle JValue_ReadFromDirectory(RE::StaticFunctionTag*, std::string directoryPath, std::string extension);
extern Handle JValue_ObjectFromPrototype(RE::StaticFunctionTag*, std::string prototype);
extern void JValue_WriteToFile(RE::StaticFunctionTag*, Handle obj, std::string filePath);
extern std::string JValue_ToString(RE::StaticFunctionTag*, Handle obj);
extern int32_t JValue_SolvedValueType(RE::StaticFunctionTag*, Handle obj, std::string path);
extern bool JValue_HasPath(RE::StaticFunctionTag*, Handle obj, std::string path);
extern float JValue_SolveFlt(RE::StaticFunctionTag*, Handle obj, std::string path, float defaultVal);
extern int32_t JValue_SolveInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t defaultVal);
extern std::string JValue_SolveStr(RE::StaticFunctionTag*, Handle obj, std::string path, std::string defaultVal);
extern Handle JValue_SolveObj(RE::StaticFunctionTag*, Handle obj, std::string path, Handle defaultVal);
extern RE::TESForm* JValue_SolveForm(RE::StaticFunctionTag*, Handle obj, std::string path, RE::TESForm* defaultVal);
extern bool JValue_SolveFltSetter(RE::StaticFunctionTag*, Handle obj, std::string path, float value, bool createMissing);
extern bool JValue_SolveIntSetter(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, bool createMissing);
extern bool JValue_SolveStrSetter(RE::StaticFunctionTag*, Handle obj, std::string path, std::string value, bool createMissing);
extern bool JValue_SolveObjSetter(RE::StaticFunctionTag*, Handle obj, std::string path, Handle value, bool createMissing);
extern bool JValue_SolveFormSetter(RE::StaticFunctionTag*, Handle obj, std::string path, RE::TESForm* value, bool createMissing);

// ============================================================================
// JMap
// ============================================================================
extern Handle JMap_Object(RE::StaticFunctionTag*);
extern int32_t JMap_GetInt(RE::StaticFunctionTag*, Handle obj, std::string key, int32_t defaultVal);
extern float JMap_GetFlt(RE::StaticFunctionTag*, Handle obj, std::string key, float defaultVal);
extern std::string JMap_GetStr(RE::StaticFunctionTag*, Handle obj, std::string key, std::string defaultVal);
extern Handle JMap_GetObj(RE::StaticFunctionTag*, Handle obj, std::string key, Handle defaultVal);
extern RE::TESForm* JMap_GetForm(RE::StaticFunctionTag*, Handle obj, std::string key, RE::TESForm* defaultVal);
extern void JMap_SetInt(RE::StaticFunctionTag*, Handle obj, std::string key, int32_t value);
extern void JMap_SetFlt(RE::StaticFunctionTag*, Handle obj, std::string key, float value);
extern void JMap_SetStr(RE::StaticFunctionTag*, Handle obj, std::string key, std::string value);
extern void JMap_SetObj(RE::StaticFunctionTag*, Handle obj, std::string key, Handle value);
extern void JMap_SetForm(RE::StaticFunctionTag*, Handle obj, std::string key, RE::TESForm* value);
extern bool JMap_HasKey(RE::StaticFunctionTag*, Handle obj, std::string key);
extern int32_t JMap_ValueType(RE::StaticFunctionTag*, Handle obj, std::string key);
extern Handle JMap_AllKeys(RE::StaticFunctionTag*, Handle obj);
extern std::vector<std::string> JMap_AllKeysPArray(RE::StaticFunctionTag*, Handle obj);
extern Handle JMap_AllValues(RE::StaticFunctionTag*, Handle obj);
extern bool JMap_RemoveKey(RE::StaticFunctionTag*, Handle obj, std::string key);
extern int32_t JMap_Count(RE::StaticFunctionTag*, Handle obj);
extern void JMap_Clear(RE::StaticFunctionTag*, Handle obj);
extern void JMap_AddPairs(RE::StaticFunctionTag*, Handle obj, Handle source, bool overrideDuplicates);
extern std::string JMap_NextKey(RE::StaticFunctionTag*, Handle obj, std::string previousKey, std::string endKey);
extern std::string JMap_GetNthKey(RE::StaticFunctionTag*, Handle obj, int32_t keyIndex);
extern int32_t JMap_InsertInt(RE::StaticFunctionTag*, Handle obj, std::string key, int32_t value);
extern float JMap_InsertFlt(RE::StaticFunctionTag*, Handle obj, std::string key, float value);
extern std::string JMap_InsertStr(RE::StaticFunctionTag*, Handle obj, std::string key, std::string value);
extern Handle JMap_InsertObj(RE::StaticFunctionTag*, Handle obj, std::string key, Handle value);
extern RE::TESForm* JMap_InsertForm(RE::StaticFunctionTag*, Handle obj, std::string key, RE::TESForm* value);

// ============================================================================
// JArray
// ============================================================================
extern Handle JArray_Object(RE::StaticFunctionTag*);
extern Handle JArray_ObjectWithSize(RE::StaticFunctionTag*, int32_t size);
extern Handle JArray_ObjectWithInts(RE::StaticFunctionTag*, std::vector<int32_t> values);
extern Handle JArray_ObjectWithStrings(RE::StaticFunctionTag*, std::vector<std::string> values);
extern Handle JArray_ObjectWithFloats(RE::StaticFunctionTag*, std::vector<float> values);
extern Handle JArray_ObjectWithBooleans(RE::StaticFunctionTag*, std::vector<bool> values);
extern Handle JArray_ObjectWithForms(RE::StaticFunctionTag*, std::vector<RE::TESForm*> values);
extern Handle JArray_SubArray(RE::StaticFunctionTag*, Handle obj, int32_t startIndex, int32_t endIndex);
extern void JArray_AddFromArray(RE::StaticFunctionTag*, Handle obj, Handle source, int32_t insertAtIndex);
extern void JArray_AddFromFormList(RE::StaticFunctionTag*, Handle obj, RE::BGSListForm* source, int32_t insertAtIndex);
extern int32_t JArray_GetInt(RE::StaticFunctionTag*, Handle obj, int32_t index, int32_t defaultVal);
extern float JArray_GetFlt(RE::StaticFunctionTag*, Handle obj, int32_t index, float defaultVal);
extern std::string JArray_GetStr(RE::StaticFunctionTag*, Handle obj, int32_t index, std::string defaultVal);
extern Handle JArray_GetObj(RE::StaticFunctionTag*, Handle obj, int32_t index, Handle defaultVal);
extern RE::TESForm* JArray_GetForm(RE::StaticFunctionTag*, Handle obj, int32_t index, RE::TESForm* defaultVal);
extern std::vector<int32_t> JArray_AsIntArray(RE::StaticFunctionTag*, Handle obj);
extern std::vector<float> JArray_AsFloatArray(RE::StaticFunctionTag*, Handle obj);
extern std::vector<std::string> JArray_AsStringArray(RE::StaticFunctionTag*, Handle obj);
extern std::vector<RE::TESForm*> JArray_AsFormArray(RE::StaticFunctionTag*, Handle obj);
extern int32_t JArray_FindInt(RE::StaticFunctionTag*, Handle obj, int32_t value, int32_t searchStartIndex);
extern int32_t JArray_FindFlt(RE::StaticFunctionTag*, Handle obj, float value, int32_t searchStartIndex);
extern int32_t JArray_FindStr(RE::StaticFunctionTag*, Handle obj, std::string value, int32_t searchStartIndex);
extern int32_t JArray_FindObj(RE::StaticFunctionTag*, Handle obj, Handle container, int32_t searchStartIndex);
extern int32_t JArray_FindForm(RE::StaticFunctionTag*, Handle obj, RE::TESForm* value, int32_t searchStartIndex);
extern int32_t JArray_CountInteger(RE::StaticFunctionTag*, Handle obj, int32_t value);
extern int32_t JArray_CountFloat(RE::StaticFunctionTag*, Handle obj, float value);
extern int32_t JArray_CountString(RE::StaticFunctionTag*, Handle obj, std::string value);
extern int32_t JArray_CountObject(RE::StaticFunctionTag*, Handle obj, Handle container);
extern int32_t JArray_CountForm(RE::StaticFunctionTag*, Handle obj, RE::TESForm* value);
extern void JArray_SetInt(RE::StaticFunctionTag*, Handle obj, int32_t index, int32_t value);
extern void JArray_SetFlt(RE::StaticFunctionTag*, Handle obj, int32_t index, float value);
extern void JArray_SetStr(RE::StaticFunctionTag*, Handle obj, int32_t index, std::string value);
extern void JArray_SetObj(RE::StaticFunctionTag*, Handle obj, int32_t index, Handle container);
extern void JArray_SetForm(RE::StaticFunctionTag*, Handle obj, int32_t index, RE::TESForm* value);
extern void JArray_AddInt(RE::StaticFunctionTag*, Handle obj, int32_t value, int32_t addToIndex);
extern void JArray_AddFlt(RE::StaticFunctionTag*, Handle obj, float value, int32_t addToIndex);
extern void JArray_AddStr(RE::StaticFunctionTag*, Handle obj, std::string value, int32_t addToIndex);
extern void JArray_AddObj(RE::StaticFunctionTag*, Handle obj, Handle container, int32_t addToIndex);
extern void JArray_AddForm(RE::StaticFunctionTag*, Handle obj, RE::TESForm* value, int32_t addToIndex);
extern int32_t JArray_Count(RE::StaticFunctionTag*, Handle obj);
extern void JArray_Clear(RE::StaticFunctionTag*, Handle obj);
extern void JArray_EraseIndex(RE::StaticFunctionTag*, Handle obj, int32_t index);
extern void JArray_EraseRange(RE::StaticFunctionTag*, Handle obj, int32_t first, int32_t last);
extern int32_t JArray_EraseInteger(RE::StaticFunctionTag*, Handle obj, int32_t value);
extern int32_t JArray_EraseFloat(RE::StaticFunctionTag*, Handle obj, float value);
extern int32_t JArray_EraseString(RE::StaticFunctionTag*, Handle obj, std::string value);
extern int32_t JArray_EraseObject(RE::StaticFunctionTag*, Handle obj, Handle container);
extern int32_t JArray_EraseForm(RE::StaticFunctionTag*, Handle obj, RE::TESForm* value);
extern int32_t JArray_ValueType(RE::StaticFunctionTag*, Handle obj, int32_t index);
extern void JArray_SwapItems(RE::StaticFunctionTag*, Handle obj, int32_t index1, int32_t index2);
extern Handle JArray_Sort(RE::StaticFunctionTag*, Handle obj);
extern Handle JArray_Unique(RE::StaticFunctionTag*, Handle obj);
extern Handle JArray_Reverse(RE::StaticFunctionTag*, Handle obj);
extern bool JArray_WriteToIntegerPArray(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackID, RE::StaticFunctionTag*, Handle obj, std::vector<int32_t> decoy, int32_t writeAtIdx, int32_t stopWriteAtIdx, int32_t readIdx, int32_t defaultRead);
extern bool JArray_WriteToFloatPArray(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackID, RE::StaticFunctionTag*, Handle obj, std::vector<float> decoy, int32_t writeAtIdx, int32_t stopWriteAtIdx, int32_t readIdx, float defaultRead);
extern bool JArray_WriteToFormPArray(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackID, RE::StaticFunctionTag*, Handle obj, std::vector<RE::TESForm*> decoy, int32_t writeAtIdx, int32_t stopWriteAtIdx, int32_t readIdx, RE::TESForm* defaultRead);
extern bool JArray_WriteToStringPArray(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackID, RE::StaticFunctionTag*, Handle obj, std::vector<std::string> decoy, int32_t writeAtIdx, int32_t stopWriteAtIdx, int32_t readIdx, std::string defaultRead);

// ============================================================================
// JDB
// ============================================================================
extern float JDB_SolveFlt(RE::StaticFunctionTag*, std::string path, float defaultVal);
extern int32_t JDB_SolveInt(RE::StaticFunctionTag*, std::string path, int32_t defaultVal);
extern std::string JDB_SolveStr(RE::StaticFunctionTag*, std::string path, std::string defaultVal);
extern Handle JDB_SolveObj(RE::StaticFunctionTag*, std::string path, Handle defaultVal);
extern RE::TESForm* JDB_SolveForm(RE::StaticFunctionTag*, std::string path, RE::TESForm* defaultVal);
extern bool JDB_SolveFltSetter(RE::StaticFunctionTag*, std::string path, float value, bool createMissing);
extern bool JDB_SolveIntSetter(RE::StaticFunctionTag*, std::string path, int32_t value, bool createMissing);
extern bool JDB_SolveStrSetter(RE::StaticFunctionTag*, std::string path, std::string value, bool createMissing);
extern bool JDB_SolveObjSetter(RE::StaticFunctionTag*, std::string path, Handle value, bool createMissing);
extern bool JDB_SolveFormSetter(RE::StaticFunctionTag*, std::string path, RE::TESForm* value, bool createMissing);
extern void JDB_SetObj(RE::StaticFunctionTag*, std::string path, Handle obj);
extern bool JDB_HasPath(RE::StaticFunctionTag*, std::string path);
extern Handle JDB_AllKeys(RE::StaticFunctionTag*);
extern Handle JDB_AllValues(RE::StaticFunctionTag*);
extern void JDB_WriteToFile(RE::StaticFunctionTag*, std::string path);
extern Handle JDB_Root(RE::StaticFunctionTag*);

// ============================================================================
// JFormDB
// ============================================================================
extern void JFormDB_SetEntry(RE::StaticFunctionTag*, std::string storageName, RE::TESForm* fKey, Handle entry);
extern Handle JFormDB_MakeEntry(RE::StaticFunctionTag*, std::string storageName, RE::TESForm* fKey);
extern Handle JFormDB_FindEntry(RE::StaticFunctionTag*, std::string storageName, RE::TESForm* fKey);
extern float JFormDB_SolveFlt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, float defaultVal);
extern int32_t JFormDB_SolveInt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, int32_t defaultVal);
extern std::string JFormDB_SolveStr(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, std::string defaultVal);
extern Handle JFormDB_SolveObj(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, Handle defaultVal);
extern RE::TESForm* JFormDB_SolveForm(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, RE::TESForm* defaultVal);
extern bool JFormDB_SolveFltSetter(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, float value, bool createMissing);
extern bool JFormDB_SolveIntSetter(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, int32_t value, bool createMissing);
extern bool JFormDB_SolveStrSetter(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, std::string value, bool createMissing);
extern bool JFormDB_SolveObjSetter(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, Handle value, bool createMissing);
extern bool JFormDB_SolveFormSetter(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, RE::TESForm* value, bool createMissing);
extern bool JFormDB_HasPath(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path);
extern Handle JFormDB_AllKeys(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path);
extern Handle JFormDB_AllValues(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path);
extern int32_t JFormDB_GetInt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path);
extern float JFormDB_GetFlt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path);
extern std::string JFormDB_GetStr(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path);
extern Handle JFormDB_GetObj(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path);
extern RE::TESForm* JFormDB_GetForm(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path);
extern void JFormDB_SetInt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, int32_t value);
extern void JFormDB_SetFlt(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, float value);
extern void JFormDB_SetStr(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, std::string value);
extern void JFormDB_SetObj(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, Handle value);
extern void JFormDB_SetForm(RE::StaticFunctionTag*, RE::TESForm* fKey, std::string path, RE::TESForm* value);

// ============================================================================
// JFormMap
// ============================================================================
extern Handle JFormMap_Object(RE::StaticFunctionTag*);
extern int32_t JFormMap_GetInt(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, int32_t defaultVal);
extern float JFormMap_GetFlt(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, float defaultVal);
extern std::string JFormMap_GetStr(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, std::string defaultVal);
extern Handle JFormMap_GetObj(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, Handle defaultVal);
extern RE::TESForm* JFormMap_GetForm(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, RE::TESForm* defaultVal);
extern void JFormMap_SetInt(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, int32_t value);
extern void JFormMap_SetFlt(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, float value);
extern void JFormMap_SetStr(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, std::string value);
extern void JFormMap_SetObj(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, Handle container);
extern void JFormMap_SetForm(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, RE::TESForm* value);
extern bool JFormMap_HasKey(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key);
extern int32_t JFormMap_ValueType(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key);
extern Handle JFormMap_AllKeys(RE::StaticFunctionTag*, Handle obj);
extern std::vector<RE::TESForm*> JFormMap_AllKeysPArray(RE::StaticFunctionTag*, Handle obj);
extern Handle JFormMap_AllValues(RE::StaticFunctionTag*, Handle obj);
extern bool JFormMap_RemoveKey(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key);
extern int32_t JFormMap_Count(RE::StaticFunctionTag*, Handle obj);
extern void JFormMap_Clear(RE::StaticFunctionTag*, Handle obj);
extern void JFormMap_AddPairs(RE::StaticFunctionTag*, Handle obj, Handle source, bool overrideDuplicates);
extern RE::TESForm* JFormMap_NextKey(RE::StaticFunctionTag*, Handle obj, RE::TESForm* previousKey, RE::TESForm* endKey);
extern RE::TESForm* JFormMap_GetNthKey(RE::StaticFunctionTag*, Handle obj, int32_t keyIndex);
extern int32_t JFormMap_InsertInt(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, int32_t value);
extern float JFormMap_InsertFlt(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, float value);
extern std::string JFormMap_InsertStr(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, std::string value);
extern Handle JFormMap_InsertObj(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, Handle value);
extern RE::TESForm* JFormMap_InsertForm(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, RE::TESForm* value);

// ============================================================================
// JIntMap
// ============================================================================
extern Handle JIntMap_Object(RE::StaticFunctionTag*);
extern int32_t JIntMap_GetInt(RE::StaticFunctionTag*, Handle obj, int32_t key, int32_t defaultVal);
extern float JIntMap_GetFlt(RE::StaticFunctionTag*, Handle obj, int32_t key, float defaultVal);
extern std::string JIntMap_GetStr(RE::StaticFunctionTag*, Handle obj, int32_t key, std::string defaultVal);
extern Handle JIntMap_GetObj(RE::StaticFunctionTag*, Handle obj, int32_t key, Handle defaultVal);
extern RE::TESForm* JIntMap_GetForm(RE::StaticFunctionTag*, Handle obj, int32_t key, RE::TESForm* defaultVal);
extern void JIntMap_SetInt(RE::StaticFunctionTag*, Handle obj, int32_t key, int32_t value);
extern void JIntMap_SetFlt(RE::StaticFunctionTag*, Handle obj, int32_t key, float value);
extern void JIntMap_SetStr(RE::StaticFunctionTag*, Handle obj, int32_t key, std::string value);
extern void JIntMap_SetObj(RE::StaticFunctionTag*, Handle obj, int32_t key, Handle container);
extern void JIntMap_SetForm(RE::StaticFunctionTag*, Handle obj, int32_t key, RE::TESForm* value);
extern bool JIntMap_HasKey(RE::StaticFunctionTag*, Handle obj, int32_t key);
extern int32_t JIntMap_ValueType(RE::StaticFunctionTag*, Handle obj, int32_t key);
extern Handle JIntMap_AllKeys(RE::StaticFunctionTag*, Handle obj);
extern std::vector<int32_t> JIntMap_AllKeysPArray(RE::StaticFunctionTag*, Handle obj);
extern Handle JIntMap_AllValues(RE::StaticFunctionTag*, Handle obj);
extern bool JIntMap_RemoveKey(RE::StaticFunctionTag*, Handle obj, int32_t key);
extern int32_t JIntMap_Count(RE::StaticFunctionTag*, Handle obj);
extern void JIntMap_Clear(RE::StaticFunctionTag*, Handle obj);
extern void JIntMap_AddPairs(RE::StaticFunctionTag*, Handle obj, Handle source, bool overrideDuplicates);
extern int32_t JIntMap_NextKey(RE::StaticFunctionTag*, Handle obj, int32_t previousKey, int32_t endKey);
extern int32_t JIntMap_GetNthKey(RE::StaticFunctionTag*, Handle obj, int32_t keyIndex);
extern int32_t JIntMap_InsertInt(RE::StaticFunctionTag*, Handle obj, int32_t key, int32_t value);
extern float JIntMap_InsertFlt(RE::StaticFunctionTag*, Handle obj, int32_t key, float value);
extern std::string JIntMap_InsertStr(RE::StaticFunctionTag*, Handle obj, int32_t key, std::string value);
extern Handle JIntMap_InsertObj(RE::StaticFunctionTag*, Handle obj, int32_t key, Handle value);
extern RE::TESForm* JIntMap_InsertForm(RE::StaticFunctionTag*, Handle obj, int32_t key, RE::TESForm* value);

// ============================================================================
// JAtomic
// ============================================================================
extern int32_t JAtomic_fetchAddInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, int32_t initialValue, bool createMissingKeys, int32_t onErrorReturn);
extern float JAtomic_fetchAddFlt(RE::StaticFunctionTag*, Handle obj, std::string path, float value, float initialValue, bool createMissingKeys, float onErrorReturn);
extern int32_t JAtomic_fetchMultInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, int32_t initialValue, bool createMissingKeys, int32_t onErrorReturn);
extern float JAtomic_fetchMultFlt(RE::StaticFunctionTag*, Handle obj, std::string path, float value, float initialValue, bool createMissingKeys, float onErrorReturn);
extern int32_t JAtomic_fetchModInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, int32_t initialValue, bool createMissingKeys, int32_t onErrorReturn);
extern int32_t JAtomic_fetchDivInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, int32_t initialValue, bool createMissingKeys, int32_t onErrorReturn);
extern float JAtomic_fetchDivFlt(RE::StaticFunctionTag*, Handle obj, std::string path, float value, float initialValue, bool createMissingKeys, float onErrorReturn);
extern int32_t JAtomic_fetchAndInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, int32_t initialValue, bool createMissingKeys, int32_t onErrorReturn);
extern int32_t JAtomic_fetchXorInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, int32_t initialValue, bool createMissingKeys, int32_t onErrorReturn);
extern int32_t JAtomic_fetchOrInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, int32_t initialValue, bool createMissingKeys, int32_t onErrorReturn);
extern int32_t JAtomic_exchangeInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, bool createMissingKeys, int32_t onErrorReturn);
extern float JAtomic_exchangeFlt(RE::StaticFunctionTag*, Handle obj, std::string path, float value, bool createMissingKeys, float onErrorReturn);
extern std::string JAtomic_exchangeStr(RE::StaticFunctionTag*, Handle obj, std::string path, std::string value, bool createMissingKeys, std::string onErrorReturn);
extern RE::TESForm* JAtomic_exchangeForm(RE::StaticFunctionTag*, Handle obj, std::string path, RE::TESForm* value, bool createMissingKeys, RE::TESForm* onErrorReturn);
extern Handle JAtomic_exchangeObj(RE::StaticFunctionTag*, Handle obj, std::string path, Handle value, bool createMissingKeys, Handle onErrorReturn);
extern int32_t JAtomic_compareExchangeInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t desired, int32_t expected, bool createMissingKeys, int32_t onErrorReturn);
extern float JAtomic_compareExchangeFlt(RE::StaticFunctionTag*, Handle obj, std::string path, float desired, float expected, bool createMissingKeys, float onErrorReturn);
extern std::string JAtomic_compareExchangeStr(RE::StaticFunctionTag*, Handle obj, std::string path, std::string desired, std::string expected, bool createMissingKeys, std::string onErrorReturn);
extern RE::TESForm* JAtomic_compareExchangeForm(RE::StaticFunctionTag*, Handle obj, std::string path, RE::TESForm* desired, RE::TESForm* expected, bool createMissingKeys, RE::TESForm* onErrorReturn);
extern Handle JAtomic_compareExchangeObj(RE::StaticFunctionTag*, Handle obj, std::string path, Handle desired, Handle expected, bool createMissingKeys, Handle onErrorReturn);

// ============================================================================
// JString
// ============================================================================
extern int32_t JString_Wrap(RE::StaticFunctionTag*, std::string sourceText, int32_t charactersPerLine);
extern int32_t JString_DecodeFormStringToFormId(RE::StaticFunctionTag*, std::string formString);
extern RE::TESForm* JString_DecodeFormStringToForm(RE::StaticFunctionTag*, std::string formString);
extern std::string JString_EncodeFormToString(RE::StaticFunctionTag*, RE::TESForm* value);
extern std::string JString_EncodeFormIdToString(RE::StaticFunctionTag*, int32_t formId);
extern std::string JString_GenerateUUID(RE::StaticFunctionTag*);

// ============================================================================
// JContainers
// ============================================================================
extern bool JContainers_IsInstalled(RE::StaticFunctionTag*);
extern int32_t JContainers_APIVersion(RE::StaticFunctionTag*);
extern int32_t JContainers_FeatureVersion(RE::StaticFunctionTag*);
extern bool JContainers_FileExistsAtPath(RE::StaticFunctionTag*, std::string path);
extern std::vector<std::string> JContainers_ContentsOfDirectoryAtPath(RE::StaticFunctionTag*, std::string directoryPath, std::string extension);
extern void JContainers_RemoveFileAtPath(RE::StaticFunctionTag*, std::string path);
extern std::string JContainers_UserDirectory(RE::StaticFunctionTag*);

// ============================================================================
// JLua
// ============================================================================
extern float JLua_EvalLuaFlt(RE::StaticFunctionTag*, std::string luaCode, Handle transport, float defaultVal, bool minimizeLifetime);
extern int32_t JLua_EvalLuaInt(RE::StaticFunctionTag*, std::string luaCode, Handle transport, int32_t defaultVal, bool minimizeLifetime);
extern std::string JLua_EvalLuaStr(RE::StaticFunctionTag*, std::string luaCode, Handle transport, std::string defaultVal, bool minimizeLifetime);
extern Handle JLua_EvalLuaObj(RE::StaticFunctionTag*, std::string luaCode, Handle transport, Handle defaultVal, bool minimizeLifetime);
extern RE::TESForm* JLua_EvalLuaForm(RE::StaticFunctionTag*, std::string luaCode, Handle transport, RE::TESForm* defaultVal, bool minimizeLifetime);
extern Handle JLua_SetStr(RE::StaticFunctionTag*, std::string key, std::string value, Handle transport);
extern Handle JLua_SetFlt(RE::StaticFunctionTag*, std::string key, float value, Handle transport);
extern Handle JLua_SetInt(RE::StaticFunctionTag*, std::string key, int32_t value, Handle transport);
extern Handle JLua_SetForm(RE::StaticFunctionTag*, std::string key, RE::TESForm* value, Handle transport);
extern Handle JLua_SetObj(RE::StaticFunctionTag*, std::string key, Handle value, Handle transport);

// ============================================================================
// JValue.evalLua* backward-compat wrappers
// ============================================================================
extern float JValue_EvalLuaFlt(RE::StaticFunctionTag*, Handle object, std::string luaCode, float defaultVal);
extern int32_t JValue_EvalLuaInt(RE::StaticFunctionTag*, Handle object, std::string luaCode, int32_t defaultVal);
extern std::string JValue_EvalLuaStr(RE::StaticFunctionTag*, Handle object, std::string luaCode, std::string defaultVal);
extern Handle JValue_EvalLuaObj(RE::StaticFunctionTag*, Handle object, std::string luaCode, Handle defaultVal);
extern RE::TESForm* JValue_EvalLuaForm(RE::StaticFunctionTag*, Handle object, std::string luaCode, RE::TESForm* defaultVal);
