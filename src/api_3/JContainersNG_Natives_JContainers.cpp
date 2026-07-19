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
    return 2;
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
