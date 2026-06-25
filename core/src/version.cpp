#include "pureleaf/version.h"

#include <sstream>

namespace pureleaf {

// ── Build-time values from the generated header ───────────────

namespace {

// Compile-time constants (substituted by CMake).
constexpr int kMajor = @PROJECT_VERSION_MAJOR@;
constexpr int kMinor = @PROJECT_VERSION_MINOR@;
constexpr int kPatch = @PROJECT_VERSION_PATCH@;

// These are string literals embedded at CMake configure time.
constexpr const char* kGitHash      = "@PURELEAF_GIT_HASH@";
constexpr const char* kGitHashFull  = "@PURELEAF_GIT_HASH_FULL@";
constexpr const char* kGitTag       = "@PURELEAF_GIT_TAG@";
constexpr const char* kGitBranch    = "@PURELEAF_GIT_BRANCH@";
constexpr bool       kGitDirty      = @PURELEAF_GIT_DIRTY_BOOL@;

constexpr const char* kBuildType    = "@CMAKE_BUILD_TYPE@";
constexpr const char* kBuildTime    = "@PURELEAF_BUILD_TIME@";
constexpr const char* kCompiler     = "@PURELEAF_COMPILER_ID@ @PURELEAF_COMPILER_VERSION@";
constexpr const char* kPlatform     = "@PURELEAF_PLATFORM@";
constexpr const char* kChannel      = "@PURELEAF_UPDATE_CHANNEL@";

}  // anonymous namespace

// ── Singleton ─────────────────────────────────────────────────

static Version s_version;

const Version& getVersion() {
    return s_version;
}

// ── Version methods ───────────────────────────────────────────

std::string Version::displayString() const {
    std::ostringstream os;
    os << "PureLeaf " << semver;
    if (!gitHash.empty() && gitHash != "unknown") {
        os << " (" << gitHash << ")";
    }
    return os.str();
}

bool Version::isDevBuild() const {
    return updateChannel == "dev" || gitDirty;
}

bool Version::isReleaseBuild() const {
    return updateChannel == "stable";
}

// ── Static initializer (runs before main) ─────────────────────

namespace {

struct VersionInit {
    VersionInit() {
        s_version.major    = kMajor;
        s_version.minor    = kMinor;
        s_version.patch    = kPatch;
        s_version.full     = "@PURELEAF_VERSION_FULL@";
        s_version.semver   = "@PROJECT_VERSION@";
        s_version.gitHash      = kGitHash;
        s_version.gitHashFull  = kGitHashFull;
        s_version.gitTag       = kGitTag;
        s_version.gitBranch    = kGitBranch;
        s_version.gitDirty     = kGitDirty;
        s_version.buildType    = kBuildType;
        s_version.buildTime    = kBuildTime;
        s_version.compiler     = kCompiler;
        s_version.platform     = kPlatform;
        s_version.updateChannel = kChannel;
    }
};

static VersionInit g_versionInit;

}  // anonymous namespace

}  // namespace pureleaf
