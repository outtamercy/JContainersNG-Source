//JContainersNG_JArray.cpp
#include "JContainersNG_Natives.hpp"

Handle JArray_Object(RE::StaticFunctionTag*) {
    return ObjectManager::Get().CreateArray();
}

Handle JArray_ObjectWithSize(RE::StaticFunctionTag*, int32_t size) {
    if (size < 0) return 0;
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
        Handle child;
        if (ObjectManager::IsRef((*ptr)[i], &child)) {
            // copied ref = new edge on the new array
            json slot;
            ObjectManager::Get().EmbedEdge(slot, child);
            newPtr->push_back(std::move(slot));
        }
        else {
            newPtr->push_back((*ptr)[i]);
        }
    }
    return result;
}

void JArray_AddFromArray(RE::StaticFunctionTag*, Handle obj, Handle source, int32_t insertAtIndex) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    auto srcPtr = ObjectManager::Get().GetObject(source);
    if (!ptr || !ptr->is_array() || !srcPtr || !srcPtr->is_array()) return;

    // refs crossing into this array gotta be counted as edges
    auto makeElement = [](const json& elem) {
        Handle child;
        if (ObjectManager::IsRef(elem, &child)) {
            json slot;
            ObjectManager::Get().EmbedEdge(slot, child);
            return slot;
        }
        return json(elem);
        };

    if (insertAtIndex < 0 || insertAtIndex >= static_cast<int32_t>(ptr->size())) {
        for (auto& elem : *srcPtr) ptr->push_back(makeElement(elem));
    }
    else {
        auto it = ptr->begin() + insertAtIndex;
        for (auto& elem : *srcPtr) {
            it = ptr->insert(it, makeElement(elem));
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
        // promoted inline json becomes a real edge on the parent
        ObjectManager::Get().EmbedEdge(val, newHandle);
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
        // type-strict like OG — float search doesnt match int elements
        if (val.is_number_float() && val.get<float>() == value) return i;
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
        if (val.is_string() && JCStrEqualsCI(val.get<std::string>(), value)) return i;
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
        if (elem.is_number_float() && elem.get<float>() == value) ++count;
    }
    return count;
}

int32_t JArray_CountString(RE::StaticFunctionTag*, Handle obj, std::string value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return 0;
    int32_t count = 0;
    for (auto& elem : *ptr) {
        if (elem.is_string() && JCStrEqualsCI(elem.get<std::string>(), value)) ++count;
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
    ObjectManager::Get().ReleaseEdge((*ptr)[index]); // slot might currently hold a ref
    (*ptr)[index] = value;
}

void JArray_SetFlt(RE::StaticFunctionTag*, Handle obj, int32_t index, float value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return;
    ObjectManager::Get().ReleaseEdge((*ptr)[index]);
    (*ptr)[index] = value;
}

void JArray_SetStr(RE::StaticFunctionTag*, Handle obj, int32_t index, std::string value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return;
    ObjectManager::Get().ReleaseEdge((*ptr)[index]);
    (*ptr)[index] = value;
}

void JArray_SetObj(RE::StaticFunctionTag*, Handle obj, int32_t index, Handle container) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return;
    // 0 clears the slot, anything else releases old edge + retains new
    ObjectManager::Get().EmbedEdge((*ptr)[index], container);
}

void JArray_SetForm(RE::StaticFunctionTag*, Handle obj, int32_t index, RE::TESForm* value) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return;
    ObjectManager::Get().ReleaseEdge((*ptr)[index]);
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
    json val;
    ObjectManager::Get().EmbedEdge(val, container); // 0 -> nullptr, else retained ref
    if (addToIndex < 0 || addToIndex >= static_cast<int32_t>(ptr->size())) {
        ptr->push_back(std::move(val));
    }
    else {
        ptr->insert(ptr->begin() + addToIndex, std::move(val));
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
    // edges out first, THEN the wipe — reverse order leaks the children
    ObjectManager::Get().ReleaseAllEdges(*ptr);
    ptr->clear();
}

void JArray_EraseIndex(RE::StaticFunctionTag*, Handle obj, int32_t index) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return;
    if (index < 0) index = static_cast<int32_t>(ptr->size()) + index;
    if (index < 0 || index >= static_cast<int32_t>(ptr->size())) return;
    ObjectManager::Get().ReleaseEdge((*ptr)[index]);
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
    for (int32_t i = first; i <= last; ++i) {
        ObjectManager::Get().ReleaseEdge((*ptr)[i]);
    }
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
        if (it->is_number_float() && it->get<float>() == value) {
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
        if (it->is_string() && JCStrEqualsCI(it->get<std::string>(), value)) {
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
            ObjectManager::Get().ReleaseEdge(*it);
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
    // OG ordering: type rank first, value inside the rank, strings case-blind.
    // sort only moves elements around, never destroys — ref counts stay put
    std::sort(ptr->begin(), ptr->end(), JCItemLessOG);
    return obj;
}

Handle JArray_Unique(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return obj;
    JArray_Sort(nullptr, obj);
    // manual unique — std::unique moves over dupes and would trash ref markers
    // without releasing their edges. move survivors out, release whats left
    json deduped = json::array();
    for (auto& elem : *ptr) {
        if (deduped.empty() || !JCItemEquals(deduped.back(), elem)) {
            deduped.push_back(std::move(elem)); // marker rides along, count unchanged
        }
    }
    ObjectManager::Get().ReleaseAllEdges(*ptr); // dupes left behind
    *ptr = std::move(deduped);
    return obj;
}

Handle JArray_Reverse(RE::StaticFunctionTag*, Handle obj) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr || !ptr->is_array()) return obj;
    std::reverse(ptr->begin(), ptr->end());
    return obj;
}

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