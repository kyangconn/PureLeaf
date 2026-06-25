#[=======================================================================[
 PureLeaf Git / version detection module.

 Detects Git metadata (hash, tag, branch, dirty flag) and sets variables
 consumed by core/CMakeLists.txt to generate a version header.

 Environment-based overrides (for CI / package builds):
   PURELEAF_VERSION_SUFFIX    — appended to version (e.g. "-ci", "-rc1")
   PURELEAF_UPDATE_CHANNEL    — "stable", "beta", or "dev"
   PURELEAF_GIT_HASH          — override the auto-detected short hash
   PURELEAF_GIT_TAG           — override the auto-detected tag
#]=======================================================================]

# ── Version suffix & update channel ─────────────────────────────
# Local dev builds default to -dev; CI sets its own via env.

if(DEFINED ENV{PURELEAF_UPDATE_CHANNEL})
    set(PURELEAF_UPDATE_CHANNEL "$ENV{PURELEAF_UPDATE_CHANNEL}")
elseif(DEFINED ENV{CI})
    set(PURELEAF_UPDATE_CHANNEL "ci")
else()
    set(PURELEAF_UPDATE_CHANNEL "dev")
endif()

if(PURELEAF_UPDATE_CHANNEL STREQUAL "stable")
    set(PURELEAF_VERSION_SUFFIX "")
elseif(DEFINED ENV{PURELEAF_VERSION_SUFFIX})
    set(PURELEAF_VERSION_SUFFIX "$ENV{PURELEAF_VERSION_SUFFIX}")
else()
    set(PURELEAF_VERSION_SUFFIX "-${PURELEAF_UPDATE_CHANNEL}")
endif()

# ── Git metadata ────────────────────────────────────────────────
# All execute_process() calls tolerate missing Git (e.g. tarball builds).

find_package(Git QUIET)

if(GIT_FOUND)
    # Short commit hash.
    if(DEFINED ENV{PURELEAF_GIT_HASH})
        set(PURELEAF_GIT_HASH "$ENV{PURELEAF_GIT_HASH}")
    else()
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse --short=9 HEAD
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE PURELEAF_GIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    endif()

    # Full commit hash.
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE PURELEAF_GIT_HASH_FULL
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Latest tag reachable from HEAD.
    if(DEFINED ENV{PURELEAF_GIT_TAG})
        set(PURELEAF_GIT_TAG "$ENV{PURELEAF_GIT_TAG}")
    else()
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" describe --tags --abbrev=0 2>/dev/null
            COMMAND "${GIT_EXECUTABLE}" describe --tags --always
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE PURELEAF_GIT_TAG
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    endif()

    # Branch name.
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE PURELEAF_GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Dirty working tree.
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" diff --quiet
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _git_dirty_ret
        ERROR_QUIET
    )
    if(_git_dirty_ret EQUAL 0)
        set(PURELEAF_GIT_DIRTY "false")
    else()
        set(PURELEAF_GIT_DIRTY "true")
    endif()
else()
    # No Git available — provide fallback values.
    set(PURELEAF_GIT_HASH       "unknown")
    set(PURELEAF_GIT_HASH_FULL  "unknown")
    set(PURELEAF_GIT_TAG        "v${PROJECT_VERSION}")
    set(PURELEAF_GIT_BRANCH     "unknown")
    set(PURELEAF_GIT_DIRTY      "false")
endif()

# ── Build metadata ──────────────────────────────────────────────
string(TIMESTAMP PURELEAF_BUILD_TIME "%Y-%m-%dT%H:%M:%SZ" UTC)

set(PURELEAF_COMPILER_ID "${CMAKE_CXX_COMPILER_ID}")
set(PURELEAF_COMPILER_VERSION "${CMAKE_CXX_COMPILER_VERSION}")

if(WIN32)
    set(PURELEAF_PLATFORM "Windows")
elseif(APPLE)
    set(PURELEAF_PLATFORM "macOS")
else()
    set(PURELEAF_PLATFORM "Linux")
endif()

# ── Full version string ─────────────────────────────────────────
# Format: MAJOR.MINOR.PATCH[-SUFFIX][+HASH][.dirty]
set(PURELEAF_VERSION_FULL "${PROJECT_VERSION}${PURELEAF_VERSION_SUFFIX}")
if(PURELEAF_GIT_HASH AND NOT PURELEAF_GIT_HASH STREQUAL "unknown" AND NOT PURELEAF_VERSION_SUFFIX STREQUAL "")
    set(PURELEAF_VERSION_FULL "${PURELEAF_VERSION_FULL}+${PURELEAF_GIT_HASH}")
endif()
if(PURELEAF_GIT_DIRTY STREQUAL "true")
    set(PURELEAF_VERSION_FULL "${PURELEAF_VERSION_FULL}.dirty")
endif()

message(STATUS "PureLeaf version: ${PURELEAF_VERSION_FULL}  "
               "(channel=${PURELEAF_UPDATE_CHANNEL}, "
               "git=${PURELEAF_GIT_HASH}, "
               "dirty=${PURELEAF_GIT_DIRTY})")
