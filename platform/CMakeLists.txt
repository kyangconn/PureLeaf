# Platform-specific implementations of core abstractions.
# Only the desktop platform is built by this CMake superbuild.
# HarmonyOS is built separately by DevEco Studio (hvigor).

if(PURELEAF_BUILD_QT)
    add_subdirectory(desktop)
endif()
