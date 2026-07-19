cmake_minimum_required(VERSION 3.21)
project(lua_static C)

# Grab all the C source files from the skynet55 fork
file(GLOB LUA_SOURCES *.c)

# Don't compile the standalone interpreter or compiler binaries
list(REMOVE_ITEM LUA_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/lua.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/luac.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/onelua.c"
)
add_library(lua_static STATIC ${LUA_SOURCES})

target_include_directories(lua_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# MSVC doesn't love some of the C89-style stuff in Lua, so quiet the noise
if(MSVC)
    target_compile_options(lua_static PRIVATE /W3 /wd4244 /wd4310 /wd4702)
    target_compile_definitions(lua_static PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()