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

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Svg Widgets LinguistTools)

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
    components/icons/appicon.cpp
    components/icons/appicon.h
    core/appsettings.cpp
    core/appsettings.h
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

set(_pureleaf_lucide_icon_dir
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/lucide"
)
qt_add_resources(desktop-qt "pureleaf_icons"
    PREFIX "/pureleaf/icons/lucide"
    BASE "${_pureleaf_lucide_icon_dir}"
    FILES
        "${_pureleaf_lucide_icon_dir}/arrow-left.svg"
        "${_pureleaf_lucide_icon_dir}/chevron-down.svg"
        "${_pureleaf_lucide_icon_dir}/copy.svg"
        "${_pureleaf_lucide_icon_dir}/ellipsis.svg"
        "${_pureleaf_lucide_icon_dir}/file-plus.svg"
        "${_pureleaf_lucide_icon_dir}/folder-open.svg"
        "${_pureleaf_lucide_icon_dir}/git-branch.svg"
        "${_pureleaf_lucide_icon_dir}/leaf.svg"
        "${_pureleaf_lucide_icon_dir}/minus.svg"
        "${_pureleaf_lucide_icon_dir}/settings.svg"
        "${_pureleaf_lucide_icon_dir}/square.svg"
        "${_pureleaf_lucide_icon_dir}/trash-2.svg"
        "${_pureleaf_lucide_icon_dir}/x.svg"
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
        Qt::Svg
        Qt::Widgets
        QWindowKit::Widgets
        pureleaf_core
        pureleaf_platform_desktop
)

option(PURELEAF_DEPLOY_QT_AFTER_BUILD
    "Deploy the Qt runtime next to the Windows desktop executable after linking"
    ON
)

if(WIN32 AND PURELEAF_DEPLOY_QT_AFTER_BUILD)
    get_target_property(_qt_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qt_qmake_executable}" DIRECTORY)
    find_program(PURELEAF_WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${_qt_bin_dir}"
        NO_DEFAULT_PATH
        REQUIRED
    )

    add_custom_command(TARGET desktop-qt POST_BUILD
        COMMAND "${PURELEAF_WINDEPLOYQT_EXECUTABLE}"
            --no-translations
            --dir "$<TARGET_FILE_DIR:desktop-qt>"
            "$<TARGET_FILE:desktop-qt>"
        COMMENT "Deploying the Qt runtime for desktop-qt"
        VERBATIM
    )
endif()

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
