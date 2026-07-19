//JContainersNG_JMap.cpp
#include "JContainersNG_Natives.hpp"

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
        ObjectManager::Get().EmbedEdge(val, newHandle);
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
        ObjectManager::Get().ReleaseEdge(it.value()); // old value might be a ref
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
        ObjectManager::Get().ReleaseEdge(it.value());
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
        ObjectManager::Get().ReleaseEdge(it.value());
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
        if (it != ptr->end()) {
            ObjectManager::Get().ReleaseEdge(it.value());
            ptr->erase(it);
        }
        return;
    }
    auto it = FindKeyCI(*ptr, key);
    if (it != ptr->end()) {
        ObjectManager::Get().EmbedEdge(it.value(), value);
    }
    else {
        ObjectManager::Get().EmbedEdge((*ptr)[key], value);
    }
}

void JMap_SetForm(RE::StaticFunctionTag*, Handle obj, std::string key, RE::TESForm* value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    if (!value) {
        auto it = FindKeyCI(*ptr, key);
        if (it != ptr->end()) {
            ObjectManager::Get().ReleaseEdge(it.value());
            ptr->erase(it);
        }
        return;
    }
    auto it = FindKeyCI(*ptr, key);
    if (it != ptr->end()) {
        ObjectManager::Get().ReleaseEdge(it.value());
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
        Handle h;
        if (ObjectManager::IsRef(val, &h)) {
            // copied ref = new edge on the new array (was a naked Retain before,
            // tied to nothing — leaked the moment the array died)
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

bool JMap_RemoveKey(RE::StaticFunctionTag*, Handle obj, std::string key) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return false;
    auto it = FindKeyCI(*ptr, key);
    if (it != ptr->end()) {
        ObjectManager::Get().ReleaseEdge(it.value());
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
    ObjectManager::Get().ReleaseAllEdges(*ptr);
    ptr->clear();
}

void JMap_AddPairs(RE::StaticFunctionTag*, Handle obj, Handle source, bool overrideDuplicates) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    auto srcPtr = ObjectManager::Get().GetObject(source);
    if (!ptr || !ptr->is_object() || !srcPtr || !srcPtr->is_object()) return;
    for (auto& [key, val] : srcPtr->items()) {
        auto it = FindKeyCI(*ptr, key);
        if (!overrideDuplicates && it != ptr->end()) continue;
        Handle h;
        if (ObjectManager::IsRef(val, &h)) {
            if (it != ptr->end()) {
                ObjectManager::Get().EmbedEdge(it.value(), h);
            }
            else {
                ObjectManager::Get().EmbedEdge((*ptr)[key], h);
            }
        }
        else {
            if (it != ptr->end()) {
                ObjectManager::Get().ReleaseEdge(it.value()); // old value mightve been a ref
                it.value() = val;
            }
            else {
                (*ptr)[key] = val;
            }
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

int32_t JMap_InsertInt(RE::StaticFunctionTag*, Handle obj, std::string key, int32_t value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    auto it = FindKeyCI(*ptr, key);
    if (it != ptr->end()) {
        auto& val = it.value();
        if (val.is_number_integer()) return val.get<int32_t>();
        if (val.is_number_float()) return static_cast<int32_t>(val.get<double>());
        return 0;
    }
    (*ptr)[key] = value;
    return value;
}

float JMap_InsertFlt(RE::StaticFunctionTag*, Handle obj, std::string key, float value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0.0f;
    auto it = FindKeyCI(*ptr, key);
    if (it != ptr->end()) {
        auto& val = it.value();
        if (val.is_number()) return val.get<float>();
        return 0.0f;
    }
    (*ptr)[key] = value;
    return value;
}

std::string JMap_InsertStr(RE::StaticFunctionTag*, Handle obj, std::string key, std::string value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return "";
    auto it = FindKeyCI(*ptr, key);
    if (it != ptr->end()) {
        auto& val = it.value();
        if (val.is_string()) return val.get<std::string>();
        return "";
    }
    (*ptr)[key] = value;
    return value;
}

Handle JMap_InsertObj(RE::StaticFunctionTag*, Handle obj, std::string key, Handle value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    auto it = FindKeyCI(*ptr, key);
    if (it != ptr->end()) {
        auto& val = it.value();
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
    ObjectManager::Get().EmbedEdge((*ptr)[key], value);
    return value;
}

RE::TESForm* JMap_InsertForm(RE::StaticFunctionTag*, Handle obj, std::string key, RE::TESForm* value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return nullptr;
    auto it = FindKeyCI(*ptr, key);
    if (it != ptr->end()) {
        auto& val = it.value();
        if (val.is_string()) {
            std::string str = val.get<std::string>();
            if (FormSerializer::IsFormString(str)) return FormSerializer::DecodeForm(str);
        }
        return nullptr;
    }
    if (!value) return nullptr;
    (*ptr)[key] = FormSerializer::EncodeForm(value);
    return value;
}