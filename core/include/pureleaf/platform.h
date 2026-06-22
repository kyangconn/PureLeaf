#pragma once

#include <string>

namespace pureleaf {

/// Platform-specific path configuration resolved by the platform layer
/// and injected into core services. Core itself never queries the OS.
struct Paths {
    /// Root data directory for the application.
    /// Windows: %LOCALAPPDATA%/pureleaf
    /// Linux:   $XDG_DATA_HOME/pureleaf or ~/.local/share/pureleaf
    std::string userDataDir;

    /// System temp directory.
    std::string tempDir;
};

}  // namespace pureleaf
