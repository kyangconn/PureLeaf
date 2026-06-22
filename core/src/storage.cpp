#include "pureleaf/storage.h"

namespace pureleaf {

std::string BlobStorage::blobRelativePath(const std::string& hash) {
    if (hash.size() < 2) return hash;
    // {hash[:2]}/{full hash} — sharded by first two hex chars.
    return hash.substr(0, 2) + "/" + hash;
}

std::string BlobStorage::sha256Hex(const std::string& content) {
    // TODO: vendor a SHA-256 implementation into third_party/.
    (void)content;
    return {};
}

}  // namespace pureleaf
