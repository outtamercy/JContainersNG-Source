#pragma once
#include <SKSE/SKSE.h>
#include <string>
#include <format>
#include <optional>
#include <mutex>
#include <unordered_map>
#include <cstdint>

namespace FormSerializer {

    inline bool IsFormString(const std::string& str) {
        return str.starts_with("__formData|");
    }

    inline std::string EncodeForm(RE::TESForm* form) {
        if (!form) return "";

        auto formID = form->GetFormID();
        auto file = form->GetFile(0);

        std::string pluginName;
        std::uint32_t localID = formID;

        if (!file || file->fileName[0] == '\0') {
            pluginName = "";
        }
        else {
            pluginName = file->fileName;
            // ESL uses 12-bit local ID, everything else uses 24-bit
            if ((formID & 0xFF000000) == 0xFE000000) {
                localID = formID & 0xFFF;
            }
            else {
                localID = formID & 0xFFFFFF;
            }
        }

        return std::format("__formData|{}|{:08X}", pluginName, localID);
    }

    struct PluginIndexCache {
        std::optional<std::uint8_t> mod;
        std::optional<std::uint16_t> light;
    };

    inline PluginIndexCache& GetCachedPluginIndex(const std::string& name) {
        static std::unordered_map<std::string, PluginIndexCache> cache;
        static std::mutex cacheMutex;

        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it = cache.find(name);
        if (it != cache.end()) return it->second;

        PluginIndexCache idx;
        if (auto dh = RE::TESDataHandler::GetSingleton()) {
            if (auto modIdx = dh->GetLoadedModIndex(name); modIdx.has_value()) {
                idx.mod = modIdx.value();
            }
            else if (auto lightIdx = dh->GetLoadedLightModIndex(name); lightIdx.has_value()) {
                idx.light = lightIdx.value();
            }
        }
        auto [inserted, _] = cache.emplace(name, idx);
        return inserted->second;
    }

    inline RE::TESForm* DecodeForm(const std::string& formString) {
        if (!IsFormString(formString)) {
            SKSE::log::info("JContainersNG: DecodeForm rejected: '{}'", formString);
            return nullptr;
        }

        auto firstPipe = formString.find('|');
        auto secondPipe = formString.find('|', firstPipe + 1);
        if (firstPipe == std::string::npos || secondPipe == std::string::npos) {
            SKSE::log::info("JContainersNG: DecodeForm malformed: '{}'", formString);
            return nullptr;
        }

        std::string pluginName = formString.substr(firstPipe + 1, secondPipe - firstPipe - 1);
        std::string localIDStr = formString.substr(secondPipe + 1);

        SKSE::log::info("JContainersNG: DecodeForm parsing plugin='{}' localID='{}'", pluginName, localIDStr);

        try {
            std::uint32_t localID = std::stoul(localIDStr, nullptr, 16);
            auto dataHandler = RE::TESDataHandler::GetSingleton();
            if (!dataHandler) {
                SKSE::log::info("JContainersNG: DecodeForm no data handler");
                return nullptr;
            }

            if (pluginName.empty()) {
                SKSE::log::info("JContainersNG: DecodeForm empty plugin, fallback LookupByID({:08X})", localID);
                auto* form = RE::TESForm::LookupByID(localID);
                SKSE::log::info("JContainersNG: DecodeForm fallback result={}", form ? "FOUND" : "NULL");
                return form;
            }

            // Strategy 1: CommonLib LookupForm (should work for most)
            auto* form = dataHandler->LookupForm(localID, pluginName);
            if (form) {
                SKSE::log::info("JContainersNG: DecodeForm LookupForm SUCCESS plugin='{}' localID={:08X}", pluginName, localID);
                return form;
            }
            SKSE::log::info("JContainersNG: DecodeForm LookupForm FAILED plugin='{}' localID={:08X}", pluginName, localID);

            // Strategy 2: cached runtime ID from load order
            std::uint32_t runtimeID = 0;
            bool found = false;

            auto& cached = GetCachedPluginIndex(pluginName);
            if (cached.mod.has_value()) {
                runtimeID = (static_cast<std::uint32_t>(cached.mod.value()) << 24) | (localID & 0xFFFFFF);
                found = true;
                SKSE::log::info("JContainersNG: DecodeForm cached modIdx={:02X} runtimeID={:08X}", cached.mod.value(), runtimeID);
            }
            else if (cached.light.has_value()) {
                runtimeID = 0xFE000000 |
                    (static_cast<std::uint32_t>(cached.light.value()) << 12) |
                    (localID & 0xFFF);
                found = true;
                SKSE::log::info("JContainersNG: DecodeForm cached lightIdx={:03X} runtimeID={:08X}", cached.light.value(), runtimeID);
            }

            if (found) {
                auto* form2 = RE::TESForm::LookupByID(runtimeID);
                SKSE::log::info("JContainersNG: DecodeForm manual result={}", form2 ? "FOUND" : "NULL");
                return form2;
            }

            // Strategy 3: Raw local ID (last resort, usually wrong for modded forms)
            SKSE::log::info("JContainersNG: DecodeForm plugin '{}' not loaded, raw fallback LookupByID({:08X})", pluginName, localID);
            auto* form3 = RE::TESForm::LookupByID(localID);
            SKSE::log::info("JContainersNG: DecodeForm raw fallback result={}", form3 ? "FOUND" : "NULL");
            return form3;
        }
        catch (const std::exception& e) {
            SKSE::log::error("JContainersNG: DecodeForm exception: {}", e.what());
            return nullptr;
        }
    }
} // namespace FormSerializer