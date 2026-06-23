#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pureleaf {

/// A LaTeX project managed by PureLeaf.
struct Project {
    std::string id;
    std::string name;
    std::string rootPath;
    std::string mainTex;  ///< Relative path to the main .tex file (empty if unset).
    int64_t createdAt = 0;
    int64_t updatedAt = 0;
};

/// A node in the project file tree.
struct FileNode {
    std::string path;  ///< Relative to project root, using '/' separators.
    std::string name;
    bool isDir = false;
    std::vector<FileNode> children;
};

/// A single revision of a file, pointing to a content-addressed blob.
struct Revision {
    std::string id;
    std::string fileId;
    std::string blobHash;  ///< SHA-256 hex string.
    int64_t createdAt = 0;
    int64_t size = 0;
};

}  // namespace pureleaf
