# SQLite amalgamation as a static library.
#
# Downloads the official amalgamation (sqlite3.c + sqlite3.h) at configure
# time. This is the recommended way to embed SQLite — a single C translation
# unit that compiles everywhere (MSVC, GCC, Clang, BiSheng).

include(FetchContent)

# Use NEW behavior for URL download timestamps (CMP0135).
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Find the latest amalgamation URL at https://www.sqlite.org/download.html
FetchContent_Declare(
    sqlite3_src
    URL      https://www.sqlite.org/2026/sqlite-amalgamation-3530200.zip
    URL_HASH SHA3_256=81142986038e18f96c4a54e1a72562ae17e502a916f2a7701eff43388cbf1a40
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_GetProperties(sqlite3_src)
if(NOT sqlite3_src_POPULATED)
    FetchContent_Populate(sqlite3_src)
endif()

add_library(sqlite3 STATIC ${sqlite3_src_SOURCE_DIR}/sqlite3.c)

target_include_directories(sqlite3 PUBLIC ${sqlite3_src_SOURCE_DIR})

# Enable FTS5 (full-text search) and thread safety.
target_compile_definitions(sqlite3 PRIVATE
    SQLITE_ENABLE_FTS5
    SQLITE_THREADSAFE=1
    SQLITE_DEFAULT_MEMSTATUS=0
    SQLITE_DEFAULT_WAL_SYNCHRONOUS=1
)

# sqlite3.c is C, not C++.
set_target_properties(sqlite3 PROPERTIES LINKER_LANGUAGE C)
