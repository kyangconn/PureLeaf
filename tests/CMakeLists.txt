include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
)

# On Windows / MSVC we need to force gtest to link the CRT statically
# to match the rest of the project.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(pureleaf_tests
    test_smoke.cpp
    test_services.cpp
)

target_link_libraries(pureleaf_tests
    PRIVATE
        pureleaf_core
        pureleaf_capi
        pureleaf_platform_desktop
        GTest::gtest_main
)

gtest_discover_tests(pureleaf_tests)
