#pragma once

#include "pureleaf/types.h"

#include <cstdint>
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
    static std::string sha256Hex(const std::string& content);

    /// Writes content to blob storage. Returns the SHA-256 hash, or empty on
    /// error. If the blob already exists (same hash), it is a no-op (dedup).
    std::string writeBlob(const std::string& content);

    /// Reads blob content by hash. Returns empty Result with IoError on miss.
    Result<std::string> readBlob(const std::string& hash);

    /// Checks whether a blob exists.
    bool exists(const std::string& hash);

    /// Deletes a blob. Used by GC.
    bool deleteBlob(const std::string& hash);

    const std::string& rootDir() const { return rootDir_; }

private:
    /// Returns the absolute path for a given hash.
    std::string blobFullPath(const std::string& hash);

    std::string rootDir_;
};

}  // namespace pureleaf
