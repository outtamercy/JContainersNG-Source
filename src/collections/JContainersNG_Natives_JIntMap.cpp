//JContainersNG_Natives_JIntMap.cpp
#include "JContainersNG_Natives.hpp"

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
        ObjectManager::Get().EmbedEdge(val, newHandle);
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
    auto keyStr = std::to_string(key);
    auto it = ptr->find(keyStr);
    if (it != ptr->end()) ObjectManager::Get().ReleaseEdge(it.value());
    (*ptr)[keyStr] = value;
}

void JIntMap_SetFlt(RE::StaticFunctionTag*, Handle obj, int32_t key, float value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    auto keyStr = std::to_string(key);
    auto it = ptr->find(keyStr);
    if (it != ptr->end()) ObjectManager::Get().ReleaseEdge(it.value());
    (*ptr)[keyStr] = value;
}

void JIntMap_SetStr(RE::StaticFunctionTag*, Handle obj, int32_t key, std::string value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    auto keyStr = std::to_string(key);
    auto it = ptr->find(keyStr);
    if (it != ptr->end()) ObjectManager::Get().ReleaseEdge(it.value());
    (*ptr)[keyStr] = value;
}

void JIntMap_SetObj(RE::StaticFunctionTag*, Handle obj, int32_t key, Handle container) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    auto keyStr = std::to_string(key);
    if (container == 0) {
        auto it = ptr->find(keyStr);
        if (it != ptr->end()) {
            ObjectManager::Get().ReleaseEdge(it.value());
            ptr->erase(it);
        }
        return;
    }
    ObjectManager::Get().EmbedEdge((*ptr)[keyStr], container);
}

void JIntMap_SetForm(RE::StaticFunctionTag*, Handle obj, int32_t key, RE::TESForm* value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    auto keyStr = std::to_string(key);
    if (!value) {
        auto it = ptr->find(keyStr);
        if (it != ptr->end()) {
            ObjectManager::Get().ReleaseEdge(it.value());
            ptr->erase(it);
        }
        return;
    }
    auto it = ptr->find(keyStr);
    if (it != ptr->end()) ObjectManager::Get().ReleaseEdge(it.value());
    (*ptr)[keyStr] = FormSerializer::EncodeForm(value);
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
        Handle h;
        if (ObjectManager::IsRef(val, &h)) {
            json slot;
            ObjectManager::Get().EmbedEdge(slot, h);
            arrPtr->push_back(std::move(slot));
        }
        else {
            arrPtr->push_back(val);
        }
    }
    return arr;
}

bool JIntMap_RemoveKey(RE::StaticFunctionTag*, Handle obj, int32_t key) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return false;
    auto it = ptr->find(std::to_string(key));
    if (it == ptr->end()) return false;
    ObjectManager::Get().ReleaseEdge(it.value());
    ptr->erase(it);
    return true;
}

int32_t JIntMap_Count(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    return static_cast<int32_t>(ptr->size());
}

void JIntMap_Clear(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    ObjectManager::Get().ReleaseAllEdges(*ptr);
    ptr->clear();
}

void JIntMap_AddPairs(RE::StaticFunctionTag*, Handle obj, Handle source, bool overrideDuplicates) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    auto srcPtr = ObjectManager::Get().GetObject(source);
    if (!ptr || !ptr->is_object() || !srcPtr || !srcPtr->is_object()) return;
    for (auto& [key, val] : srcPtr->items()) {
        if (!overrideDuplicates && ptr->contains(key)) continue;
        Handle h;
        if (ObjectManager::IsRef(val, &h)) {
            ObjectManager::Get().EmbedEdge((*ptr)[key], h);
        }
        else {
            auto it = ptr->find(key);
            if (it != ptr->end()) ObjectManager::Get().ReleaseEdge(it.value());
            (*ptr)[key] = val;
        }
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

int32_t JIntMap_InsertInt(RE::StaticFunctionTag*, Handle obj, int32_t key, int32_t value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    auto keyStr = std::to_string(key);
    if (ptr->contains(keyStr)) {
        auto& val = (*ptr)[keyStr];
        if (val.is_number_integer()) return val.get<int32_t>();
        if (val.is_number_float()) return static_cast<int32_t>(val.get<double>());
        return 0;
    }
    (*ptr)[keyStr] = value;
    return value;
}

float JIntMap_InsertFlt(RE::StaticFunctionTag*, Handle obj, int32_t key, float value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0.0f;
    auto keyStr = std::to_string(key);
    if (ptr->contains(keyStr)) {
        auto& val = (*ptr)[keyStr];
        if (val.is_number()) return val.get<float>();
        return 0.0f;
    }
    (*ptr)[keyStr] = value;
    return value;
}

std::string JIntMap_InsertStr(RE::StaticFunctionTag*, Handle obj, int32_t key, std::string value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return "";
    auto keyStr = std::to_string(key);
    if (ptr->contains(keyStr)) {
        auto& val = (*ptr)[keyStr];
        if (val.is_string()) return val.get<std::string>();
        return "";
    }
    (*ptr)[keyStr] = value;
    return value;
}

Handle JIntMap_InsertObj(RE::StaticFunctionTag*, Handle obj, int32_t key, Handle value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    auto keyStr = std::to_string(key);
    if (ptr->contains(keyStr)) {
        auto& val = (*ptr)[keyStr];
        Handle refHandle;
        if (ObjectManager::IsRef(val, &refHandle)) return refHandle;
        if (val.is_object() || val.is_array()) {
            Handle newHandle = ObjectManager::Get().RegisterObject(std::move(val));
            ObjectManager::Get().EmbedEdge(val, newHandle);
            return newHandle;
        }
        return 0;
    }
    if (value == 0) return 0;
    ObjectManager::Get().EmbedEdge((*ptr)[keyStr], value);
    return value;
}

RE::TESForm* JIntMap_InsertForm(RE::StaticFunctionTag*, Handle obj, int32_t key, RE::TESForm* value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return nullptr;
    auto keyStr = std::to_string(key);
    if (ptr->contains(keyStr)) {
        auto& val = (*ptr)[keyStr];
        if (val.is_string()) {
            std::string str = val.get<std::string>();
            if (FormSerializer::IsFormString(str)) return FormSerializer::DecodeForm(str);
        }
        return nullptr;
    }
    if (!value) return nullptr;
    (*ptr)[keyStr] = FormSerializer::EncodeForm(value);
    return value;
}