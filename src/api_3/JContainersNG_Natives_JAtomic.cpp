//JContainersNG_JAtomic.cpp
#include "JContainersNG_Natives.hpp"

int32_t JAtomic_fetchAddInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, int32_t initialValue, bool createMissingKeys, int32_t onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    if (target->is_null()) {
        // OG: null slot stores func(initialValue, value) but returns the type
        // default — previousVal never gets touched on the null branch
        *target = initialValue + value;
        return 0;
    }
    // OG is variant-strict — int ops on a float slot error out, no conversion
    if (!target->is_number_integer()) return onErrorReturn;
    int32_t prev = target->get<int32_t>();
    *target = prev + value;
    return prev;
}

float JAtomic_fetchAddFlt(RE::StaticFunctionTag*, Handle obj, std::string path, float value, float initialValue, bool createMissingKeys, float onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    if (target->is_null()) {
        *target = initialValue + value;
        return 0.0f;
    }
    if (!target->is_number_float()) return onErrorReturn;
    float prev = target->get<float>();
    *target = prev + value;
    return prev;
}

int32_t JAtomic_fetchMultInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, int32_t initialValue, bool createMissingKeys, int32_t onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    if (target->is_null()) {
        *target = initialValue * value;
        return 0;
    }
    if (!target->is_number_integer()) return onErrorReturn;
    int32_t prev = target->get<int32_t>();
    *target = prev * value;
    return prev;
}

float JAtomic_fetchMultFlt(RE::StaticFunctionTag*, Handle obj, std::string path, float value, float initialValue, bool createMissingKeys, float onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    if (target->is_null()) {
        *target = initialValue * value;
        return 0.0f;
    }
    if (!target->is_number_float()) return onErrorReturn;
    float prev = target->get<float>();
    *target = prev * value;
    return prev;
}

int32_t JAtomic_fetchModInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, int32_t initialValue, bool createMissingKeys, int32_t onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;
    // deliberate divergence: OG eats a div-by-zero CTD here, we bail clean
    if (value == 0) return onErrorReturn;

    if (target->is_null()) {
        *target = initialValue % value;
        return 0;
    }
    if (!target->is_number_integer()) return onErrorReturn;
    int32_t prev = target->get<int32_t>();
    *target = prev % value;
    return prev;
}

int32_t JAtomic_fetchDivInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, int32_t initialValue, bool createMissingKeys, int32_t onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;
    if (value == 0) return onErrorReturn; // same CTD guard as fetchModInt

    if (target->is_null()) {
        *target = initialValue / value;
        return 0;
    }
    if (!target->is_number_integer()) return onErrorReturn;
    int32_t prev = target->get<int32_t>();
    *target = prev / value;
    return prev;
}

float JAtomic_fetchDivFlt(RE::StaticFunctionTag*, Handle obj, std::string path, float value, float initialValue, bool createMissingKeys, float onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;
    // float div-by-zero is well-defined (inf), no guard needed — OG parity

    if (target->is_null()) {
        *target = initialValue / value;
        return 0.0f;
    }
    if (!target->is_number_float()) return onErrorReturn;
    float prev = target->get<float>();
    *target = prev / value;
    return prev;
}

int32_t JAtomic_fetchAndInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, int32_t initialValue, bool createMissingKeys, int32_t onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    if (target->is_null()) {
        *target = initialValue & value;
        return 0;
    }
    if (!target->is_number_integer()) return onErrorReturn;
    int32_t prev = target->get<int32_t>();
    *target = prev & value;
    return prev;
}

int32_t JAtomic_fetchXorInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, int32_t initialValue, bool createMissingKeys, int32_t onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    if (target->is_null()) {
        *target = initialValue ^ value;
        return 0;
    }
    if (!target->is_number_integer()) return onErrorReturn;
    int32_t prev = target->get<int32_t>();
    *target = prev ^ value;
    return prev;
}

int32_t JAtomic_fetchOrInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, int32_t initialValue, bool createMissingKeys, int32_t onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    if (target->is_null()) {
        *target = initialValue | value;
        return 0;
    }
    if (!target->is_number_integer()) return onErrorReturn;
    int32_t prev = target->get<int32_t>();
    *target = prev | value;
    return prev;
}

int32_t JAtomic_exchangeInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t value, bool createMissingKeys, int32_t onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    if (target->is_null()) {
        // OG: null slot takes the value and returns type default — returning
        // onErrorReturn here spooks mods that use it as an error sentinel
        *target = value;
        return 0;
    }
    if (!target->is_number_integer()) return onErrorReturn;
    int32_t prev = target->get<int32_t>();
    *target = value;
    return prev;
}

float JAtomic_exchangeFlt(RE::StaticFunctionTag*, Handle obj, std::string path, float value, bool createMissingKeys, float onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    if (target->is_null()) {
        *target = value;
        return 0.0f;
    }
    if (!target->is_number_float()) return onErrorReturn;
    float prev = target->get<float>();
    *target = value;
    return prev;
}

std::string JAtomic_exchangeStr(RE::StaticFunctionTag*, Handle obj, std::string path, std::string value, bool createMissingKeys, std::string onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    if (target->is_null()) {
        *target = value;
        return "";
    }
    if (!target->is_string()) return onErrorReturn;
    std::string prev = target->get<std::string>();
    *target = value;
    return prev;
}

RE::TESForm* JAtomic_exchangeForm(RE::StaticFunctionTag*, Handle obj, std::string path, RE::TESForm* value, bool createMissingKeys, RE::TESForm* onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    if (target->is_null()) {
        *target = value ? json(FormSerializer::EncodeForm(value)) : json(nullptr);
        return nullptr;
    }
    if (!target->is_string()) return onErrorReturn;
    std::string str = target->get<std::string>();
    if (!FormSerializer::IsFormString(str)) return onErrorReturn;
    RE::TESForm* prev = FormSerializer::DecodeForm(str);
    *target = value ? json(FormSerializer::EncodeForm(value)) : json(nullptr);
    return prev;
}

Handle JAtomic_exchangeObj(RE::StaticFunctionTag*, Handle obj, std::string path, Handle value, bool createMissingKeys, Handle onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    if (target->is_null()) {
        // null slot: takes the value, returns type default. EmbedEdge handles
        // the retain — raw MakeRef is how dangling handles happen
        ObjectManager::Get().EmbedEdge(*target, value);
        return 0;
    }

    Handle prev;
    Handle h;
    if (ObjectManager::IsRef(*target, &h)) {
        prev = h;
    }
    else if (target->is_object() || target->is_array()) {
        // inline json gets promoted to a real object first, same as solveObj —
        // and promotion goes through EmbedEdge too
        prev = ObjectManager::Get().RegisterObject(std::move(*target));
        ObjectManager::Get().EmbedEdge(*target, prev);
    }
    else {
        return onErrorReturn;
    }

    // releases whatever edge the slot held (old ref or the promotion ref we
    // just made), retains the new one. 0 nulls the slot
    ObjectManager::Get().EmbedEdge(*target, value);
    return prev;
}

int32_t JAtomic_compareExchangeInt(RE::StaticFunctionTag*, Handle obj, std::string path, int32_t desired, int32_t expected, bool createMissingKeys, int32_t onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    // OG compares the full item INCLUDING type — null or wrong-type slots
    // never match, never exchange, and return type default (onErrorReturn is
    // strictly for unresolvable paths)
    if (!target->is_number_integer()) return 0;
    int32_t prev = target->get<int32_t>();
    if (prev == expected) *target = desired;
    return prev;
}

float JAtomic_compareExchangeFlt(RE::StaticFunctionTag*, Handle obj, std::string path, float desired, float expected, bool createMissingKeys, float onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    if (!target->is_number_float()) return 0.0f;
    float prev = target->get<float>();
    if (prev == expected) *target = desired;
    return prev;
}

std::string JAtomic_compareExchangeStr(RE::StaticFunctionTag*, Handle obj, std::string path, std::string desired, std::string expected, bool createMissingKeys, std::string onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    if (!target->is_string()) return "";
    std::string prev = target->get<std::string>();
    // OG item equality is _stricmp for strings — CAS matches CI like everything else
    if (JCStrEqualsCI(prev, expected)) *target = desired;
    return prev;
}

RE::TESForm* JAtomic_compareExchangeForm(RE::StaticFunctionTag*, Handle obj, std::string path, RE::TESForm* desired, RE::TESForm* expected, bool createMissingKeys, RE::TESForm* onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    // non-form strings and wrong types alike: no match, no exchange, None back
    if (!target->is_string()) return nullptr;
    std::string str = target->get<std::string>();
    if (!FormSerializer::IsFormString(str)) return nullptr;
    RE::TESForm* prev = FormSerializer::DecodeForm(str);
    if (prev == expected) {
        *target = desired ? json(FormSerializer::EncodeForm(desired)) : json(nullptr);
    }
    return prev;
}

Handle JAtomic_compareExchangeObj(RE::StaticFunctionTag*, Handle obj, std::string path, Handle desired, Handle expected, bool createMissingKeys, Handle onErrorReturn) {
    auto ptr = ObjectManager::Get().GetObject(obj);
    if (!ptr) return onErrorReturn;
    auto* target = ResolvePath(*ptr, path, createMissingKeys);
    if (!target) return onErrorReturn;

    if (target->is_null()) {
        // best-effort OG read: empty slot vs 0 expected counts as a match and
        // the write goes through. anything else, no exchange
        if (expected == 0) {
            ObjectManager::Get().EmbedEdge(*target, desired);
        }
        return 0;
    }

    Handle prev;
    Handle h;
    if (ObjectManager::IsRef(*target, &h)) {
        prev = h;
    }
    else if (target->is_object() || target->is_array()) {
        // same promotion dance as exchangeObj, same edge rules
        prev = ObjectManager::Get().RegisterObject(std::move(*target));
        ObjectManager::Get().EmbedEdge(*target, prev);
    }
    else {
        return 0; // wrong type: no match, no exchange, type default
    }

    if (prev == expected) {
        // swap only fires on a match, but edge discipline still applies
        ObjectManager::Get().EmbedEdge(*target, desired);
    }
    return prev;
}