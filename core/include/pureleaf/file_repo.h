#pragma once

#include <string>
#include <vector>

#include "pureleaf/project.h"
#include "pureleaf/types.h"

namespace pureleaf {

class Database;

/// A file or directory entry in a project.
/// Uses adjacency-list tree model (parent_id → children).
struct FileEntry {
    std::string id;
    std::string projectId;
    std::string parentId;  ///< Empty for root-level entries.
    std::string name;      ///< Bare name, no path separators.
    bool isDir = false;
    int64_t createdAt = 0;
};

/// Data access for the `files` table (adjacency-list tree).
class FileRepo {
public:
    explicit FileRepo(Database& db);

    /// Creates a file or directory entry.
    Result<FileEntry> create(const std::string& projectId, const std::string& parentId,
                             const std::string& name, bool isDir);

    /// Fetches a single entry by id.
    Result<FileEntry> get(const std::string& id);

    /// Finds an entry by project + parent + name.
    /// parentId can be empty for root-level.
    Result<FileEntry> findByName(const std::string& projectId, const std::string& parentId,
                                 const std::string& name);

    /// Lists direct children of a parent (or root if parentId is empty).
    Result<std::vector<FileEntry>> listChildren(const std::string& projectId,
                                                const std::string& parentId);

    /// Lists all entries in a project (flat).
    Result<std::vector<FileEntry>> listByProject(const std::string& projectId);

    /// Renames an entry.
    Result<FileEntry> rename(const std::string& id, const std::string& newName);

    /// Deletes a single entry (no recursion — use FileService for that).
    bool remove(const std::string& id);

    /// Deletes all entries in a project (used during project deletion).
    bool removeAllByProject(const std::string& projectId);

private:
    Database& db_;
};

}  // namespace pureleaf
