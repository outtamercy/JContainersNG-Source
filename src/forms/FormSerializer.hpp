// FormSerializer.hpp

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

    // canonical OG format: "__formData|ModName|0x%x" — lowercase, no padding,
    // 0x prefix. dynamic forms (0xFF prefix) keep the whole id with an empty
    // mod slot: "__formData||0xff000014". anything else breaks string-key
    // compatibility with OG-written files, and keys are where compat lives
    inline std::string EncodeForm(RE::TESForm* form) {
        if (!form) return "";

        auto formID = form->GetFormID();
        std::uint32_t u32 = formID;

        // dynamic form (0xFF top byte) — no owning mod, id goes out whole
        if ((u32 & 0xFF000000u) == 0xFF000000u) {
            return std::format("__formData||0x{:x}", u32);
        }

        auto file = form->GetFile(0);
        std::string pluginName;
        if (!file || file->fileName[0] == '\0') {
            // mod name unresolvable — OG bails (nullopt), we bail (empty)
            return "";
        }

        pluginName = file->fileName;
        // ESL uses 12-bit local ID, everything else 24-bit
        if ((u32 & 0xFF000000u) == 0xFE000000u) {
            u32 &= 0xFFFu;
        }
        else {
            u32 &= 0xFFFFFFu;
        }

        return std::format("__formData|{}|0x{:x}", pluginName, u32);
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

    // accepts canonical "0x1a" AND our old legacy "0000001A" format — old
    // co-saves and JSON files have the legacy one baked in forever
    inline RE::TESForm* DecodeForm(const std::string& formString) {
        if (!IsFormString(formString)) {
            return nullptr;
        }

        auto firstPipe = formString.find('|');
        auto secondPipe = formString.find('|', firstPipe + 1);
        if (firstPipe == std::string::npos || secondPipe == std::string::npos) {
            return nullptr;
        }

        std::string pluginName = formString.substr(firstPipe + 1, secondPipe - firstPipe - 1);
        std::string idStr = formString.substr(secondPipe + 1);

        try {
            // canonical has 0x -> base 0 sniffs it. legacy is bare hex -> force 16.
            // plain stoul(base 0) on legacy would try OCTAL and die on digits 8-9
            std::uint32_t parsed;
            if (idStr.starts_with("0x") || idStr.starts_with("0X")) {
                parsed = std::stoul(idStr, nullptr, 0);
            }
            else {
                parsed = std::stoul(idStr, nullptr, 16);
            }

            auto dataHandler = RE::TESDataHandler::GetSingleton();
            if (!dataHandler) {
                return nullptr;
            }

            // empty mod slot = dynamic form. OG always ORs the 0xFF top byte in;
            // legacy strings already have it baked, so mask-then-OR covers both
            if (pluginName.empty()) {
                std::uint32_t runtimeID = 0xFF000000u | (parsed & 0xFFFFFFu);
                // co-save remap, same as OG's skse::resolve_handle: if this string
                // came out of a save and the load order moved since, SKSE gives us
                // the new id. unresolvable = treat as dead (nullptr), NEVER look up
                // the raw id — 0xFF ids get reused, we'd hand back the wrong form
                if (auto serial = SKSE::GetSerializationInterface()) {
                    std::uint32_t resolved = 0;
                    if (serial->ResolveFormID(runtimeID, resolved)) {
                        return RE::TESForm::LookupByID(resolved);
                    }
                }
                return nullptr;
            }

            // mod bits on the incoming id mean nothing — OG ignores them, so do we
            std::uint32_t localID = parsed & 0xFFFFFFu;

            // Strategy 1: CommonLib LookupForm (should work for most)
            auto* form = dataHandler->LookupForm(localID, pluginName);
            if (form) {
                return form;
            }

            // Strategy 2: cached runtime ID from load order
            std::uint32_t runtimeID = 0;
            bool found = false;

            auto& cached = GetCachedPluginIndex(pluginName);
            if (cached.mod.has_value()) {
                runtimeID = (static_cast<std::uint32_t>(cached.mod.value()) << 24) | localID;
                found = true;
            }
            else if (cached.light.has_value()) {
                runtimeID = 0xFE000000u |
                    (static_cast<std::uint32_t>(cached.light.value()) << 12) |
                    (localID & 0xFFFu);
                found = true;
            }

            if (found) {
                return RE::TESForm::LookupByID(runtimeID);
            }

            // Strategy 3: raw local ID (last resort, usually wrong for modded forms)
            return RE::TESForm::LookupByID(localID);
        }
        catch (const std::exception& e) {
            SKSE::log::error("JContainersNG: DecodeForm exception on '{}': {}", formString, e.what());
            return nullptr;
        }
    }

} // namespace FormSerializer