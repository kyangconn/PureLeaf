add_library(pureleaf_core STATIC
    src/storage.cpp
    src/diff.cpp
    src/synctex.cpp
    src/database.cpp
    src/repo.cpp
)

target_include_directories(pureleaf_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
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
