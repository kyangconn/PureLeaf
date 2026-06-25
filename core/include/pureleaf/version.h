#pragma once

/// @file version.h
/// PureLeaf version metadata, available to all layers (core, capi, UI).

#include <cstdint>
#include <string>

namespace pureleaf {

struct Version {
    // ── Semantic version ──────────────────────────────────
    int major = 0;
    int minor = 0;
    int patch = 0;

    // ── Full version strings ──────────────────────────────
    std::string full;    ///< e.g. "0.1.0-dev+abc1234.dirty"
    std::string semver;  ///< e.g. "0.1.0"

    // ── Git metadata ──────────────────────────────────────
    std::string gitHash;      ///< Short hash (9 chars), or "unknown".
    std::string gitHashFull;  ///< Full SHA-1 hash.
    std::string gitTag;       ///< Most recent tag, or "unknown".
    std::string gitBranch;    ///< Branch name, or "unknown".
    bool gitDirty = false;    ///< Uncommitted changes present.

    // ── Build metadata ────────────────────────────────────
    std::string buildType;  ///< "Debug", "Release", "RelWithDebInfo".
    std::string buildTime;  ///< ISO 8601 UTC timestamp.
    std::string compiler;   ///< e.g. "MSVC 19.42.34433".
    std::string platform;   ///< "Windows", "Linux", "macOS".

    // ── Update check ──────────────────────────────────────
    std::string updateChannel;  ///< "stable", "beta", "ci", "dev".

    // ── Helpers ───────────────────────────────────────────
    /// "PureLeaf 0.1.0 (abc1234)" for UI display.
    std::string displayString() const;

    /// True when updateChannel == "dev" or dirty.
    bool isDevBuild() const;

    /// True when updateChannel == "stable".
    bool isReleaseBuild() const;
};

/// Singleton accessor — safe to call from any thread after main().
const Version& getVersion();

}  // namespace pureleaf
