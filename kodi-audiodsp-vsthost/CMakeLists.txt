cmake_minimum_required(VERSION 3.16)

project(audiodsp.vsthost VERSION 1.0.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Windows-only addon — skip the entire build on other platforms.
# ---------------------------------------------------------------------------
if(NOT WIN32)
    message(STATUS "audiodsp.vsthost: skipping build — Windows only")
    return()
endif()

# ---------------------------------------------------------------------------
# Global C++ standard
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# User-configurable paths
# ---------------------------------------------------------------------------
set(KODI_ADSP_SDK_DIR
    "C:/src/xbmc/xbmc/addons/kodi-addon-dev-kit/include/kodi"
    CACHE PATH
    "Path to the Kodi ADSP addon dev-kit include directory \
(xbmc/addons/kodi-addon-dev-kit/include/kodi)")

# ---------------------------------------------------------------------------
# Collect main plugin sources
# (src/ subdirectories: bridge, dsp, plugin, settings, util, vst2)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE PLUGIN_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)
list(FILTER PLUGIN_SOURCES EXCLUDE REGEX ".*/src/vst3/.*\\.cpp$")

# ---------------------------------------------------------------------------
# audiodsp.vsthost — the Kodi ADSP addon shared library
# ---------------------------------------------------------------------------
add_library(audiodsp.vsthost SHARED ${PLUGIN_SOURCES})

# Kodi addon shared libraries must not have a "lib" prefix on any platform.
set_target_properties(audiodsp.vsthost PROPERTIES PREFIX "")

target_compile_features(audiodsp.vsthost PRIVATE cxx_std_17)

target_include_directories(audiodsp.vsthost
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/include/kodi-legacy-adsp"
        "${KODI_ADSP_SDK_DIR}"
)

if(WIN32)
    set_target_properties(audiodsp.vsthost PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
    target_compile_definitions(audiodsp.vsthost PRIVATE
        NOMINMAX
        WIN32
        TARGET_WINDOWS
        UNICODE
        _UNICODE
    )
endif()

# ---------------------------------------------------------------------------
# vstscanner — standalone host-side scanner executable
# ---------------------------------------------------------------------------
if(EXISTS "${CMAKE_SOURCE_DIR}/scanner/vstscanner.cpp")
    add_executable(vstscanner
        "${CMAKE_SOURCE_DIR}/scanner/vstscanner.cpp"
    )

    target_compile_features(vstscanner PRIVATE cxx_std_17)

    target_include_directories(vstscanner
        PRIVATE
            "${CMAKE_SOURCE_DIR}/src"
    )

    if(WIN32)
        target_compile_definitions(vstscanner PRIVATE
            NOMINMAX
            WIN32
            UNICODE
            _UNICODE
        )
    endif()
else()
    message(STATUS
        "audiodsp.vsthost: scanner/vstscanner.cpp not found — "
        "vstscanner target skipped (will be added when the file is created)")
endif()

# ---------------------------------------------------------------------------
# Installation (optional — place next to Kodi addon tree)
# ---------------------------------------------------------------------------
install(TARGETS audiodsp.vsthost
    LIBRARY DESTINATION .
    RUNTIME DESTINATION .
)

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
message(STATUS "------------------------------------------------------------")
message(STATUS "audiodsp.vsthost build configuration")
message(STATUS "  Version          : ${PROJECT_VERSION}")
message(STATUS "  C++ standard     : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Kodi ADSP SDK    : ${KODI_ADSP_SDK_DIR}")
message(STATUS "  Platform         : ${CMAKE_SYSTEM_NAME}")
message(STATUS "------------------------------------------------------------")
