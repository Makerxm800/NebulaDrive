cmake_minimum_required(VERSION 3.22)

project(NebulaDrive
    VERSION 1.1.0
    DESCRIPTION "NebulaDrive — your private cloud. No servers, no subscriptions."
    LANGUAGES C CXX
)

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Static-ish self-contained builds for the GUI exe (libgcc/libstdc++ linked in).
option(NEBULADRIVE_STATIC "Link the C/C++ runtime statically (GUI build)" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default 64-bit check — a 32-bit toolchain produces a 32-bit exe.
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(WARNING "This is a 32-bit build. Switch to a 64-bit toolchain (e.g. mingw64/ucrt64).")
endif()

# ---------------------------------------------------------------------------
# Shared: database + storage (both platforms)
# ---------------------------------------------------------------------------

add_library(nebuladrive_core STATIC
    database.cpp
    storage_handler.cpp
)

target_include_directories(nebuladrive_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(nebuladrive_core PRIVATE vendor/sqlite)

target_compile_definitions(nebuladrive_core PRIVATE
    UNICODE
    _UNICODE
    SQLITE_THREADSAFE=1
    SQLITE_DQS=0
)

if(MSVC)
    target_compile_options(nebuladrive_core PRIVATE /W4 /utf-8)
else()
    target_compile_options(nebuladrive_core PRIVATE -Wall -Wextra)
endif()

# Vendored SQLite3 amalgamation (no network access needed at build time).
add_library(nebuladrive_sqlite STATIC vendor/sqlite/sqlite3.c)
target_include_directories(nebuladrive_sqlite PUBLIC vendor/sqlite)
if(MSVC)
    target_compile_options(nebuladrive_sqlite PRIVATE /w)
else()
    target_compile_options(nebuladrive_sqlite PRIVATE -w) # vendored third-party code
endif()

target_link_libraries(nebuladrive_core PUBLIC nebuladrive_sqlite)

# ---------------------------------------------------------------------------
# CLI executable — every platform (Linux, macOS, Windows)
# ---------------------------------------------------------------------------

add_executable(NebulaDriveCLI main.cpp)
target_link_libraries(NebulaDriveCLI PRIVATE nebuladrive_core)

# ---------------------------------------------------------------------------
# GUI executable — Windows only (Win32 API + GDI+)
# ---------------------------------------------------------------------------

if(WIN32)
    enable_language(RC)

    add_executable(NebulaDrive WIN32
        gui_app.cpp
        nebula_ui.cpp
        NebulaDrive.rc
    )

    target_link_libraries(NebulaDrive PRIVATE
        nebuladrive_core
        comctl32
        gdi32
        gdiplus
        user32
        shell32
        ole32
        shlwapi
        winmm
    )

    target_compile_definitions(NebulaDrive PRIVATE UNICODE _UNICODE)

    # A self-contained executable that runs without MinGW installed.
    if(NEBULADRIVE_STATIC AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set_target_properties(NebulaDrive PROPERTIES
            LINK_FLAGS "-static -static-libgcc -static-libstdc++"
        )
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
            target_link_libraries(NebulaDrive PRIVATE stdc++fs)
        endif()
    elseif(MSVC)
        set_property(TARGET NebulaDrive PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    if(MSVC)
        target_compile_options(NebulaDrive PRIVATE /W4 /utf-8)
    else()
        target_compile_options(NebulaDrive PRIVATE -Wall -Wextra)
    endif()
endif()

# ---------------------------------------------------------------------------
# Install rules
# ---------------------------------------------------------------------------

install(TARGETS NebulaDriveCLI RUNTIME DESTINATION bin)
if(WIN32)
    install(TARGETS NebulaDrive RUNTIME DESTINATION bin)
endif()

# ---------------------------------------------------------------------------
# CPack — one-click installer / portable package (optional)
# ---------------------------------------------------------------------------

set(CPACK_PACKAGE_NAME "NebulaDrive")
set(CPACK_PACKAGE_VENDOR "NebulaDrive")
set(CPACK_PACKAGE_VERSION "1.1.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NebulaDrive — your private cloud. No servers, no subscriptions.")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "NebulaDrive")
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "NebulaDrive-${CMAKE_PROJECT_VERSION}")
include(CPack)
