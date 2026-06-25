cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED PURELEAF_FORMAT_MODE OR PURELEAF_FORMAT_MODE STREQUAL "")
    set(PURELEAF_FORMAT_MODE fix)
endif()

if(NOT PURELEAF_FORMAT_MODE MATCHES "^(fix|check)$")
    message(FATAL_ERROR "PURELEAF_FORMAT_MODE must be 'fix' or 'check'")
endif()

set(_clang_format_names clang-format clang-format-20 clang-format-19 clang-format-18 clang-format-17
                        clang-format-16 clang-format-15)

set(_clang_format_hints)
if(WIN32)
    file(GLOB _vs_llvm_bin_dirs
         "C:/Program Files/Microsoft Visual Studio/*/*/VC/Tools/Llvm/x64/bin"
         "C:/Program Files/Microsoft Visual Studio/*/*/VC/Tools/Llvm/bin")
    list(APPEND _clang_format_hints ${_vs_llvm_bin_dirs})
endif()

if(DEFINED CLANG_FORMAT_EXE AND NOT CLANG_FORMAT_EXE STREQUAL "")
    if(EXISTS "${CLANG_FORMAT_EXE}")
        set(_clang_format "${CLANG_FORMAT_EXE}")
    else()
        find_program(_clang_format NAMES "${CLANG_FORMAT_EXE}" ${_clang_format_names}
                     HINTS ${_clang_format_hints})
    endif()
else()
    find_program(_clang_format NAMES ${_clang_format_names} HINTS ${_clang_format_hints})
endif()

if(NOT _clang_format)
    message(FATAL_ERROR "clang-format not found. Install clang-format or pass CLANG_FORMAT=/path/to/clang-format.")
endif()

get_filename_component(_repo_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(_format_roots
    apps/desktop-qt
    capi
    core
    platform
    tests
)

set(_format_patterns
    *.c
    *.cc
    *.cpp
    *.cxx
    *.h
    *.hh
    *.hpp
    *.hxx
)

set(_format_files)
foreach(_root IN LISTS _format_roots)
    foreach(_pattern IN LISTS _format_patterns)
        file(GLOB_RECURSE _matches "${_repo_root}/${_root}/${_pattern}")
        list(APPEND _format_files ${_matches})
    endforeach()
endforeach()

list(REMOVE_DUPLICATES _format_files)

set(_excluded_path_regexes
    "(^|/)(build|out|CMakeFiles|_deps)(/|$)"
    "(^|/)\\.qt(/|$)"
    "(^|/)\\.qtcreator(/|$)"
    "(^|/)cmake-build-[^/]+(/|$)"
)

set(_filtered_format_files)
foreach(_file IN LISTS _format_files)
    file(RELATIVE_PATH _relative_file "${_repo_root}" "${_file}")
    set(_is_excluded OFF)
    foreach(_regex IN LISTS _excluded_path_regexes)
        if(_relative_file MATCHES "${_regex}")
            set(_is_excluded ON)
        endif()
    endforeach()

    if(NOT _is_excluded)
        list(APPEND _filtered_format_files "${_file}")
    endif()
endforeach()

set(_format_files ${_filtered_format_files})
list(SORT _format_files)
list(LENGTH _format_files _format_file_count)

if(_format_file_count EQUAL 0)
    message(STATUS "No C/C++ files found to format")
    return()
endif()

if(PURELEAF_FORMAT_MODE STREQUAL "check")
    set(_format_args --dry-run --Werror)
else()
    set(_format_args -i)
endif()

set(_format_failed OFF)
foreach(_file IN LISTS _format_files)
    execute_process(
        COMMAND "${_clang_format}" ${_format_args} "${_file}"
        WORKING_DIRECTORY "${_repo_root}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _stdout
        ERROR_VARIABLE _stderr
    )

    if(NOT _result EQUAL 0)
        file(RELATIVE_PATH _relative_file "${_repo_root}" "${_file}")
        message(SEND_ERROR "clang-format failed for ${_relative_file}\n${_stdout}${_stderr}")
        set(_format_failed ON)
    endif()
endforeach()

if(_format_failed)
    message(FATAL_ERROR "clang-format reported errors")
endif()

if(PURELEAF_FORMAT_MODE STREQUAL "check")
    message(STATUS "Formatting check passed for ${_format_file_count} files")
else()
    message(STATUS "Formatted ${_format_file_count} files")
endif()
