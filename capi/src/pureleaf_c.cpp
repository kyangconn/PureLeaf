#include "pureleaf_c/pureleaf.h"

#include "pureleaf/storage.h"
#include "pureleaf/synctex.h"

#include <string>

extern "C" {

const char* pl_version(void) {
    return "0.1.0";
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
