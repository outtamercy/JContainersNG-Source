//JContainersNG_Natives_JContainers.cpp
#include "JContainersNG_Natives.hpp"
#include <shlobj.h>
#include <combaseapi.h>
#include <shlobj.h>
#include <filesystem>

std::string ResolveJCPath(const std::string& inputPath);

bool JContainers_IsInstalled(RE::StaticFunctionTag*) {
    SKSE::log::info("JContainersNG: __isInstalled CALLED, returning TRUE");
    return true;
}

int32_t JContainers_APIVersion(RE::StaticFunctionTag*) {
    return 4;
}

int32_t JContainers_FeatureVersion(RE::StaticFunctionTag*) {
    return 3;
}

int32_t JContainers_MinorVersion(RE::StaticFunctionTag*) {
    return 2;
}

int32_t JContainers_PatchVersion(RE::StaticFunctionTag*) {
    return 0;
}

int32_t JContainers_VersionInt(RE::StaticFunctionTag*) {
    return JContainers_APIVersion(nullptr) * 1000000
        + JContainers_FeatureVersion(nullptr) * 10000
        + JContainers_MinorVersion(nullptr) * 100
        + JContainers_PatchVersion(nullptr);
}

std::string JContainers_VersionString(RE::StaticFunctionTag*) {
    return "4.3.2.0";
}

bool JContainers_VersionAtLeast(RE::StaticFunctionTag*, int32_t api, int32_t feature, int32_t minor, int32_t patch) {
    // OG receives UInt32 values. Keep that conversion even though Papyrus
    // exposes its only integer type as signed.
    const uint64_t requested = static_cast<uint64_t>(static_cast<uint32_t>(api)) * 1000000u
        + static_cast<uint64_t>(static_cast<uint32_t>(feature)) * 10000u
        + static_cast<uint64_t>(static_cast<uint32_t>(minor)) * 100u
        + static_cast<uint32_t>(patch);
    return static_cast<uint64_t>(JContainers_VersionInt(nullptr)) >= requested;
}

bool JContainers_FileExistsAtPath(RE::StaticFunctionTag*, std::string path) {
    // ResolveJCPath was supposed to be wired here already — raw "user:/..."
    // always fails std::filesystem::exists
    return std::filesystem::exists(ResolveJCPath(path));
}

std::vector<std::string> JContainers_ContentsOfDirectoryAtPath(RE::StaticFunctionTag*, std::string directoryPath, std::string extension) {
    directoryPath = ResolveJCPath(directoryPath);
    SKSE::log::info("JContainersNG: ContentsOfDirectoryAtPath resolved path: '{}'", directoryPath);

    std::vector<std::string> result;
    try {
        if (!std::filesystem::exists(directoryPath)) return result;

        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            // two OG-faithfulness fixes: extension compared WITH the dot, and
            // no is_regular_file filter — OG lists subdirectories too
            if (!extension.empty() && entry.path().extension().string() != extension) continue;
            result.push_back(entry.path().string());
        }
    }
    catch (...) {}
    return result;
}

void JContainers_RemoveFileAtPath(RE::StaticFunctionTag*, std::string path) {
    try {
        std::filesystem::remove_all(ResolveJCPath(path));
    }
    catch (...) {}
}

std::string JContainers_UserDirectory(RE::StaticFunctionTag*) {
    wchar_t* wpath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &wpath))) {
        std::filesystem::path docPath(wpath);
        CoTaskMemFree(wpath);

        auto jcUserPath = docPath / "My Games" / "Skyrim Special Edition" / "JCUser" / "";
        std::filesystem::create_directories(jcUserPath);
        return jcUserPath.string(); // Safely converts to UTF-8/ANSI string
    }
    return "Data/SKSE/Plugins/JContainersNG/";
}
