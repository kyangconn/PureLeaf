#include "pureleaf/storage.h"

#include <sha256.h>  // third_party/sha256

#include <filesystem>
#include <fstream>
#include <sstream>

namespace pureleaf {

namespace fs = std::filesystem;

std::string BlobStorage::blobRelativePath(const std::string& hash) {
    if (hash.size() < 2) return hash;
    // {hash[:2]}/{full hash} — sharded by first two hex chars.
    return hash.substr(0, 2) + "/" + hash;
}

std::string BlobStorage::sha256Hex(const std::string& content) {
    return pureleaf_sha256::sha256_hex(content);
}

std::string BlobStorage::blobFullPath(const std::string& hash) {
    // Build as fs::path and use .string() to get the platform-native form.
    // blobRelativePath uses '/', but fs::path handles the join correctly.
    auto rel = blobRelativePath(hash);
    // Split on '/' and join via fs::path to get native separators.
    fs::path result = rootDir_;
    size_t start = 0;
    while (start < rel.size()) {
        size_t sep = rel.find('/', start);
        if (sep == std::string::npos) {
            result /= rel.substr(start);
            break;
        }
        result /= rel.substr(start, sep - start);
        start = sep + 1;
    }
    return result.string();
}

std::string BlobStorage::writeBlob(const std::string& content) {
    std::string hash = sha256Hex(content);
    std::string fullPath = blobFullPath(hash);

    // Dedup: if the blob already exists, skip writing.
    std::error_code ec;
    if (fs::exists(fullPath, ec)) {
        return hash;
    }

    // Create the sharded directory.
    fs::path dir = fs::path(fullPath).parent_path();
    fs::create_directories(dir, ec);
    if (ec) return {};

    // Write atomically: write to .tmp then rename.
    std::string tmpPath = fullPath + ".tmp";
    {
        std::ofstream out(tmpPath, std::ios::binary);
        if (!out) return {};
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!out) {
            fs::remove(tmpPath, ec);
            return {};
        }
    }
    fs::rename(tmpPath, fullPath, ec);
    if (ec) {
        fs::remove(tmpPath, ec);
        return {};
    }

    return hash;
}

Result<std::string> BlobStorage::readBlob(const std::string& hash) {
    std::string fullPath = blobFullPath(hash);
    if (!fs::exists(fullPath)) {
        return Result<std::string>::Err(Error::NotFound);
    }

    std::ifstream in(fullPath, std::ios::binary);
    if (!in) {
        return Result<std::string>::Err(Error::IoError);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return Result<std::string>::Ok(ss.str());
}

bool BlobStorage::exists(const std::string& hash) {
    return fs::exists(blobFullPath(hash));
}

bool BlobStorage::deleteBlob(const std::string& hash) {
    std::error_code ec;
    return fs::remove(blobFullPath(hash), ec);
}

}  // namespace pureleaf
