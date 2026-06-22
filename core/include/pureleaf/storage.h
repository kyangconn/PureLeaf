#pragma once

#include "pureleaf/types.h"

#include <string>

namespace pureleaf {

/// Content-addressed blob storage (SHA-256).
///
/// Blobs are stored at `{root}/{hash[:2]}/{hash}` to keep directories
/// sharded. Paths are stored as *relative* strings so the data root can
/// be relocated without rewriting metadata.
class BlobStorage {
public:
    explicit BlobStorage(std::string rootDir) : rootDir_(std::move(rootDir)) {}

    /// Returns the relative storage path for a hash: `{hash[:2]}/{hash}`.
    static std::string blobRelativePath(const std::string& hash);

    /// Computes the SHA-256 hex digest of `content`.
    /// TODO: vendor a SHA-256 implementation into third_party/.
    static std::string sha256Hex(const std::string& content);

    const std::string& rootDir() const { return rootDir_; }

private:
    std::string rootDir_;
};

}  // namespace pureleaf
