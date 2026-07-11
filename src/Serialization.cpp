#include <SKSE/SKSE.h>
#include <nlohmann/json.hpp>
#include "ObjectManager.hpp"

using json = nlohmann::json;
using Handle = ObjectManager::Handle;

constexpr std::uint32_t kJContainersRecord = 'JCON';
constexpr std::uint32_t kVersion = 2;

void SaveCallback(SKSE::SerializationInterface* a_intfc) {
    SKSE::log::info("JContainersNG: save callback triggered");

    json saveDoc;
    saveDoc["version"] = kVersion;
    saveDoc["nextHandle"] = ObjectManager::Get().GetNextHandle();
    saveDoc["jdbRoot"] = ObjectManager::Get().GetJDBRoot();

    json registry = json::object();
    for (auto h : ObjectManager::Get().GetAllHandles()) {
        auto ptr = ObjectManager::Get().GetObject(h);
        if (ptr) {
            registry[std::to_string(h)] = *ptr;
        }
    }
    saveDoc["registry"] = registry;

    std::string payload = saveDoc.dump();
    std::size_t len = payload.size();

    if (!a_intfc->OpenRecord(kJContainersRecord, kVersion)) {
        SKSE::log::error("JContainersNG: failed to open save record");
        return;
    }
    if (!a_intfc->WriteRecordData(&len, sizeof(len))) {
        SKSE::log::error("JContainersNG: failed to write length");
        return;
    }
    if (!a_intfc->WriteRecordData(payload.data(), static_cast<std::uint32_t>(len))) {
        SKSE::log::error("JContainersNG: failed to write payload");
        return;
    }

    SKSE::log::info("JContainersNG: saved {} objects ({} bytes)", registry.size(), len);
}

void LoadCallback(SKSE::SerializationInterface* a_intfc) {
    SKSE::log::info("JContainersNG: load callback triggered");

    std::uint32_t type = 0;
    std::uint32_t version = 0;
    std::uint32_t length = 0;
    bool found = false;

    while (a_intfc->GetNextRecordInfo(type, version, length)) {
        if (type != kJContainersRecord) continue;

        if (version != kVersion) {
            SKSE::log::warn("JContainersNG: co-save version {} != {}, skipping", version, kVersion);
            continue;
        }

        found = true;

        std::size_t jsonLen = 0;
        if (!a_intfc->ReadRecordData(&jsonLen, sizeof(jsonLen))) {
            SKSE::log::error("JContainersNG: failed to read length");
            return;
        }

        std::string payload(jsonLen, '\0');
        if (!a_intfc->ReadRecordData(payload.data(), static_cast<std::uint32_t>(jsonLen))) {
            SKSE::log::error("JContainersNG: failed to read payload");
            return;
        }

        try {
            json saveDoc = json::parse(payload);

            if (!saveDoc.contains("registry") || !saveDoc["registry"].is_object()) {
                throw std::runtime_error("missing or invalid registry");
            }

            ObjectManager::Get().Clear();

            for (auto& [key, val] : saveDoc["registry"].items()) {
                Handle h = static_cast<Handle>(std::stoul(key));
                ObjectManager::Get().RestoreObject(h, val);
            }

            if (saveDoc.contains("nextHandle")) {
                ObjectManager::Get().SetNextHandle(saveDoc["nextHandle"].get<Handle>());
            }

            if (saveDoc.contains("jdbRoot")) {
                ObjectManager::Get().SetJDBRoot(saveDoc["jdbRoot"].get<Handle>());
            }

            SKSE::log::info("JContainersNG: restored {} objects", saveDoc["registry"].size());
        }
        catch (const std::exception& e) {
            SKSE::log::error("JContainersNG: co-save load failed: {}", e.what());
            ObjectManager::Get().Clear();
            Handle root = ObjectManager::Get().CreateObject();
            ObjectManager::Get().SetJDBRoot(root);
        }
        break;
    }

    if (!found) {
        // Don't wipe if we already have a JDB root (subsequent callbacks)
        if (ObjectManager::Get().GetJDBRoot() != 0) {
            SKSE::log::info("JContainersNG: load callback found no record, but JDB root exists, keeping state");
            return;
        }
        SKSE::log::info("JContainersNG: no co-save data, using fresh state");
        ObjectManager::Get().Clear();
        Handle root = ObjectManager::Get().CreateObject();
        ObjectManager::Get().SetJDBRoot(root);
    }
}

void RevertCallback(SKSE::SerializationInterface*) {
    SKSE::log::info("JContainersNG: revert callback triggered");
    ObjectManager::Get().Clear();
    Handle root = ObjectManager::Get().CreateObject();
    ObjectManager::Get().SetJDBRoot(root);
    SKSE::log::info("JContainersNG: reverted");
}