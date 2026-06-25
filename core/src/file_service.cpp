#include "pureleaf/file_service.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "pureleaf/database.h"
#include "pureleaf/path_util.h"

namespace pureleaf {

namespace fs = std::filesystem;

FileService::FileService(Database& db, ProjectLockManager& lockManager)
    : db_(db), lockManager_(lockManager), fileRepo_(db) {}

// ── Tree building ─────────────────────────────────────────────────

TreeNode FileService::buildTree(const std::vector<FileEntry>& entries, const std::string& parentId,
                                const std::string& basePath) {
    TreeNode node;
    node.id = parentId;

    for (const auto& entry : entries) {
        if (entry.parentId == parentId) {
            TreeNode child;
            child.id = entry.id;
            child.name = entry.name;
            child.isDir = entry.isDir;
            child.relativePath =
                basePath.empty() ? entry.name : path_util::join(basePath, entry.name);

            if (entry.isDir) {
                child.children = std::vector<TreeNode>{};
                // Recurse: we need a subtree, so call buildTree for this child.
                auto subTree = buildTree(entries, entry.id, child.relativePath);
                child.children = std::move(subTree.children);
            }
            node.children.push_back(std::move(child));
        }
    }

    // Sort: directories first, then alphabetically.
    std::sort(node.children.begin(), node.children.end(), [](const TreeNode& a, const TreeNode& b) {
        if (a.isDir != b.isDir) return a.isDir;  // dirs first
        return a.name < b.name;
    });

    return node;
}

Result<TreeNode> FileService::getTree(const std::string& projectId) {
    auto list = fileRepo_.listByProject(projectId);
    if (!list.ok()) return Result<TreeNode>::Err(list.error);

    // The root is a virtual node with empty parentId.
    TreeNode root;
    root.name = "/";
    root.isDir = true;
    root.relativePath = "";

    for (const auto& entry : list.value()) {
        if (entry.parentId.empty()) {
            TreeNode child;
            child.id = entry.id;
            child.name = entry.name;
            child.isDir = entry.isDir;
            child.relativePath = entry.name;

            if (entry.isDir) {
                auto sub = buildTree(list.value(), entry.id, child.relativePath);
                child.children = std::move(sub.children);
            }
            root.children.push_back(std::move(child));
        }
    }

    std::sort(root.children.begin(), root.children.end(), [](const TreeNode& a, const TreeNode& b) {
        if (a.isDir != b.isDir) return a.isDir;
        return a.name < b.name;
    });

    return Result<TreeNode>::Ok(std::move(root));
}

// ── Content read/write ────────────────────────────────────────────

Result<std::string> FileService::getContent(const std::string& projectRootPath,
                                            const std::string& relativePath) {
    if (!path_util::isSafeRelativePath(relativePath)) {
        return Result<std::string>::Err(Error::InvalidPath);
    }

    std::string fullPath = path_util::joinNative(projectRootPath, relativePath);
    if (!fs::exists(fullPath)) {
        return Result<std::string>::Err(Error::NotFound);
    }

    std::ifstream in(fullPath, std::ios::binary);
    if (!in) return Result<std::string>::Err(Error::IoError);
    std::ostringstream ss;
    ss << in.rdbuf();
    return Result<std::string>::Ok(ss.str());
}

Result<void> FileService::updateContent(const std::string& projectRootPath,
                                        const std::string& relativePath,
                                        const std::string& content) {
    if (!path_util::isSafeRelativePath(relativePath)) {
        return Result<void>::Err(Error::InvalidPath);
    }

    std::string fullPath = path_util::joinNative(projectRootPath, relativePath);

    // Ensure parent directory exists.
    std::error_code ec;
    fs::path parentDir = fs::path(fullPath).parent_path();
    fs::create_directories(parentDir, ec);
    if (ec) return Result<void>::Err(Error::IoError);

    // Atomic write: .tmp then rename.
    std::string tmpPath = fullPath + ".pureleaf_tmp";
    {
        std::ofstream out(tmpPath, std::ios::binary);
        if (!out) return Result<void>::Err(Error::IoError);
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!out) {
            fs::remove(tmpPath, ec);
            return Result<void>::Err(Error::IoError);
        }
    }
    fs::rename(tmpPath, fullPath, ec);
    if (ec) {
        fs::remove(tmpPath, ec);
        return Result<void>::Err(Error::IoError);
    }

    return Result<void>::Ok();
}

// ── Path resolution ───────────────────────────────────────────────

Result<FileEntry> FileService::resolveByPath(const std::string& projectId,
                                             const std::string& relativePath) {
    if (relativePath.empty()) {
        return Result<FileEntry>::Err(Error::InvalidArgument);
    }

    // Walk the path components.
    std::string currentParentId;
    std::string remaining = relativePath;

    while (!remaining.empty()) {
        size_t sep = remaining.find('/');
        std::string segment = (sep == std::string::npos) ? remaining : remaining.substr(0, sep);

        if (segment.empty()) break;

        auto entry = fileRepo_.findByName(projectId, currentParentId, segment);
        if (!entry.ok()) return entry;

        currentParentId = entry.value().id;

        if (sep == std::string::npos) {
            return entry;  // Found the leaf.
        }
        remaining = remaining.substr(sep + 1);
    }

    return Result<FileEntry>::Err(Error::NotFound);
}

// ── Create / Rename / Delete ──────────────────────────────────────

Result<FileEntry> FileService::createEntry(const std::string& projectId,
                                           const std::string& projectRootPath,
                                           const std::string& parentRelativePath,
                                           const std::string& name, bool isDir) {
    if (!path_util::isValidName(name)) {
        return Result<FileEntry>::Err(Error::InvalidArgument);
    }

    auto guard = lockManager_.acquire(projectId);

    // Resolve parent.
    std::string parentId;
    std::string fullRelativePath = name;

    if (!parentRelativePath.empty()) {
        if (!path_util::isSafeRelativePath(parentRelativePath)) {
            return Result<FileEntry>::Err(Error::InvalidPath);
        }
        auto parent = resolveByPath(projectId, parentRelativePath);
        if (!parent.ok()) return parent;
        if (!parent.value().isDir) {
            return Result<FileEntry>::Err(Error::InvalidArgument);
        }
        parentId = parent.value().id;
        fullRelativePath = path_util::join(parentRelativePath, name);
    }

    // Check for sibling conflict.
    auto existing = fileRepo_.findByName(projectId, parentId, name);
    if (existing.ok()) {
        return Result<FileEntry>::Err(Error::AlreadyExists);
    }

    // Create on disk first.
    std::error_code ec;
    std::string fullPath = path_util::joinNative(projectRootPath, fullRelativePath);
    if (isDir) {
        fs::create_directory(fullPath, ec);
        if (ec) return Result<FileEntry>::Err(Error::IoError);
    } else {
        // Ensure parent dir exists.
        fs::create_directories(fs::path(fullPath).parent_path(), ec);
        if (ec) return Result<FileEntry>::Err(Error::IoError);
        std::ofstream out(fullPath, std::ios::binary);
        if (!out) return Result<FileEntry>::Err(Error::IoError);
    }

    // Then create DB metadata.
    auto result = fileRepo_.create(projectId, parentId, name, isDir);
    if (!result.ok()) {
        // Compensating: remove disk entry.
        fs::remove(fullPath, ec);
    }
    return result;
}

Result<FileEntry> FileService::renameEntry(const std::string& projectId,
                                           const std::string& projectRootPath,
                                           const std::string& relativePath,
                                           const std::string& newName) {
    if (!path_util::isValidName(newName)) {
        return Result<FileEntry>::Err(Error::InvalidArgument);
    }
    if (!path_util::isSafeRelativePath(relativePath)) {
        return Result<FileEntry>::Err(Error::InvalidPath);
    }

    auto guard = lockManager_.acquire(projectId);

    auto entry = resolveByPath(projectId, relativePath);
    if (!entry.ok()) return entry;

    // Check for sibling conflict with the new name.
    auto existing = fileRepo_.findByName(projectId, entry.value().parentId, newName);
    if (existing.ok()) {
        return Result<FileEntry>::Err(Error::AlreadyExists);
    }

    // Rename on disk.
    std::string oldFullPath = path_util::joinNative(projectRootPath, relativePath);
    std::string newRelativePath = path_util::join(path_util::parent(relativePath), newName);
    std::string newFullPath = path_util::joinNative(projectRootPath, newRelativePath);

    std::error_code ec;
    fs::rename(oldFullPath, newFullPath, ec);
    if (ec) return Result<FileEntry>::Err(Error::IoError);

    // Rename in DB.
    auto result = fileRepo_.rename(entry.value().id, newName);
    if (!result.ok()) {
        // Compensating: rename back.
        fs::rename(newFullPath, oldFullPath, ec);
    }
    return result;
}

std::vector<std::string> FileService::collectDescendants(const std::string& projectId,
                                                         const std::string& parentId) {
    std::vector<std::string> ids;
    auto children = fileRepo_.listChildren(projectId, parentId);
    if (!children.ok()) return ids;

    for (const auto& child : children.value()) {
        ids.push_back(child.id);
        if (child.isDir) {
            auto sub = collectDescendants(projectId, child.id);
            ids.insert(ids.end(), sub.begin(), sub.end());
        }
    }
    return ids;
}

Result<void> FileService::deleteEntry(const std::string& projectId,
                                      const std::string& projectRootPath,
                                      const std::string& relativePath) {
    if (!path_util::isSafeRelativePath(relativePath)) {
        return Result<void>::Err(Error::InvalidPath);
    }

    auto guard = lockManager_.acquire(projectId);

    auto entry = resolveByPath(projectId, relativePath);
    if (!entry.ok()) return Result<void>::Err(entry.error);

    // Collect all descendant ids (recursive for directories).
    auto descendantIds = collectDescendants(projectId, entry.value().id);

    // Delete on disk.
    std::error_code ec;
    std::string fullPath = path_util::joinNative(projectRootPath, relativePath);
    fs::remove_all(fullPath, ec);

    // Delete in DB: descendants first, then the entry itself.
    for (const auto& id : descendantIds) {
        fileRepo_.remove(id);
    }
    fileRepo_.remove(entry.value().id);

    return Result<void>::Ok();
}

}  // namespace pureleaf
