add_library(pureleaf_capi STATIC
    src/pureleaf_c.cpp
)

target_include_directories(pureleaf_capi
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pureleaf_capi
    PUBLIC
        pureleaf_core
)

# The C API must be built as C, not C++, for ABI stability.
set_target_properties(pureleaf_capi PROPERTIES
    LINKER_LANGUAGE CXX
)
