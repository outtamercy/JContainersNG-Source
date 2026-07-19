cmake_minimum_required(VERSION 3.21)

# Plugin name has to match the runtime — mods probe by declared name
# (AP's gate: GetPluginVersion("JContainers64"/"JContainersGOG"/"JContainersVR"), passes on ANY match)
# "JContainers64" covers SE + GOG since GOG runs the same scripts, but VR mods
# may specifically want "JContainersVR", so we name it right per-build
if(VCPKG_TARGET_TRIPLET MATCHES "sksevr")
    project(JContainersVR VERSION 2.0.0 LANGUAGES CXX)
else()
    project(JContainers64 VERSION 2.0.0 LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CommonLibSSE CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)

set(SOURCES
    src/skse/plugin.cpp
    src/skse/Serialization.cpp
    src/reflection/JContainersNG_Lua.cpp
    src/reflection/JContainersNG_Natives_JLua.cpp
    src/util/JContainersNG_Internal.cpp
    src/api_3/JContainersNG_Natives_JContainers.cpp
    src/api_3/JContainersNG_Natives_JValue.cpp
    src/api_3/JContainersNG_Natives_JAtomic.cpp
    src/collections/JContainersNG_Natives_JMap.cpp
    src/collections/JContainersNG_Natives_JArray.cpp
    src/collections/JContainersNG_Natives_JFormMap.cpp
    src/collections/JContainersNG_Natives_JIntMap.cpp
    src/domains/JContainersNG_Natives_JDB.cpp
    src/domains/JContainersNG_Natives_JFormDB.cpp
    src/forms/JContainersNG_Natives_JString.cpp
)

set(HEADERS
    src/object/ObjectManager.hpp
    src/object/JContainersNG_AutoreleaseQueue.hpp
    src/api_3/JContainersNG_Natives.hpp
    src/forms/FormSerializer.hpp
    src/util/JContainersNG_Internal.hpp
    src/util/JsonSerializer.hpp
    src/util/FixStaticFunctionTag.hpp
)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${SOURCES})

# OUTPUT_NAME defaults to the target name (= project name), so the dll
# comes out as JContainers64.dll / JContainersVR.dll on its own

target_precompile_headers(${PROJECT_NAME} PRIVATE src/util/PCH.h)

# every subfolder on the include path so no #include line has to change
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/object
    src/collections
    src/domains
    src/forms
    src/api_3
    src/reflection
    src/skse
    src/util
)

# Lua backend for JLua eval natives
add_subdirectory(third_party/lua)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    nlohmann_json::nlohmann_json
    lua_static
)

set(OUTPUT_FOLDER "H:/Mayhem's Madness AE/mods/JContainersNG")
set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DLL_FOLDER}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> ${DLL_FOLDER}/${PROJECT_NAME}.dll
    VERBATIM
)