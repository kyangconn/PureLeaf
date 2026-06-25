#pragma once

/// @file pureleaf_c.h
/// C ABI wrapper around pureleaf_core.
///
/// Exposes stable C-linkage symbols so that HarmonyOS NAPI, Python ctypes,
/// or any other FFI consumer can call core without dealing with the C++
/// ABI (which differs between MSVC and BiSheng/clang).

#ifdef __cplusplus
extern "C" {
#endif

/// Library version string (static storage, do not free).
const char* pl_version(void);

/// Full version string with build metadata (e.g. "0.1.0-dev+abc1234").
const char* pl_version_full(void);

/// Build metadata: git hash@branch, compiler, platform.
const char* pl_build_info(void);

/// Update channel: "stable", "beta", "ci", or "dev".
const char* pl_update_channel(void);

/* ---- SyncTeX --------------------------------------------------------- */

typedef struct {
    int page;
    double x;
    double y;
} pl_synctex_forward;

/// Converts synctex big-point coordinates to pdf.js CSS px coordinates.
pl_synctex_forward pl_synctex_to_pdfjs(int page, double x, double y, double scale);

/* ---- Storage --------------------------------------------------------- */

/// Computes the relative blob storage path for a SHA-256 hash.
/// Returns a pointer to static/thread-local storage; copy before next call.
const char* pl_blob_relative_path(const char* hash);

#ifdef __cplusplus
}  // extern "C"
#endif
