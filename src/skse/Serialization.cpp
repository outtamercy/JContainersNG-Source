// --- Serialization.cpp
#include <SKSE/SKSE.h>
#include <nlohmann/json.hpp>
#include "ObjectManager.hpp"

using json = nlohmann::json;
using Handle = ObjectManager::Handle;

constexpr std::uint32_t kJContainersRecord = 'JCON';
constexpr std::uint32_t kVersion = 3; // v3: persists tes refcounts (OG persisted them too)

void SaveCallback(SKSE::SerializationInterface* a_intfc) {
    ObjectManager::Get().SweepDeadObjects();
    if (ObjectManager::Get().AreFormsDirty()) {
        ObjectManager::Get().SweepDeadForms();
    }
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

    // OG saved _tes_refCount per object, and for good reason: Papyrus script
    // properties holding these handles persist in the same save file, so a
    // retained handle MUST survive the round trip or the mod's object gets
    // eaten by the next GC while the script is still pointing at it
    json tesRefs = json::object();
    for (const auto& [h, count] : ObjectManager::Get().GetTesRefCounts()) {
        tesRefs[std::to_string(h)] = count;
    }
    saveDoc["tesRefs"] = tesRefs;

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

        if (version != 2 && version != 3) {
            SKSE::log::warn("JContainersNG: co-save version {} unsupported, skipping", version);
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

            // edges are the ground truth — rebuild Internal refs from the graph.
            // this zeroes ALL counters first, so order matters:
            // edges -> tes counts -> root pin
            ObjectManager::Get().RebuildEdgeRefs();

            // v3: put the tes refcounts back. retained handles survive the load,
            // same as OG. v2 saves have no tesRefs — v2 never had working
            // lifetime anyway, nothing worth preserving is lost
            if (version >= 3 && saveDoc.contains("tesRefs") && saveDoc["tesRefs"].is_object()) {
                for (auto& [key, val] : saveDoc["tesRefs"].items()) {
                    Handle h = static_cast<Handle>(std::stoul(key));
                    ObjectManager::Get().RestoreTesRefCount(h, val.get<int>());
                }
            }

            if (saveDoc.contains("nextHandle")) {
                ObjectManager::Get().SetNextHandle(saveDoc["nextHandle"].get<Handle>());
            }

            if (saveDoc.contains("jdbRoot")) {
                ObjectManager::Get().SetJDBRoot(saveDoc["jdbRoot"].get<Handle>());
            }

            // G4: a load can mean uninstalled mods — dead forms wont fire
            // TESFormDeleteEvent, they just stop resolving. flag the sweep so
            // the first save after every load cleans the corpses. steady-state
            // saves stay dirty-gated and cheap
            ObjectManager::Get().MarkFormsDirty();

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