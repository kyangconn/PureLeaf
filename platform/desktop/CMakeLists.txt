add_library(pureleaf_platform_desktop STATIC
    src/platform.cpp
)

target_include_directories(pureleaf_platform_desktop
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pureleaf_platform_desktop
    PUBLIC
        pureleaf_core
)

target_compile_features(pureleaf_platform_desktop PUBLIC cxx_std_17)
