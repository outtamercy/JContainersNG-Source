//JContainersNG_JFormMap.cpp
#include "JContainersNG_Natives.hpp"

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
        ObjectManager::Get().EmbedEdge(val, newHandle);
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
    auto formStr = FormSerializer::EncodeForm(key);
    auto it = ptr->find(formStr);
    if (it != ptr->end()) ObjectManager::Get().ReleaseEdge(it.value());
    (*ptr)[formStr] = value;
}

void JFormMap_SetFlt(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, float value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return;
    auto formStr = FormSerializer::EncodeForm(key);
    auto it = ptr->find(formStr);
    if (it != ptr->end()) ObjectManager::Get().ReleaseEdge(it.value());
    (*ptr)[formStr] = value;
}

void JFormMap_SetStr(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, std::string value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return;
    auto formStr = FormSerializer::EncodeForm(key);
    auto it = ptr->find(formStr);
    if (it != ptr->end()) ObjectManager::Get().ReleaseEdge(it.value());
    (*ptr)[formStr] = value;
}

void JFormMap_SetObj(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, Handle container) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return;
    auto formStr = FormSerializer::EncodeForm(key);
    if (container == 0) {
        auto it = ptr->find(formStr);
        if (it != ptr->end()) {
            ObjectManager::Get().ReleaseEdge(it.value());
            ptr->erase(it);
        }
        return;
    }
    ObjectManager::Get().EmbedEdge((*ptr)[formStr], container);
}

void JFormMap_SetForm(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, RE::TESForm* value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return;
    auto formStr = FormSerializer::EncodeForm(key);
    if (!value) {
        auto it = ptr->find(formStr);
        if (it != ptr->end()) {
            ObjectManager::Get().ReleaseEdge(it.value());
            ptr->erase(it);
        }
        return;
    }
    auto it = ptr->find(formStr);
    if (it != ptr->end()) ObjectManager::Get().ReleaseEdge(it.value());
    (*ptr)[formStr] = FormSerializer::EncodeForm(value);
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
        arrPtr->push_back(key);
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

bool JFormMap_RemoveKey(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return false;
    auto it = ptr->find(FormSerializer::EncodeForm(key));
    if (it == ptr->end()) return false;
    ObjectManager::Get().ReleaseEdge(it.value());
    ptr->erase(it);
    return true;
}

int32_t JFormMap_Count(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return 0;
    return static_cast<int32_t>(ptr->size());
}

void JFormMap_Clear(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return;
    ObjectManager::Get().ReleaseAllEdges(*ptr);
    ptr->clear();
}

void JFormMap_AddPairs(RE::StaticFunctionTag*, Handle obj, Handle source, bool overrideDuplicates) {
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

RE::TESForm* JFormMap_NextKey(RE::StaticFunctionTag*, Handle obj, RE::TESForm* previousKey, RE::TESForm* endKey) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object()) return endKey;

    std::string prevStr = previousKey ? FormSerializer::EncodeForm(previousKey) : "";
    std::string endStr = endKey ? FormSerializer::EncodeForm(endKey) : "";

    // OG skips any key that doesnt resolve to a loaded form. returning None
    // mid-iteration looks like end-of-loop to Papyrus (while key != None),
    // so one dead key would silently hide every entry after it
    auto firstLiveForm = [&](auto it) -> RE::TESForm* {
        for (; it != ptr->end(); ++it) {
            if (auto* form = FormSerializer::DecodeForm(it.key())) {
                return form;
            }
        }
        return endKey;
        };

    if (prevStr == endStr) {
        return firstLiveForm(ptr->begin());
    }

    for (auto it = ptr->begin(); it != ptr->end(); ++it) {
        if (it.key() == prevStr) {
            return firstLiveForm(std::next(it));
        }
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

int32_t JFormMap_InsertInt(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, int32_t value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return 0;
    auto formStr = FormSerializer::EncodeForm(key);
    if (ptr->contains(formStr)) {
        auto& val = (*ptr)[formStr];
        if (val.is_number_integer()) return val.get<int32_t>();
        if (val.is_number_float()) return static_cast<int32_t>(val.get<double>());
        return 0;
    }
    (*ptr)[formStr] = value;
    return value;
}

float JFormMap_InsertFlt(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, float value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return 0.0f;
    auto formStr = FormSerializer::EncodeForm(key);
    if (ptr->contains(formStr)) {
        auto& val = (*ptr)[formStr];
        if (val.is_number()) return val.get<float>();
        return 0.0f;
    }
    (*ptr)[formStr] = value;
    return value;
}

std::string JFormMap_InsertStr(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, std::string value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return "";
    auto formStr = FormSerializer::EncodeForm(key);
    if (ptr->contains(formStr)) {
        auto& val = (*ptr)[formStr];
        if (val.is_string()) return val.get<std::string>();
        return "";
    }
    (*ptr)[formStr] = value;
    return value;
}

Handle JFormMap_InsertObj(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, Handle value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return 0;
    auto formStr = FormSerializer::EncodeForm(key);
    if (ptr->contains(formStr)) {
        auto& val = (*ptr)[formStr];
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
    ObjectManager::Get().EmbedEdge((*ptr)[formStr], value);
    return value;
}

RE::TESForm* JFormMap_InsertForm(RE::StaticFunctionTag*, Handle obj, RE::TESForm* key, RE::TESForm* value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_object() || !key) return nullptr;
    auto formStr = FormSerializer::EncodeForm(key);
    if (ptr->contains(formStr)) {
        auto& val = (*ptr)[formStr];
        if (val.is_string()) {
            std::string str = val.get<std::string>();
            if (FormSerializer::IsFormString(str)) return FormSerializer::DecodeForm(str);
        }
        return nullptr;
    }
    if (!value) return nullptr;
    (*ptr)[formStr] = FormSerializer::EncodeForm(value);
    return value;
}