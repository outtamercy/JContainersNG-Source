//JContainersNG_Natives_JLua.cpp
#include "JContainersNG_Natives.hpp"

float JLua_EvalLuaFlt(RE::StaticFunctionTag*, std::string luaCode, Handle transport, float defaultVal, bool minimizeLifetime) {
    float result = defaultVal;
    bool ok = JContainersNG::Lua::EvaluateLuaExpression(luaCode, transport, [&](lua_State* L) {
        if (lua_isnumber(L, -1)) {
            result = static_cast<float>(lua_tonumber(L, -1));
        }
        });
    if (minimizeLifetime) {
        JValue_ZeroLifetime(nullptr, transport);
    }
    return ok ? result : defaultVal;
}

int32_t JLua_EvalLuaInt(RE::StaticFunctionTag*, std::string luaCode, Handle transport, int32_t defaultVal, bool minimizeLifetime) {
    int32_t result = defaultVal;
    bool ok = JContainersNG::Lua::EvaluateLuaExpression(luaCode, transport, [&](lua_State* L) {
        if (lua_isnumber(L, -1)) {
            result = static_cast<int32_t>(lua_tonumber(L, -1));
        }
        });
    if (minimizeLifetime) {
        JValue_ZeroLifetime(nullptr, transport);
    }
    return ok ? result : defaultVal;
}

std::string JLua_EvalLuaStr(RE::StaticFunctionTag*, std::string luaCode, Handle transport, std::string defaultVal, bool minimizeLifetime) {
    std::string result = defaultVal;
    bool ok = JContainersNG::Lua::EvaluateLuaExpression(luaCode, transport, [&](lua_State* L) {
        if (lua_isstring(L, -1)) {
            result = lua_tostring(L, -1);
        }
        });
    if (minimizeLifetime) {
        JValue_ZeroLifetime(nullptr, transport);
    }
    return ok ? result : defaultVal;
}

Handle JLua_EvalLuaObj(RE::StaticFunctionTag*, std::string luaCode, Handle transport, Handle defaultVal, bool minimizeLifetime) {
    Handle result = defaultVal;
    bool ok = JContainersNG::Lua::EvaluateLuaExpression(luaCode, transport, [&](lua_State* L) {
        int t = lua_type(L, -1);
        if (t == LUA_TNUMBER) {
            result = static_cast<Handle>(lua_tonumber(L, -1));
        }
        else if (t == LUA_TTABLE || t == LUA_TSTRING || t == LUA_TBOOLEAN || t == LUA_TNIL) {
            json j = JContainersNG::Lua::PullJsonFromLua(L, -1);
            result = ObjectManager::Get().RegisterObject(std::move(j));
        }
        });
    if (minimizeLifetime) {
        JValue_ZeroLifetime(nullptr, transport);
    }
    return ok ? result : defaultVal;
}

RE::TESForm* JLua_EvalLuaForm(RE::StaticFunctionTag*, std::string luaCode, Handle transport, RE::TESForm* defaultVal, bool minimizeLifetime) {
    RE::TESForm* result = defaultVal;
    bool ok = JContainersNG::Lua::EvaluateLuaExpression(luaCode, transport, [&](lua_State* L) {
        if (lua_isstring(L, -1)) {
            std::string str = lua_tostring(L, -1);
            if (FormSerializer::IsFormString(str)) {
                result = FormSerializer::DecodeForm(str);
            }
        }
        else if (lua_isnumber(L, -1)) {
            auto formID = static_cast<RE::FormID>(lua_tonumber(L, -1));
            result = RE::TESForm::LookupByID(formID);
        }
        });
    if (minimizeLifetime) {
        JValue_ZeroLifetime(nullptr, transport);
    }
    return ok ? result : defaultVal;
}

Handle JLua_SetStr(RE::StaticFunctionTag*, std::string key, std::string value, Handle transport) {
    if (transport == 0) transport = ObjectManager::Get().CreateObject();
    auto ptr = ObjectManager::Get().GetObject(transport);
    if (!ptr || !ptr->is_object()) return transport;
    (*ptr)[key] = value;
    return transport;
}

Handle JLua_SetFlt(RE::StaticFunctionTag*, std::string key, float value, Handle transport) {
    if (transport == 0) transport = ObjectManager::Get().CreateObject();
    auto ptr = ObjectManager::Get().GetObject(transport);
    if (!ptr || !ptr->is_object()) return transport;
    (*ptr)[key] = value;
    return transport;
}

Handle JLua_SetInt(RE::StaticFunctionTag*, std::string key, int32_t value, Handle transport) {
    if (transport == 0) transport = ObjectManager::Get().CreateObject();
    auto ptr = ObjectManager::Get().GetObject(transport);
    if (!ptr || !ptr->is_object()) return transport;
    (*ptr)[key] = value;
    return transport;
}

Handle JLua_SetForm(RE::StaticFunctionTag*, std::string key, RE::TESForm* value, Handle transport) {
    if (transport == 0) transport = ObjectManager::Get().CreateObject();
    auto ptr = ObjectManager::Get().GetObject(transport);
    if (!ptr || !ptr->is_object()) return transport;

    // ReleaseEdge BEFORE the write — the slot might hold a ref, and plain
    // assignment (or worse, erase) leaks its Internal ref. also: OG stores a
    // null item for None forms rather than erasing the key, so we match that
    ObjectManager::Get().ReleaseEdge((*ptr)[key]);
    (*ptr)[key] = value ? json(FormSerializer::EncodeForm(value)) : json(nullptr);
    return transport;
}

Handle JLua_SetObj(RE::StaticFunctionTag*, std::string key, Handle value, Handle transport) {
    if (transport == 0) transport = ObjectManager::Get().CreateObject();
    auto ptr = ObjectManager::Get().GetObject(transport);
    if (!ptr || !ptr->is_object()) return transport;

    // one call covers both branches: nonzero value releases the old edge and
    // retains the new one, 0 nulls the slot and releases the old edge.
    // this replaces both the raw MakeRef (no retain, dangling handle bait)
    // and the erase() call (leaked the old edge every single time)
    ObjectManager::Get().EmbedEdge((*ptr)[key], value);
    return transport;
}

// JValue.evalLua* backward-compat wrappers
float JValue_EvalLuaFlt(RE::StaticFunctionTag*, Handle object, std::string luaCode, float defaultVal) {
    return JLua_EvalLuaFlt(nullptr, luaCode, object, defaultVal, false);
}

int32_t JValue_EvalLuaInt(RE::StaticFunctionTag*, Handle object, std::string luaCode, int32_t defaultVal) {
    return JLua_EvalLuaInt(nullptr, luaCode, object, defaultVal, false);
}

std::string JValue_EvalLuaStr(RE::StaticFunctionTag*, Handle object, std::string luaCode, std::string defaultVal) {
    return JLua_EvalLuaStr(nullptr, luaCode, object, defaultVal, false);
}

Handle JValue_EvalLuaObj(RE::StaticFunctionTag*, Handle object, std::string luaCode, Handle defaultVal) {
    return JLua_EvalLuaObj(nullptr, luaCode, object, defaultVal, false);
}

RE::TESForm* JValue_EvalLuaForm(RE::StaticFunctionTag*, Handle object, std::string luaCode, RE::TESForm* defaultVal) {
    return JLua_EvalLuaForm(nullptr, luaCode, object, defaultVal, false);
}
