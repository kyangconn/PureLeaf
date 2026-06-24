#pragma once

#include <string>

namespace pureleaf {

/// Path validation utilities for safe file operations within a project root.
namespace path_util {

/// Validates that `name` is a safe single path component (no separators,
/// no `..`, no absolute paths, no null chars).
/// Returns true if safe.
bool isValidName(const std::string& name);

/// Validates that `relativePath` is safe within a project root:
/// - No `..` components
/// - No leading separator (not absolute)
/// - No null characters
/// Returns true if safe.
bool isSafeRelativePath(const std::string& relativePath);

/// Joins two path components using '/' (canonical forward-slash form).
std::string join(const std::string& a, const std::string& b);

/// Joins a filesystem root with a '/'-separated relative path,
/// returning a native filesystem path (handles \\?\ on Windows).
std::string joinNative(const std::string& root, const std::string& relativePath);

/// Returns the parent directory of a relative path, or empty if root.
std::string parent(const std::string& relativePath);

/// Returns the last component (filename) of a relative path.
std::string basename(const std::string& relativePath);

}  // namespace path_util

}  // namespace pureleaf
