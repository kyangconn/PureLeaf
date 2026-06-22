#include "pureleaf/desktop/platform.h"

#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <shlobj.h>
#  include <windows.h>
#endif

namespace pureleaf::desktop {

namespace fs = std::filesystem;

pureleaf::Paths getPaths() {
    pureleaf::Paths paths;

    // ---- userDataDir ----
#ifdef _WIN32
    // Prefer %LOCALAPPDATA% (per user: data, not roaming).
    if (const char* appdata = std::getenv("LOCALAPPDATA")) {
        paths.userDataDir = (fs::path(appdata) / "pureleaf").string();
    } else {
        // Fallback via SHGetKnownFolderPath.
        PWSTR rawPath = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &rawPath))) {
            paths.userDataDir = (fs::path(rawPath) / "pureleaf").string();
            CoTaskMemFree(rawPath);
        }
    }
#else
    // XDG Base Directory Specification.
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg && xdg[0] == '/') {
        paths.userDataDir = (fs::path(xdg) / "pureleaf").string();
    } else if (const char* home = std::getenv("HOME")) {
        paths.userDataDir = (fs::path(home) / ".local/share/pureleaf").string();
    }
#endif

    // ---- tempDir ----
#ifdef _WIN32
    {
        char buf[MAX_PATH + 1] = {};
        if (GetTempPathA(sizeof(buf), buf) > 0) {
            paths.tempDir = buf;
        }
    }
#else
    if (const char* tmpdir = std::getenv("TMPDIR")) {
        paths.tempDir = tmpdir;
    } else {
        paths.tempDir = "/tmp";
    }
#endif

    return paths;
}

}  // namespace pureleaf::desktop
