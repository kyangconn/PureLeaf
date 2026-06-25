# Convert PURELEAF_GIT_DIRTY to a C++ boolean literal for version.cpp.in.
if(PURELEAF_GIT_DIRTY STREQUAL "true")
    set(PURELEAF_GIT_DIRTY_BOOL "true")
else()
    set(PURELEAF_GIT_DIRTY_BOOL "false")
endif()

# Generate version.cpp from template (embeds build-time constants).
set(_version_cpp_in  "${CMAKE_CURRENT_SOURCE_DIR}/src/version.cpp.in")
set(_version_cpp_out "${CMAKE_CURRENT_BINARY_DIR}/version.cpp")
configure_file("${_version_cpp_in}" "${_version_cpp_out}" @ONLY)

add_library(pureleaf_core STATIC
    src/storage.cpp
    src/diff.cpp
    src/synctex.cpp
    src/database.cpp
    src/repo.cpp
    src/file_repo.cpp
    src/project_service.cpp
    src/file_service.cpp
    src/lock_manager.cpp
    src/path_util.cpp
    "${_version_cpp_out}"
)

target_include_directories(pureleaf_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/sha256
)

# Core targets C++17 for maximum portability (HarmonyOS BiSheng / MSVC / GCC).
target_compile_features(pureleaf_core PUBLIC cxx_std_17)

target_link_libraries(pureleaf_core
    PUBLIC
        sqlite3
)

# On MSVC, Qt already sets /W3 via qt_standard_project_setup(). We
# suppress noisy CRT deprecations instead of fighting /W3 with /W4
# (which triggers D9025).
if(MSVC)
    target_compile_definitions(pureleaf_core PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(pureleaf_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
