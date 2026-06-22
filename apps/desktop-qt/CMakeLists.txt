cmake_minimum_required(VERSION 3.19)
project(desktop-qt LANGUAGES CXX)

get_filename_component(PURELEAF_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# Qt Creator opens this CMakeLists.txt directly, while the repository build
# reaches it through the root CMakeLists.txt. Supply the non-Qt project targets
# only when the parent build has not already created them.
if(NOT TARGET pureleaf_core)
    add_subdirectory(
        "${PURELEAF_ROOT_DIR}/core"
        "${CMAKE_CURRENT_BINARY_DIR}/pureleaf-core"
    )
endif()

if(NOT TARGET pureleaf_platform_desktop)
    add_subdirectory(
        "${PURELEAF_ROOT_DIR}/platform/desktop"
        "${CMAKE_CURRENT_BINARY_DIR}/pureleaf-platform-desktop"
    )
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools)

qt_standard_project_setup()

option(PURELEAF_USE_BUNDLED_QWINDOWKIT "Use bundled QWindowKit" ON)

if(NOT TARGET QWindowKit::Widgets)
    if(PURELEAF_USE_BUNDLED_QWINDOWKIT)
        set(_qwindowkit_source_dir "${PURELEAF_ROOT_DIR}/third_party/qwindowkit")
        if(NOT EXISTS "${_qwindowkit_source_dir}/CMakeLists.txt")
            message(FATAL_ERROR
                "Bundled QWindowKit is missing. Initialize third_party/qwindowkit first."
            )
        endif()

        set(QWINDOWKIT_BUILD_WIDGETS ON CACHE BOOL "" FORCE)
        set(QWINDOWKIT_BUILD_QUICK OFF CACHE BOOL "" FORCE)
        set(QWINDOWKIT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(QWINDOWKIT_BUILD_DOCUMENTATIONS OFF CACHE BOOL "" FORCE)
        set(QWINDOWKIT_INSTALL OFF CACHE BOOL "" FORCE)

        add_subdirectory(
            "${_qwindowkit_source_dir}"
            "${CMAKE_CURRENT_BINARY_DIR}/qwindowkit"
        )
    else()
        find_package(QWindowKit CONFIG REQUIRED)
    endif()
endif()

qt_add_executable(desktop-qt
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    core/navigator.cpp
    core/navigator.h
    core/navpage.h
    pages/home/homepage.cpp
    pages/home/homepage.h
    pages/editor/editorpage.cpp
    pages/editor/editorpage.h
    pages/settings/settingspage.cpp
    pages/settings/settingspage.h
)

qt_add_translations(
    TARGETS desktop-qt
    TS_FILES desktop_zh_CN.ts
)

target_include_directories(desktop-qt
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(desktop-qt PRIVATE cxx_std_20)

target_link_libraries(desktop-qt
    PRIVATE
        Qt::Core
        Qt::Widgets
        QWindowKit::Widgets
        pureleaf_core
        pureleaf_platform_desktop
)

install(TARGETS desktop-qt
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET desktop-qt
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
