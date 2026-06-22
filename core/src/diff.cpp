#include "pureleaf/diff.h"

namespace pureleaf {

std::vector<DiffHunk> computeDiff(const std::string& oldText, const std::string& newText) {
    // TODO: port the Myers diff from the previous Go implementation.
    // The Go version had a subtlety: pure-insertion hunks need the anchor
    // at op.i1, not just deletion markers in changed[].
    (void)oldText;
    (void)newText;
    return {};
}

}  // namespace pureleaf
