#include "pureleaf/path_util.h"

#include <algorithm>
#include <cstring>
#include <filesystem>

namespace pureleaf::path_util {

bool isValidName(const std::string& name) {
    if (name.empty()) return false;
    if (name == "." || name == "..") return false;

    // Reject path separators and null chars.
    for (char c : name) {
        if (c == '/' || c == '\\' || c == '\0') return false;
    }

    // Reject Windows reserved names (CON, PRN, AUX, NUL, COM1-9, LPT1-9).
#ifdef _WIN32
    // Check case-insensitively.
    std::string upper;
    upper.reserve(name.size());
    for (char c : name) upper.push_back(static_cast<char>(toupper(c)));

    if (upper == "CON" || upper == "PRN" || upper == "AUX" || upper == "NUL") return false;
    if (upper.size() == 4 && (upper.compare(0, 3, "COM") == 0 || upper.compare(0, 3, "LPT") == 0) &&
        upper[3] >= '1' && upper[3] <= '9') {
        return false;
    }
#endif

    return true;
}

bool isSafeRelativePath(const std::string& relativePath) {
    if (relativePath.empty()) return true;  // root is safe

    // Reject leading separator (absolute path).
    if (relativePath[0] == '/' || relativePath[0] == '\\') return false;

    // Reject null chars.
    if (relativePath.find('\0') != std::string::npos) return false;

    // Check each component for ".." or empty segments.
    size_t start = 0;
    while (start <= relativePath.size()) {
        size_t end = relativePath.find('/', start);
        if (end == std::string::npos) end = relativePath.size();
        std::string segment = relativePath.substr(start, end - start);

        // Normalize backslashes too.
        size_t bs = segment.find('\\');
        if (bs != std::string::npos) {
            segment = segment.substr(0, bs);
        }

        if (segment.empty()) {
            // Consecutive separators — treat as unsafe (or skip on non-Windows).
            // Allow trailing slash.
            if (start == relativePath.size()) break;
            return false;
        }
        if (segment == "..") return false;

        start = end + 1;
    }

    return true;
}

std::string join(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    if (a.back() == '/') return a + b;
    return a + "/" + b;
}

std::string joinNative(const std::string& root, const std::string& relativePath) {
    namespace fs = std::filesystem;
    fs::path result = root;
    // Split on '/' to avoid mixed separators (critical for \\?\ paths on Windows).
    size_t start = 0;
    while (start < relativePath.size()) {
        size_t sep = relativePath.find('/', start);
        if (sep == std::string::npos) {
            result /= relativePath.substr(start);
            break;
        }
        if (sep > start) {
            result /= relativePath.substr(start, sep - start);
        }
        start = sep + 1;
    }
    return result.string();
}

std::string parent(const std::string& relativePath) {
    if (relativePath.empty()) return {};
    size_t pos = relativePath.find_last_of('/');
    if (pos == std::string::npos) return {};  // root level
    if (pos == 0) return {};                  // parent is root
    return relativePath.substr(0, pos);
}

std::string basename(const std::string& relativePath) {
    if (relativePath.empty()) return {};
    size_t pos = relativePath.find_last_of('/');
    if (pos == std::string::npos) return relativePath;
    return relativePath.substr(pos + 1);
}

}  // namespace pureleaf::path_util
