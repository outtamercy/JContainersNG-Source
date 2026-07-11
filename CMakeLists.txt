cmake_minimum_required(VERSION 3.21)
project(jcontainersng VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CommonLibSSE CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)

set(SOURCES
    src/plugin.cpp
    src/JContainersNG_Natives.cpp
    src/Serialization.cpp
)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${SOURCES})

# Force the linker to output it as JContainers64.dll
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "JContainers64")

target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)
target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    nlohmann_json::nlohmann_json
)

set(OUTPUT_FOLDER "H:/Mayhem's Madness AE/mods/JContainersNG")
set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DLL_FOLDER}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> ${DLL_FOLDER}/JContainers64.dll
    VERBATIM
)