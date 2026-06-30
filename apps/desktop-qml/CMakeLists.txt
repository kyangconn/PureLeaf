cmake_minimum_required(VERSION 3.21)
project(desktop-qml LANGUAGES CXX)

get_filename_component(PURELEAF_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# Qt Creator can open this CMakeLists.txt directly while the repository
# build reaches it through the root CMakeLists.txt. Supply the non-Qt
# project targets only when the parent build has not created them.
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

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Svg
    LinguistTools
)

# Qt PDF is optional in this spike (used later for the embedded PDF preview).
# Record availability rather than hard-failing — see todo.md §1 / §5.
find_package(Qt6 COMPONENTS Pdf)
if(Qt6Pdf_FOUND)
    message(STATUS "desktop-qml: Qt6::Pdf available; PDF preview will be linkable.")
else()
    message(STATUS "desktop-qml: Qt6::Pdf NOT found; PDF preview deferred.")
endif()

qt_standard_project_setup()

if(COMMAND qt_policy)
    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0004 NEW)
endif()

option(PURELEAF_USE_BUNDLED_QWINDOWKIT "Use bundled QWindowKit" ON)

if(NOT TARGET QWindowKit::Quick)
    if(PURELEAF_USE_BUNDLED_QWINDOWKIT)
        set(_qwindowkit_source_dir "${PURELEAF_ROOT_DIR}/third_party/qwindowkit")
        if(NOT EXISTS "${_qwindowkit_source_dir}/CMakeLists.txt")
            message(FATAL_ERROR
                "Bundled QWindowKit is missing. Initialize third_party/qwindowkit first."
            )
        endif()

        set(QWINDOWKIT_BUILD_WIDGETS OFF CACHE BOOL "" FORCE)
        set(QWINDOWKIT_BUILD_QUICK ON CACHE BOOL "" FORCE)
        set(QWINDOWKIT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(QWINDOWKIT_BUILD_DOCUMENTATIONS OFF CACHE BOOL "" FORCE)
        set(QWINDOWKIT_INSTALL OFF CACHE BOOL "" FORCE)

        add_subdirectory(
            "${_qwindowkit_source_dir}"
            "${CMAKE_CURRENT_BINARY_DIR}/qwindowkit"
        )
    else()
        find_package(QWindowKit CONFIG REQUIRED COMPONENTS Quick)
    endif()
endif()

# Content imported from Figma / Qt Design Studio, if present.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

qt_add_executable(desktop-qml
    main.cpp
    controller/desktopqmlcontroller.h
    controller/desktopqmlcontroller.cpp
    core/appsettings.h
    core/appsettings.cpp
)

qt_add_qml_module(desktop-qml
    URI PureLeaf.Desktop
    VERSION 1.0
    QML_FILES
        Main.qml
        shell/TitleBar.qml
        theme/Theme.qml
        ui/Icon.qml
        ui/PLButton.qml
        ui/PLCard.qml
        ui/PLComboBox.qml
        ui/PLIconButton.qml
        ui/PLSpinBox.qml
        ui/PLTextField.qml
        pages/HomePage.qml
        pages/EditorPage.qml
        pages/SettingsPage.qml
)

set(_pureleaf_lucide_icon_dir
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/lucide"
)
qt_add_resources(desktop-qml "pureleaf_qml_icons"
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

target_include_directories(desktop-qml
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(desktop-qml PRIVATE cxx_std_20)

target_link_libraries(desktop-qml
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickControls2
        Qt::Svg
        QWindowKit::Quick
        pureleaf_core
        pureleaf_platform_desktop
)

set_target_properties(desktop-qml PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
)

option(PURELEAF_DEPLOY_QT_AFTER_BUILD
    "Deploy the Qt runtime next to the desktop-qml executable after linking"
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

    add_custom_command(TARGET desktop-qml POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:desktop-qml>
            $<TARGET_FILE_DIR:desktop-qml>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying runtime DLLs for desktop-qml"
        VERBATIM
    )

    add_custom_command(TARGET desktop-qml POST_BUILD
        COMMAND "${PURELEAF_WINDEPLOYQT_EXECUTABLE}"
            --no-translations
            --qmldir "${CMAKE_CURRENT_SOURCE_DIR}"
            --dir "$<TARGET_FILE_DIR:desktop-qml>"
            "$<TARGET_FILE:desktop-qml>"
        COMMENT "Deploying the Qt runtime for desktop-qml"
        VERBATIM
    )
endif()

install(TARGETS desktop-qml
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
