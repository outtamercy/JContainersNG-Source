// JContainersNG_Lua.cpp

#include <SKSE/SKSE.h>
#include "ObjectManager.hpp"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

using json = nlohmann::json;
using Handle = ObjectManager::Handle;

namespace JContainersNG::Lua {

    // Just use standard C allocators — same CRT heap as SKSE, no drama
    static void* SkyrimLuaAlloc(void* ud, void* ptr, size_t osize, size_t nsize) {
        (void)ud; (void)osize;
        if (nsize == 0) {
            free(ptr);
            return nullptr;
        }
        if (!ptr) {
            return malloc(nsize);
        }
        return realloc(ptr, nsize);
    }

    // Shove a json tree onto the Lua stack recursively
    static void PushJsonToLua(lua_State* L, const json& j) {
        if (j.is_null()) {
            lua_pushnil(L);
        }
        else if (j.is_boolean()) {
            lua_pushboolean(L, j.get<bool>());
        }
        else if (j.is_number_integer()) {
            lua_pushinteger(L, j.get<lua_Integer>());
        }
        else if (j.is_number_float()) {
            lua_pushnumber(L, j.get<double>());
        }
        else if (j.is_string()) {
            std::string str = j.get<std::string>();
            lua_pushlstring(L, str.c_str(), str.size());
        }
        else if (j.is_array()) {
            lua_newtable(L);
            int index = 1; // Lua arrays start at 1, because why not
            for (const auto& element : j) {
                PushJsonToLua(L, element);
                lua_rawseti(L, -2, index++);
            }
        }
        else if (j.is_object()) {
            lua_newtable(L);
            for (auto it = j.begin(); it != j.end(); ++it) {
                lua_pushlstring(L, it.key().c_str(), it.key().size());
                PushJsonToLua(L, it.value());
                lua_rawset(L, -3);
            }
        }
        else {
            lua_pushnil(L);
        }
    }

    // Pull a Lua value back into json. Handles scalars and tables.
    // Form pointers and JC refs don't round-trip here — they'd need extra glue.
    json PullJsonFromLua(lua_State* L, int idx) {
        int t = lua_type(L, idx);
        switch (t) {
        case LUA_TNIL:       return json(nullptr);
        case LUA_TBOOLEAN:   return json(lua_toboolean(L, idx) != 0);
        case LUA_TNUMBER:
            if (lua_isinteger(L, idx)) return json(lua_tointeger(L, idx));
            return json(lua_tonumber(L, idx));
        case LUA_TSTRING:    return json(lua_tostring(L, idx));
        case LUA_TTABLE: {
            // Figure out if it's an array (consecutive int keys from 1) or an object
            bool isArray = true;
            int expectedKey = 1;
            int n = 0;
            lua_pushnil(L);
            while (lua_next(L, idx < 0 ? idx - 1 : idx)) {
                if (lua_type(L, -2) != LUA_TNUMBER || lua_tointeger(L, -2) != expectedKey) {
                    isArray = false;
                    lua_pop(L, 2);
                    break;
                }
                expectedKey++;
                n++;
                lua_pop(L, 1);
            }

            if (isArray) {
                json arr = json::array();
                for (int i = 1; i <= n; ++i) {
                    lua_rawgeti(L, idx, i);
                    arr.push_back(PullJsonFromLua(L, -1));
                    lua_pop(L, 1);
                }
                return arr;
            }
            else {
                json obj = json::object();
                lua_pushnil(L);
                while (lua_next(L, idx < 0 ? idx - 1 : idx)) {
                    std::string key = lua_tostring(L, -2);
                    obj[key] = PullJsonFromLua(L, -1);
                    lua_pop(L, 1);
                }
                return obj;
            }
        }
        default: return json(nullptr);
        }
    }

    // Core pipeline. Fresh Lua state per eval, maps transport JMap to global 'args'.
    // extractResult lambda reads the return value off the stack.
    bool EvaluateLuaExpression(const std::string& luaCode, Handle transportHandle, std::function<void(lua_State*)> extractResult) {
        lua_State* L = lua_newstate(SkyrimLuaAlloc, nullptr);
        if (!L) {
            SKSE::log::error("JContainersNG Lua: failed to create state");
            return false;
        }

        luaL_openlibs(L);

        // OG sandboxed eval scripts for a reason: openlibs hands out os.execute and
        // package/loadlib, i.e. arbitrary dll loading from papyrus-visible lua.
        // also unpack moved to table.unpack in 5.4 — alias it so 5.1-era (LuaJIT)
        // scripts dont die. jc subset = the pure-table helpers from OG's jc lib,
        // covers the most-copied doc snippets like jc.count(jobject, jc.less(6)).
        // filter/accumulateValues need live JC objects, cant work on plain tables
        // OG sandboxed eval scripts for a reason: openlibs hands out os.execute and
        // package/loadlib, i.e. arbitrary dll loading from papyrus-visible lua.
        // we keep a private loadfile ref for jrequire BEFORE killing the globals.
        // also unpack moved to table.unpack in 5.4 — alias it so 5.1-era (LuaJIT)
        // scripts dont die. jc subset = the pure-table helpers from OG's jc lib.
        static const char* kPrelude = R"(
            local _loadfile = loadfile  -- jrequire's private key, globals get axed below

            os.execute = nil
            os.exit = nil
            os.remove = nil
            os.rename = nil
            package = nil
            require = nil
            dofile = nil
            loadfile = nil
            loadlib = nil
            unpack = table.unpack

            jc = {}
            function jc.apply(collection, predicate)
                for _, v in pairs(collection) do
                    if predicate(v) then return end
                end
            end
            function jc.count(collection, predicate)
                local n = 0
                for _, v in pairs(collection) do
                    if predicate(v) then n = n + 1 end
                end
                return n
            end
            function jc.find(collection, predicate)
                for k, v in pairs(collection) do
                    if predicate(v) then return k end
                end
                return nil
            end
            function jc.less(than) return function(x) return x < than end end
            function jc.greater(than) return function(x) return x > than end end

            -- OG module system, fresh-state edition: no cache (state dies per eval),
            -- mods like CSM load once per menu open so re-reading the file is free.
            -- 'jc' short-circuits to our helpers since real mods cargo-cult that line
            function jrequire(name)
                if name == 'jc' then return jc end
                local rel = name:gsub('%.', '/')
                local chunk, err = _loadfile(JC_LUA_PATH .. rel .. '/init.lua')
                if not chunk then
                    chunk, err = _loadfile(JC_LUA_PATH .. rel .. '.lua')
                end
                if not chunk then error("jrequire: cant load '" .. name .. "': " .. tostring(err)) end
                return chunk()
            end

            -- JC object constructors, plain-table flavor. our PullJsonFromLua turns
            -- any returned table into a real JObject, so lua code never needs the
            -- FFI handle dance OG did. metatable marks intent + future-proofs us
            local function mkcontainer(kind)
                return setmetatable({}, { __jc_kind = kind })
            end

            JMap = {
                object = function() return mkcontainer('map') end,
            }
            JArray = {
                object = function() return mkcontainer('array') end,
            }
            JValue = {}
        )";
        luaL_dostring(L, kPrelude); // our own string, cant fail — no check needed

        // where jrequire hunts modules. matches the user:/ root + lua/ subdir,
        // same layout OG ships (JCData/lua/<mod>/init.lua)
        static const std::string kLuaModulePath = "Data/SKSE/Plugins/JCData/lua/";
        lua_pushlstring(L, kLuaModulePath.c_str(), kLuaModulePath.size());
        lua_setglobal(L, "JC_LUA_PATH");

        // transport handle -> args table. nothing pushed this before, so args
        // (and jobject via the alias below) was always nil — every evalLua
        // script that read its transport got nothing. invalid/zero handle =
        // empty table, same as passing OG an empty transport map
        {
            auto transportPtr = (transportHandle != 0)
                ? ObjectManager::Get().GetObject(transportHandle)
                : nullptr;
            if (transportPtr) {
                PushJsonToLua(L, *transportPtr);
            }
            else {
                lua_newtable(L);
            }
            lua_setglobal(L, "args");
        }

        // OG binds BOTH names (init.lua: 'local args = ...; local jobject = args;')
        // and every documented evalLua example uses jobject. same table, two names
        lua_getglobal(L, "args");
        lua_setglobal(L, "jobject");

        bool ok = false;
        if (luaL_dostring(L, luaCode.c_str()) == LUA_OK) {
            if (extractResult) {
                extractResult(L);
            }
            ok = true;
        }
        else {
            const char* err = lua_tostring(L, -1);
            SKSE::log::error("JContainersNG Lua eval error: {}", err ? err : "unknown");
        }

        lua_close(L);
        return ok;
    }

} // namespace JContainersNG::Lua