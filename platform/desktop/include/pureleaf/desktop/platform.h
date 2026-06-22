#pragma once

#include "pureleaf/platform.h"

namespace pureleaf::desktop {

/// Resolves platform-specific paths for Windows and Linux.
///
/// Windows: userDataDir = %LOCALAPPDATA%/pureleaf
/// Linux:   userDataDir = $XDG_DATA_HOME/pureleaf
///                       or ~/.local/share/pureleaf
pureleaf::Paths getPaths();

}  // namespace pureleaf::desktop
