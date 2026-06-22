cmake_minimum_required(VERSION 3.5)

project(PureLeaf
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(PURELEAF_BUILD_QT "Build PureLeaf Qt desktop app" ON)
option(PURELEAF_BUILD_TESTS "Build tests" ON)

add_subdirectory(core)
add_subdirectory(capi)
add_subdirectory(platform)

if(PURELEAF_BUILD_QT)
    add_subdirectory(apps/desktop-qt)
endif()