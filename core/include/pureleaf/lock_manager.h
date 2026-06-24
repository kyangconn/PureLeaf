#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

namespace pureleaf {

/// Per-project mutex manager.
///
/// Serializes concurrent operations on the same project (compile, save,
/// snapshot, etc.). Different projects run in parallel.
///
/// Ported from the Go version's ProjectLockManager.
class ProjectLockManager {
public:
    /// Acquires the lock for a project. Blocks until the lock is available.
    void lock(const std::string& projectId);

    /// Releases the lock for a project.
    void unlock(const std::string& projectId);

    /// RAII guard for a project lock.
    class Guard {
    public:
        Guard(ProjectLockManager* mgr, std::string projectId)
            : mgr_(mgr), projectId_(std::move(projectId)) {
            mgr_->lock(projectId_);
        }
        ~Guard() {
            if (mgr_) mgr_->unlock(projectId_);
        }
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
        Guard(Guard&& other) noexcept : mgr_(other.mgr_), projectId_(std::move(other.projectId_)) {
            other.mgr_ = nullptr;
        }

    private:
        ProjectLockManager* mgr_;
        std::string projectId_;
    };

    /// Convenience: returns an RAII guard.
    Guard acquire(const std::string& projectId) { return Guard(this, projectId); }

private:
    std::mutex mapMutex_;
    std::unordered_map<std::string, std::unique_ptr<std::mutex>> locks_;

    std::mutex& getLock(const std::string& projectId);
};

}  // namespace pureleaf
