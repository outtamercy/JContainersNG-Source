// ObjectManager

#pragma once
#include <SKSE/SKSE.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <atomic>
#include <string>
#include <vector>
#include <chrono>
#include "FormSerializer.hpp"

using json = nlohmann::json;

// who's holding this thing alive. mirrors OG object_base's four counters
enum class RefDomain : std::uint8_t {
    Internal,   // OG _refCount — ref-marker edges from other objects, root pins
    TES,        // OG _tes_refCount — JValue.retain() / release()
    Stack,      // OG _stack_refCount — parity only, fresh lua_State per eval
    AQueue,     // OG _aqueue_refCount — the grace pin
};

class ObjectManager {
public:
    using Handle = std::uint32_t;
    static constexpr Handle INVALID_HANDLE = 0;
    static constexpr const char* REF_KEY = "__jc_ref";
    static constexpr const char* REF_KEY_LEGACY = "__reference";

    static ObjectManager& Get() {
        static ObjectManager instance;
        return instance;
    }

    // --- handle factory ---
    // OG prolongs at uid() (first exposure) if the object has no internal owners.
    // our handles are minted at birth, so birth IS the exposure — everything
    // starts with the 10-second aqueue pin

    Handle CreateObject() {
        std::lock_guard<std::mutex> lock(_mutex);
        Handle id = AllocateHandle();
        _objects[id] = MakeEntry(std::make_shared<json>(json::object()));
        return id;
    }

    Handle CreateArray() {
        std::lock_guard<std::mutex> lock(_mutex);
        Handle id = AllocateHandle();
        _objects[id] = MakeEntry(std::make_shared<json>(json::array()));
        return id;
    }

    Handle CreateNull() {
        std::lock_guard<std::mutex> lock(_mutex);
        Handle id = AllocateHandle();
        _objects[id] = MakeEntry(std::make_shared<json>(json(nullptr)));
        return id;
    }

    // Register an existing json blob (deserialization, inline-json promotion)
    Handle RegisterObject(json obj) {
        std::lock_guard<std::mutex> lock(_mutex);
        Handle id = AllocateHandle();
        _objects[id] = MakeEntry(std::make_shared<json>(std::move(obj)));
        return id;
    }

    // --- ref counting ---
    // OG RULE, do not "fix": Release NEVER deletes. when the last owner lets go,
    // the object goes back to the aqueue with a fresh 10s. deletion happens ONLY
    // in AQueueDrop (timer, last owner) and CollectGarbage

    void Retain(Handle handle, RefDomain domain = RefDomain::TES) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _objects.find(handle);
        if (it == _objects.end()) return;
        Counter(it->second, domain)++;
    }

    void Release(Handle handle, RefDomain domain = RefDomain::TES) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _objects.find(handle);
        if (it == _objects.end()) return;

        int& counter = Counter(it->second, domain);
        if (counter <= 0) {
            // double-release — somebody's counting is off. scream instead of
            // corrupting the count and UB-ing quietly at 3 AM
            SKSE::log::error("JContainersNG: Release underflow on handle {} domain {}", handle, static_cast<int>(domain));
            return;
        }
        counter--;

        if (TotalRefs(it->second) == 0) {
            // noOwners() -> prolong_lifetime(). OG behavior: object gets 10 more
            // seconds instead of dying on the spot. if the pin is somehow already
            // there this is a no-op (pinned implies aqueue >= 1 implies total > 0,
            // so we can't actually be here while pinned — but paranoia is free)
            ProlongLocked(handle, it->second);
        }
    }

    int GetRefCount(Handle handle) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _objects.find(handle);
        if (it == _objects.end()) return 0;
        return TotalRefs(it->second);
    }

    int GetRefCount(Handle handle, RefDomain domain) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _objects.find(handle);
        if (it == _objects.end()) return 0;
        return Counter(it->second, domain);
    }

    // --- edge bookkeeping ---
    // a ref marker sitting inside a container IS an Internal ref. raw MakeRef
    // assignment without EmbedEdge = leak; erasing without ReleaseEdge = leak

    void EmbedEdge(json& slot, Handle child) {
        ReleaseEdge(slot);
        if (child == INVALID_HANDLE || !IsValid(child)) {
            slot = nullptr;
            return;
        }
        Retain(child, RefDomain::Internal);
        slot = MakeRef(child);
    }

    void ReleaseEdge(const json& val) {
        Handle h;
        if (IsRefAny(val, &h)) {
            Release(h, RefDomain::Internal);
        }
    }

    void ReleaseAllEdges(const json& j) {
        if (j.is_object()) {
            Handle h;
            if (IsRefAny(j, &h)) {
                Release(h, RefDomain::Internal);
                return; // refs are leaves — the referenced object lives elsewhere
            }
            for (const auto& [key, val] : j.items()) {
                ReleaseAllEdges(val);
            }
        }
        else if (j.is_array()) {
            for (const auto& elem : j) {
                ReleaseAllEdges(elem);
            }
        }
    }

    // --- root pins (JDB root, pools) ---
    // OG has no equivalent concept — its database root is held alive by the
    // tes_context retaining it. ours needs an explicit pin for GC rooting

    void AddRootPin(Handle handle) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _objects.find(handle);
        if (it == _objects.end()) return;
        it->second.rootPins++;
    }

    void RemoveRootPin(Handle handle) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _objects.find(handle);
        if (it == _objects.end()) return;
        if (it->second.rootPins > 0) it->second.rootPins--;
    }

    // --- lifetime policy ---

    // OG zero_lifetime() == not_prolong_lifetime(): the aqueue pin is marked
    // expired and the object dies NEXT TICK (~2s), NOT after a fresh 10s.
    // on an unpinned (e.g. tes-held) object this is just flag bookkeeping —
    // OG behaves the same way
    void SetZeroLifetime(Handle handle) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _objects.find(handle);
        if (it == _objects.end()) return;
        it->second.zeroLifetime = true;
        if (it->second.tes == 0 && it->second.stack == 0 && it->second.aqueuePinned) {
            it->second.expireEarly = true; // tick treats this as "time's up"
        }
    }

    bool HasZeroLifetime(Handle handle) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _objects.find(handle);
        if (it == _objects.end()) return true;
        return it->second.zeroLifetime;
    }

    // --- autorelease queue interface (queue thread calls these) ---

    // queue drains this every tick — handles born since last tick
    std::vector<Handle> TakePendingExposures() {
        std::lock_guard<std::mutex> lock(_mutex);
        std::vector<Handle> out;
        out.swap(_pendingExpose);
        return out;
    }

    enum class QueueVerdict : std::uint8_t { Keep, Drop };

    // one lock, one answer: should the queue release this pin right now?
    // drop reasons: stale handle / zeroLifetime marked it expired / adopted
    // (tes, stack, or root-pinned) / 10 seconds are simply up
    QueueVerdict GetQueueVerdict(Handle handle, std::chrono::steady_clock::time_point exposedAt) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _objects.find(handle);
        if (it == _objects.end()) return QueueVerdict::Drop; // stale — evaporate
        const auto& e = it->second;
        if (e.expireEarly) return QueueVerdict::Drop;
        if (e.tes > 0 || e.stack > 0 || e.rootPins > 0) return QueueVerdict::Drop; // adopted
        if (!e.zeroLifetime) return QueueVerdict::Drop;
        if (std::chrono::steady_clock::now() - exposedAt >= std::chrono::seconds(10)) return QueueVerdict::Drop;
        return QueueVerdict::Keep;
    }

    // OG _aqueue_release(): drop the pin; if it was the last owner, the object dies
    void AQueueDrop(Handle handle) {
        std::shared_ptr<json> corpse;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = _objects.find(handle);
            if (it == _objects.end()) return;
            if (!it->second.aqueuePinned) return; // somebody beat us to it
            it->second.aqueuePinned = false;
            it->second.expireEarly = false;
            if (it->second.aqueue <= 0) {
                SKSE::log::error("JContainersNG: AQueueDrop underflow on handle {}", handle);
                return;
            }
            it->second.aqueue--;
            if (TotalRefs(it->second) == 0) {
                // aqueue was the last owner — this is the ONLY place Release-style
                // death is allowed to happen. yank first, release edges after
                corpse = std::move(it->second.data);
                _objects.erase(it);
            }
        }
        // outside the lock — child edge releases recurse through Release()
        if (corpse) {
            ReleaseAllEdges(*corpse);
        }
    }

    // --- garbage collector ---
    // OG garbage_collector::u_collect, translated. roots = tes-retained or
    // in-aqueue (stack explicitly NOT a root in OG — "this ref.count is not
    // persistent"; rootPins is our addition standing in for OG's context-held root).
    // unreachable + ownerless -> dies now. unreachable cycle members -> their
    // edges get released, which drops them to ownerless -> aqueue finishes them.
    // our two-phase yank gets the same outcome in one pass

    void CollectGarbage() {
        std::vector<std::shared_ptr<json>> corpses;
        std::size_t doomedCount = 0;
        {
            std::lock_guard<std::mutex> lock(_mutex);

            // mark — roots: tes > 0 || aqueue > 0 || rootPins > 0
            std::unordered_set<Handle> reachable;
            std::vector<Handle> pending;
            reachable.reserve(_objects.size());
            for (const auto& [h, e] : _objects) {
                if (e.tes > 0 || e.aqueue > 0 || e.rootPins > 0) {
                    if (reachable.insert(h).second) {
                        pending.push_back(h);
                    }
                }
            }
            while (!pending.empty()) {
                const Handle h = pending.back();
                pending.pop_back();
                auto it = _objects.find(h);
                if (it == _objects.end() || !it->second.data) continue;
                CollectChildRefs(*it->second.data, [&](Handle child) {
                    if (child != INVALID_HANDLE && reachable.insert(child).second) {
                        pending.push_back(child);
                    }
                    });
            }

            // sweep — yank every unreachable entry under one lock so the doomed
            // set is closed before any edge release happens
            for (auto it = _objects.begin(); it != _objects.end();) {
                if (reachable.contains(it->first)) {
                    ++it;
                    continue;
                }
                corpses.push_back(std::move(it->second.data));
                it = _objects.erase(it);
            }
            doomedCount = corpses.size();
        }

        // guts first — child Releases hit handles already gone from the
        // registry and no-op instead of double-freeing
        for (auto& corpse : corpses) {
            if (corpse) {
                ReleaseAllEdges(*corpse);
            }
        }
        if (doomedCount > 0) {
            SKSE::log::info("JContainersNG: GC collected {} unreachable objects", doomedCount);
        }
    }

    // kept name — SaveCallback already calls this, it just does the real thing now
    void SweepDeadObjects() {
        CollectGarbage();
    }

    // after a load: edges are the ground truth. internal refs get rebuilt by
    // walking every object's json. tes counts are NOT rebuilt here — they're
    // restored from the co-save (v3) right after this call
    void RebuildEdgeRefs() {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto& [h, e] : _objects) {
            e.internal = 0;
            e.tes = 0;
            e.stack = 0;
            e.aqueue = 0;
            e.aqueuePinned = false;
            e.expireEarly = false;
            e.rootPins = 0;
            e.zeroLifetime = true;
        }
        _pendingExpose.clear();
        for (const auto& [h, e] : _objects) {
            if (!e.data) continue;
            CollectChildRefs(*e.data, [&](Handle child) {
                auto cit = _objects.find(child);
                if (cit != _objects.end()) {
                    cit->second.internal++;
                }
                });
        }
    }

    // v3 co-save support — restore one object's persisted tes count
    void RestoreTesRefCount(Handle handle, int count) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _objects.find(handle);
        if (it == _objects.end()) return;
        it->second.tes = count;
    }

    // snapshot of nonzero tes counts for the co-save
    std::unordered_map<Handle, int> GetTesRefCounts() const {
        std::lock_guard<std::mutex> lock(_mutex);
        std::unordered_map<Handle, int> out;
        for (const auto& [h, e] : _objects) {
            if (e.tes > 0) {
                out[h] = e.tes;
            }
        }
        return out;
    }

    // --- raw access ---

    bool IsValid(Handle handle) const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _objects.contains(handle);
    }

    std::shared_ptr<json> GetObject(Handle handle) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _objects.find(handle);
        if (it != _objects.end()) return it->second.data;
        return nullptr;
    }

    // Non-owning pointer. unsafe across threads now that the queue thread
    // exists — prefer GetObject() (the shared_ptr IS the guard)
    json* GetObjectPtr(Handle handle) {
        auto it = _objects.find(handle);
        if (it != _objects.end()) return it->second.data.get();
        return nullptr;
    }

    // --- snapshot for serialization ---

    std::vector<Handle> GetAllHandles() const {
        std::lock_guard<std::mutex> lock(_mutex);
        std::vector<Handle> result;
        result.reserve(_objects.size());
        for (const auto& [h, _] : _objects) {
            result.push_back(h);
        }
        return result;
    }

    // --- nuclear option ---
    // queue needs NO flush call: its stale entries fail the verdict check and
    // self-evaporate next tick, handles never recycle so there's no ABA.
    // OG u_clearState nullifies cross-refs then mass-deletes; wiping the map
    // gets the same outcome since edges/counts die together

    void Clear() {
        std::lock_guard<std::mutex> lock(_mutex);
        _objects.clear();
        _pendingExpose.clear();
        _nextHandle = 1;
        _jdbRoot = 0;
    }

    // --- forms ---

    void MarkFormsDirty() {
        _formsDirty.store(true);
    }

    bool AreFormsDirty() const {
        return _formsDirty.load();
    }

    // Nuke any form strings that point to dead TESForms. Only called from SaveCallback.
    void SweepDeadForms() {
        std::lock_guard<std::mutex> lock(_mutex);
        _formsDirty.store(false);

        for (auto& [h, e] : _objects) {
            if (!e.data) continue;
            SweepJsonForDeadForms(*e.data);
        }
    }

    // --- JDB root ---

    Handle GetJDBRoot() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_jdbRoot == 0) {
            _jdbRoot = AllocateHandle();
            _objects[_jdbRoot] = MakeEntry(std::make_shared<json>(json::object()));
            _objects[_jdbRoot].rootPins = 1; // root lives forever
        }
        return _jdbRoot;
    }

    void SetJDBRoot(Handle handle) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _objects.find(handle);
        if (it != _objects.end()) {
            _jdbRoot = handle;
            it->second.rootPins++;
        }
    }

    // --- deserialization helpers ---

    void RestoreObject(Handle handle, json obj) {
        std::lock_guard<std::mutex> lock(_mutex);
        Entry e;
        e.data = std::make_shared<json>(std::move(obj));
        // counters zeroed — RebuildEdgeRefs + RestoreTesRefCount reconstruct them
        _objects[handle] = std::move(e);
        if (handle >= _nextHandle) {
            _nextHandle = handle + 1;
        }
    }

    Handle GetNextHandle() const {
        return _nextHandle.load();
    }

    void SetNextHandle(Handle handle) {
        _nextHandle.store(handle);
    }

    // --- internal ref marker helpers ---

    static json MakeRef(Handle handle) {
        return json::object({ {REF_KEY, handle} });
    }

    static bool IsRef(const json& j, Handle* outHandle = nullptr) {
        if (!j.is_object()) return false;
        if (!j.contains(REF_KEY)) return false;
        // Relaxed: extra keys don't invalidate the ref. Prevents data vanishing
        // if a script accidentally tacks metadata onto a ref object.
        if (outHandle) {
            *outHandle = j[REF_KEY].get<Handle>();
        }
        return true;
    }

    // edge walking honors the legacy key too — a leaked edge is a leaked object
    static bool IsRefAny(const json& j, Handle* outHandle = nullptr) {
        if (!j.is_object()) return false;
        if (j.contains(REF_KEY)) {
            if (outHandle) *outHandle = j[REF_KEY].get<Handle>();
            return true;
        }
        if (j.contains(REF_KEY_LEGACY)) {
            if (outHandle) *outHandle = j[REF_KEY_LEGACY].get<Handle>();
            return true;
        }
        return false;
    }

private:
    struct Entry {
        std::shared_ptr<json> data;
        int internal = 0;
        int tes = 0;
        int stack = 0;
        int aqueue = 0;
        int rootPins = 0;
        bool zeroLifetime = true;    // objects are born mortal
        bool aqueuePinned = false;
        bool expireEarly = false;    // zeroLifetime() was called — dies next tick
    };

    ObjectManager() : _nextHandle(1), _jdbRoot(0) {}

    Entry MakeEntry(std::shared_ptr<json> data) {
        Entry e;
        e.data = std::move(data);
        // born exposed — OG uid() prolongs unowned objects at first exposure;
        // our exposure IS the birth
        e.aqueue = 1;
        e.aqueuePinned = true;
        _pendingExpose.push_back(_nextHandle - 1); // AllocateHandle already ran
        return e;
    }

    // noOwners() -> prolong_lifetime(). caller holds _mutex.
    // fresh 10s pin, expireEarly cleared — OG gives public objects a real
    // second chance, not a shortened one
    void ProlongLocked(Handle handle, Entry& e) {
        if (e.aqueuePinned) return;
        e.aqueuePinned = true;
        e.expireEarly = false;
        e.aqueue++;
        _pendingExpose.push_back(handle);
    }

    static int& Counter(Entry& e, RefDomain domain) {
        switch (domain) {
        case RefDomain::Internal: return e.internal;
        case RefDomain::TES:      return e.tes;
        case RefDomain::Stack:    return e.stack;
        case RefDomain::AQueue:   return e.aqueue;
        }
        return e.internal; // unreachable, just shutting the compiler up
    }

    static int Counter(const Entry& e, RefDomain domain) {
        switch (domain) {
        case RefDomain::Internal: return e.internal;
        case RefDomain::TES:      return e.tes;
        case RefDomain::Stack:    return e.stack;
        case RefDomain::AQueue:   return e.aqueue;
        }
        return e.internal;
    }

    static int TotalRefs(const Entry& e) {
        return e.internal + e.tes + e.stack + e.aqueue;
    }

    // walk every handle an object's own json directly points at.
    // refs are leaves; everything else recurses
    static void CollectChildRefs(const json& j, const std::function<void(Handle)>& visitor) {
        if (j.is_object()) {
            Handle h;
            if (IsRefAny(j, &h)) {
                visitor(h);
                return;
            }
            for (const auto& [key, val] : j.items()) {
                CollectChildRefs(val, visitor);
            }
        }
        else if (j.is_array()) {
            for (const auto& elem : j) {
                CollectChildRefs(elem, visitor);
            }
        }
    }

    Handle AllocateHandle() {
        return _nextHandle++;
    }

    // values pointing at dead forms get nulled. KEYS that are dead form strings
    // get the whole entry erased — OG's form_map::u_onLoaded drops em entirely,
    // leaving em would leak dead-key entries forever
    void SweepJsonForDeadForms(json& j) {
        if (j.is_object()) {
            // Check if this object is a ref — don't recurse into refs, they live elsewhere
            if (j.contains(REF_KEY) || j.contains(REF_KEY_LEGACY)) return;

            for (auto it = j.begin(); it != j.end(); ) {
                const std::string& key = it.key();
                auto& val = it.value();

                // dead form key? whole entry goes
                if (FormSerializer::IsFormString(key) && !FormSerializer::DecodeForm(key)) {
                    ReleaseEdge(val);
                    it = j.erase(it);
                    continue;
                }

                if (val.is_string()) {
                    std::string str = val.get<std::string>();
                    if (FormSerializer::IsFormString(str) && !FormSerializer::DecodeForm(str)) {
                        val = nullptr; // null out dead form entries
                    }
                }
                else if (val.is_object() || val.is_array()) {
                    SweepJsonForDeadForms(val);
                }
                ++it;
            }
        }
        else if (j.is_array()) {
            for (auto& elem : j) {
                if (elem.is_string()) {
                    std::string str = elem.get<std::string>();
                    if (FormSerializer::IsFormString(str) && !FormSerializer::DecodeForm(str)) {
                        elem = nullptr;
                    }
                }
                else if (elem.is_object() || elem.is_array()) {
                    SweepJsonForDeadForms(elem);
                }
            }
        }
    }

    std::atomic<Handle> _nextHandle{ 1 };
    std::unordered_map<Handle, Entry> _objects;
    std::vector<Handle> _pendingExpose;   // born/prolonged since last tick — queue drains this
    std::atomic<bool> _formsDirty{ false };
    Handle _jdbRoot;
    mutable std::mutex _mutex;
};