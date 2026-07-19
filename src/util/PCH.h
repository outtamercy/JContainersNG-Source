#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
// Lua backend for JLua natives
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

using namespace std::literals;