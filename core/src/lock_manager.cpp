#include "pureleaf/lock_manager.h"

namespace pureleaf {

std::mutex& ProjectLockManager::getLock(const std::string& projectId) {
    std::lock_guard<std::mutex> lk(mapMutex_);
    auto& slot = locks_[projectId];
    if (!slot) {
        slot = std::make_unique<std::mutex>();
    }
    return *slot;
}

void ProjectLockManager::lock(const std::string& projectId) {
    // getLock returns a reference into the map; the map entry is stable
    // because unordered_map does not invalidate references on insert.
    getLock(projectId).lock();
}

void ProjectLockManager::unlock(const std::string& projectId) {
    getLock(projectId).unlock();
}

}  // namespace pureleaf
