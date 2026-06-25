#pragma once

#include <string>

namespace pureleaf {

/// SyncTeX uses 72-dpi big points; pdf.js uses 96-dpi CSS pixels.
/// Both directions apply the ratio and the current zoom scale.
constexpr double kSynctexDpi = 72.0;
constexpr double kPdfjsDpi = 96.0;
constexpr double kSynctexPdfjsRatio = kSynctexDpi / kPdfjsDpi;  ///< 0.75

struct SyncTexForward {
    int page = 0;
    double x = 0.0;  ///< CSS px
    double y = 0.0;  ///< CSS px
};

struct SyncTexReverse {
    std::string file;
    int line = 0;
    int column = 0;
};

/// Converts a SyncTeX forward-search hit (in synctex big points) to
/// pdf.js canvas coordinates, accounting for zoom.
inline SyncTexForward synctexToPdfjs(int page, double synctexX, double synctexY, double scale) {
    return {page, synctexX * kSynctexPdfjsRatio * scale, synctexY * kSynctexPdfjsRatio * scale};
}

}  // namespace pureleaf
