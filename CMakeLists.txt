cmake_minimum_required(VERSION 3.21)

# Plugin name has to match the runtime
if(VCPKG_TARGET_TRIPLET MATCHES "sksevr")
    project(JContainersVR VERSION 2.0.0 LANGUAGES CXX)
else()
    project(JContainers64 VERSION 2.0.0 LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Disable CommonLibSSE internal tests before pulling it
# ---------------------------------------------------------------------------
set(BUILD_TESTS OFF CACHE BOOL "Disable CommonLibSSE tests" FORCE)
set(ENABLE_SKYRIM_TESTS OFF CACHE BOOL "Disable Skyrim tests" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Disable CTest" FORCE)

# ---------------------------------------------------------------------------
# Dependency Management (FetchContent for alandtse's CommonLibSSE-NG)
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    CommonLibSSE
    GIT_REPOSITORY https://github.com/alandtse/CommonLibSSE-NG.git
    GIT_TAG        ng
)

FetchContent_MakeAvailable(CommonLibSSE)

# Remaining dependencies managed via vcpkg
find_package(nlohmann_json CONFIG REQUIRED)

# ---------------------------------------------------------------------------
# Source Declarations (Auto-Discover)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# Create shared library target directly
add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_precompile_headers(${PROJECT_NAME} PRIVATE src/util/PCH.h)

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

# ---------------------------------------------------------------------------
# Dependencies & Linking
# ---------------------------------------------------------------------------
add_subdirectory(third_party/lua)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    CommonLibSSE::CommonLibSSE
    nlohmann_json::nlohmann_json
    lua_static
)

# ---------------------------------------------------------------------------
# Post-Build Deployment
# ---------------------------------------------------------------------------
set(JCNG_DEPLOY_FOLDER "" CACHE PATH "Optional: mod folder to copy the dll into after build")

if(JCNG_DEPLOY_FOLDER)
    set(DLL_FOLDER "${JCNG_DEPLOY_FOLDER}/SKSE/Plugins")
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DLL_FOLDER}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> ${DLL_FOLDER}/${PROJECT_NAME}.dll
        VERBATIM
    )
endif()