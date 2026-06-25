cmake_minimum_required(VERSION 3.21)

project(PureLeaf
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clangd / IDE IntelliSense.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# MSVC: treat source files as UTF-8 to avoid C4819 code-page warnings.
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Core targets C++17 internally (see core/CMakeLists.txt) for HarmonyOS
# portability; the app layer may use C++20.

option(PURELEAF_BUILD_QT "Build PureLeaf Qt desktop app" ON)
option(PURELEAF_BUILD_TESTS "Build unit tests (requires network for FetchContent)" OFF)
option(PURELEAF_USE_BUNDLED_QWINDOWKIT "Use bundled QWindowKit" ON)

# Detect Git metadata and set PURELEAF_* variables for version embedding.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GetGitInfo)

add_subdirectory(third_party/sqlite3)
add_subdirectory(core)
add_subdirectory(capi)
add_subdirectory(platform)

if(PURELEAF_BUILD_QT)
    add_subdirectory(apps/desktop-qt)
endif()

if(PURELEAF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
