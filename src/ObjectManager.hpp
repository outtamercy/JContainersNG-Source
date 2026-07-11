#pragma once
#include <SKSE/SKSE.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <string>

using json = nlohmann::json;

class ObjectManager {
public:
    using Handle = std::uint32_t;
    static constexpr Handle INVALID_HANDLE = 0;
    static constexpr const char* REF_KEY = "__jc_ref";

    static ObjectManager& Get() {
        static ObjectManager instance;
        return instance;
    }

    // --- handle factory ---

    Handle CreateObject() {
        std::lock_guard<std::mutex> lock(_mutex);
        Handle id = AllocateHandle();
        _registry[id] = std::make_shared<json>(json::object());
        _refCounts[id] = 1;
        return id;
    }

    Handle CreateArray() {
        std::lock_guard<std::mutex> lock(_mutex);
        Handle id = AllocateHandle();
        _registry[id] = std::make_shared<json>(json::array());
        _refCounts[id] = 1;
        return id;
    }

    Handle CreateNull() {
        std::lock_guard<std::mutex> lock(_mutex);
        Handle id = AllocateHandle();
        _registry[id] = std::make_shared<json>(json(nullptr));
        _refCounts[id] = 1;
        return id;
    }

    // Register an existing json blob (used during deserialization)
    Handle RegisterObject(json obj) {
        std::lock_guard<std::mutex> lock(_mutex);
        Handle id = AllocateHandle();
        _registry[id] = std::make_shared<json>(std::move(obj));
        _refCounts[id] = 1;
        return id;
    }

    // --- ref counting ---

    void Retain(Handle handle) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _refCounts.find(handle);
        if (it != _refCounts.end()) {
            it->second++;
        }
    }

    void Release(Handle handle) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _refCounts.find(handle);
        if (it != _refCounts.end()) {
            it->second--;
            if (it->second <= 0) {
                _registry.erase(handle);
                _refCounts.erase(it);
            }
        }
    }

    int GetRefCount(Handle handle) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _refCounts.find(handle);
        if (it != _refCounts.end()) return it->second;
        return 0;
    }

    // --- raw access ---

    bool IsValid(Handle handle) const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _registry.contains(handle);
    }

    std::shared_ptr<json> GetObject(Handle handle) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _registry.find(handle);
        if (it != _registry.end()) return it->second;
        return nullptr;
    }

    // Non-owning pointer for quick access inside already-locked contexts
    json* GetObjectPtr(Handle handle) {
        auto it = _registry.find(handle);
        if (it != _registry.end()) return it->second.get();
        return nullptr;
    }

    // --- snapshot for serialization ---

    std::vector<Handle> GetAllHandles() const {
        std::lock_guard<std::mutex> lock(_mutex);
        std::vector<Handle> result;
        result.reserve(_registry.size());
        for (const auto& [h, _] : _registry) {
            result.push_back(h);
        }
        return result;
    }

    // --- nuclear option ---

    void Clear() {
        std::lock_guard<std::mutex> lock(_mutex);
        _registry.clear();
        _refCounts.clear();
        _nextHandle = 1;
        _jdbRoot = 0;
    }

    // --- JDB root ---

    Handle GetJDBRoot() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_jdbRoot == 0) {
            _jdbRoot = AllocateHandle();
            _registry[_jdbRoot] = std::make_shared<json>(json::object());
            _refCounts[_jdbRoot] = 1; // root lives forever
        }
        return _jdbRoot;
    }

    void SetJDBRoot(Handle handle) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_registry.contains(handle)) {
            _jdbRoot = handle;
            _refCounts[_jdbRoot] = 1;
        }
    }

    // --- deserialization helpers ---

    void RestoreObject(Handle handle, json obj) {
        std::lock_guard<std::mutex> lock(_mutex);
        _registry[handle] = std::make_shared<json>(std::move(obj));
        _refCounts[handle] = 1;
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

private:
    ObjectManager() : _nextHandle(1), _jdbRoot(0) {}

    Handle AllocateHandle() {
        return _nextHandle++;
    }

    std::atomic<Handle> _nextHandle{ 1 };
    std::unordered_map<Handle, std::shared_ptr<json>> _registry;
    std::unordered_map<Handle, int> _refCounts;
    Handle _jdbRoot;
    mutable std::mutex _mutex;
};