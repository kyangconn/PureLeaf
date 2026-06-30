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

option(PURELEAF_BUILD_QML "Build PureLeaf Qt Quick desktop app" ON)
option(PURELEAF_BUILD_TESTS "Build unit tests (requires network for FetchContent)" OFF)
option(PURELEAF_USE_BUNDLED_QWINDOWKIT "Use bundled QWindowKit" ON)

# Detect Git metadata and set PURELEAF_* variables for version embedding.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GetGitInfo)

add_subdirectory(third_party/sqlite3)
add_subdirectory(core)
add_subdirectory(capi)
add_subdirectory(platform)

if(PURELEAF_BUILD_QML)
    if(PURELEAF_USE_BUNDLED_QWINDOWKIT)
        set(_qwindowkit_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/third_party/qwindowkit")
        if(NOT EXISTS "${_qwindowkit_source_dir}/CMakeLists.txt")
            message(FATAL_ERROR
                "Bundled QWindowKit is missing. Initialize third_party/qwindowkit first."
            )
        endif()

        set(QWINDOWKIT_BUILD_WIDGETS OFF CACHE BOOL "" FORCE)
        set(QWINDOWKIT_BUILD_QUICK ${PURELEAF_BUILD_QML} CACHE BOOL "" FORCE)
        set(QWINDOWKIT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(QWINDOWKIT_BUILD_DOCUMENTATIONS OFF CACHE BOOL "" FORCE)
        set(QWINDOWKIT_INSTALL OFF CACHE BOOL "" FORCE)

        add_subdirectory("${_qwindowkit_source_dir}" "${CMAKE_CURRENT_BINARY_DIR}/qwindowkit")
    else()
        find_package(QWindowKit CONFIG REQUIRED COMPONENTS Core Quick)
    endif()
endif()

if(PURELEAF_BUILD_QML)
    add_subdirectory(apps/desktop-qml)
endif()

if(PURELEAF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
