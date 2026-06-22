#pragma once

#include <string>
#include <vector>

namespace pureleaf {

/// A contiguous block of changes produced by the diff algorithm.
struct DiffHunk {
    int oldStart = 0;  ///< 1-based start line in old text.
    int oldLines = 0;
    int newStart = 0;  ///< 1-based start line in new text.
    int newLines = 0;
    std::vector<std::string> oldContent;
    std::vector<std::string> newContent;
};

/// Computes a line-level diff between two texts.
/// TODO: implement Myers diff (ported from the previous Go implementation).
std::vector<DiffHunk> computeDiff(const std::string& oldText, const std::string& newText);

}  // namespace pureleaf
