#include <string>

#include "pureleaf/storage.h"
#include "pureleaf/synctex.h"
#include "pureleaf/version.h"
#include "pureleaf_c/pureleaf.h"

extern "C" {

const char* pl_version(void) {
    static const std::string s = std::to_string(pureleaf::getVersion().major) + "." +
                                 std::to_string(pureleaf::getVersion().minor) + "." +
                                 std::to_string(pureleaf::getVersion().patch);
    return s.c_str();
}

const char* pl_version_full(void) {
    return pureleaf::getVersion().full.c_str();
}

const char* pl_build_info(void) {
    static const std::string s =
        pureleaf::getVersion().gitHash + "@" + pureleaf::getVersion().gitBranch + ", " +
        pureleaf::getVersion().compiler + ", " + pureleaf::getVersion().platform;
    return s.c_str();
}

const char* pl_update_channel(void) {
    return pureleaf::getVersion().updateChannel.c_str();
}

pl_synctex_forward pl_synctex_to_pdfjs(int page, double x, double y, double scale) {
    auto r = pureleaf::synctexToPdfjs(page, x, y, scale);
    return {r.page, r.x, r.y};
}

const char* pl_blob_relative_path(const char* hash) {
    if (!hash) return "";
    static thread_local std::string buf;
    buf = pureleaf::BlobStorage::blobRelativePath(hash);
    return buf.c_str();
}

}  // extern "C"
