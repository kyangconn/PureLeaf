#pragma once

#include <string>
#include <vector>

#include "pureleaf/file_repo.h"
#include "pureleaf/lock_manager.h"
#include "pureleaf/project.h"
#include "pureleaf/types.h"

namespace pureleaf {

class Database;

/// A node in the built file tree (for UI consumption).
struct TreeNode {
    std::string id;
    std::string name;
    bool isDir = false;
    std::string relativePath;  ///< Full path relative to project root.
    std::vector<TreeNode> children;
};

/// High-level file operations service.
///
/// Coordinates DB metadata with disk I/O, path safety, and tree building.
/// All mutations are serialized via ProjectLockManager.
class FileService {
public:
    FileService(Database& db, ProjectLockManager& lockManager);

    /// Builds the full file tree for a project from DB metadata.
    /// Scans disk to include untracked files? No — only tracked DB entries.
    Result<TreeNode> getTree(const std::string& projectId);

    /// Reads file content from disk.
    /// `relativePath` is relative to the project root.
    Result<std::string> getContent(const std::string& projectRootPath,
                                   const std::string& relativePath);

    /// Writes file content to disk (atomic write via .tmp + rename).
    Result<void> updateContent(const std::string& projectRootPath, const std::string& relativePath,
                               const std::string& content);

    /// Creates a new file or directory.
    /// `parentRelativePath` can be empty for root level.
    Result<FileEntry> createEntry(const std::string& projectId, const std::string& projectRootPath,
                                  const std::string& parentRelativePath, const std::string& name,
                                  bool isDir);

    /// Renames a file or directory.
    Result<FileEntry> renameEntry(const std::string& projectId, const std::string& projectRootPath,
                                  const std::string& relativePath, const std::string& newName);

    /// Deletes a file or directory (recursive for directories).
    Result<void> deleteEntry(const std::string& projectId, const std::string& projectRootPath,
                             const std::string& relativePath);

private:
    Database& db_;
    ProjectLockManager& lockManager_;
    FileRepo fileRepo_;

    /// Resolves a relative path to a FileEntry by walking the tree.
    Result<FileEntry> resolveByPath(const std::string& projectId, const std::string& relativePath);

    /// Recursively collects all descendant ids of a directory entry.
    std::vector<std::string> collectDescendants(const std::string& projectId,
                                                const std::string& parentId);

    /// Builds tree nodes from a flat list.
    TreeNode buildTree(const std::vector<FileEntry>& entries, const std::string& parentId,
                       const std::string& basePath);
};

}  // namespace pureleaf
