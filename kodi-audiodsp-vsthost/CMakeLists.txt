cmake_minimum_required(VERSION 3.16)

project(audiodsp.vsthost VERSION 1.0.0 LANGUAGES CXX)

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
# Locate the VST3 SDK
# Strategy 1: local override — deps/vst3sdk already populated
# Strategy 2: FetchContent download
# ---------------------------------------------------------------------------
if(EXISTS "${CMAKE_SOURCE_DIR}/deps/vst3sdk/CMakeLists.txt")
    message(STATUS "audiodsp.vsthost: using local deps/vst3sdk")
    set(VST3SDK_DIR "${CMAKE_SOURCE_DIR}/deps/vst3sdk")
else()
    message(STATUS "audiodsp.vsthost: fetching vst3sdk via FetchContent")
    include(FetchContent)

    FetchContent_Declare(vst3sdk
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_TAG        v3.7.9_build_61
        GIT_SUBMODULES "base;pluginterfaces;public.sdk"
        GIT_SHALLOW    TRUE
    )
    FetchContent_GetProperties(vst3sdk)
    if(NOT vst3sdk_POPULATED)
        FetchContent_Populate(vst3sdk)
    endif()
    set(VST3SDK_DIR "${vst3sdk_SOURCE_DIR}")
endif()

message(STATUS "audiodsp.vsthost: VST3SDK_DIR = ${VST3SDK_DIR}")

# ---------------------------------------------------------------------------
# vst3_hosting — static library: only the hosting subset of the VST3 SDK
# Wrapped in an existence guard so a CMake configure-only pass (no submodule)
# does not abort with a missing-file error; the guard is rechecked at
# generate time when FetchContent has already populated the directory.
# ---------------------------------------------------------------------------
set(VST3_HOSTING_SOURCES
    "${VST3SDK_DIR}/public.sdk/source/vst/hosting/module.cpp"
    "${VST3SDK_DIR}/public.sdk/source/vst/hosting/module_win32.cpp"
    "${VST3SDK_DIR}/public.sdk/source/vst/hosting/plugprovider.cpp"
    "${VST3SDK_DIR}/public.sdk/source/vst/hosting/pluginterfacesupport.cpp"
    "${VST3SDK_DIR}/public.sdk/source/vst/hosting/hostclasses.cpp"
    "${VST3SDK_DIR}/public.sdk/source/vst/hosting/processdata.cpp"
    "${VST3SDK_DIR}/public.sdk/source/vst/hosting/parameterchanges.cpp"
    "${VST3SDK_DIR}/public.sdk/source/vst/hosting/eventlist.cpp"
    "${VST3SDK_DIR}/public.sdk/source/vst/hosting/connectionproxy.cpp"
    # Defines DEF_CLASS_IID for all Vst::* interfaces (IComponent,
    # IEditController, IAudioProcessor, IHostApplication, IConnectionPoint,
    # IParameterChanges, IParamValueQueue, ITestPlugProvider, ...)
    "${VST3SDK_DIR}/public.sdk/source/vst/vstinitiids.cpp"
    # VST3::StringConvert::convert (char16 <-> UTF-8) used by module_win32.cpp
    "${VST3SDK_DIR}/public.sdk/source/vst/utility/stringconvert.cpp"
    # Steinberg::Vst::ThreadChecker::create used by connectionproxy.cpp.
    # The header is public.sdk/source/common/threadchecker.h;
    # the Win32 implementation is in source/common/ (NOT source/vst/utility/).
    "${VST3SDK_DIR}/public.sdk/source/common/threadchecker_win32.cpp"
    # Steinberg::MemoryStream used by VSTPlugin3::save/loadState
    "${VST3SDK_DIR}/public.sdk/source/common/memorystream.cpp"
    "${VST3SDK_DIR}/base/source/fstring.cpp"
    # Steinberg::FStreamer / IBStreamer used by VSTPlugin3::save/loadState
    "${VST3SDK_DIR}/base/source/fstreamer.cpp"
    "${VST3SDK_DIR}/base/source/fbuffer.cpp"
    "${VST3SDK_DIR}/base/source/updatehandler.cpp"
    "${VST3SDK_DIR}/pluginterfaces/base/funknown.cpp"
    "${VST3SDK_DIR}/pluginterfaces/base/conststringtable.cpp"
    # Defines DEF_CLASS_IID for FUnknown, IPluginFactory, IBStream, IPluginBase,
    # ICloneable, IUpdateHandler, ISizeableStream, etc.  Without this the linker
    # reports unresolved externals for every interface ::iid static member.
    "${VST3SDK_DIR}/pluginterfaces/base/coreiids.cpp"
    # Defines IPlugView::iid, IPlugFrame::iid, IPlugViewContentScaleSupport::iid
    # (included by VSTPlugin3.h via pluginterfaces/gui/iplugview.h).
    "${VST3SDK_DIR}/public.sdk/source/common/commoniids.cpp"
)

# Use a generator-expression-friendly existence check: build the list only
# from files that actually exist on disk.  When FetchContent populates the
# tree the files will be present; if someone runs cmake before the tree is
# ready they get a clear warning rather than a hard error.
set(VST3_HOSTING_SOURCES_FOUND)
foreach(_src IN LISTS VST3_HOSTING_SOURCES)
    if(EXISTS "${_src}")
        list(APPEND VST3_HOSTING_SOURCES_FOUND "${_src}")
    endif()
endforeach()

if(VST3_HOSTING_SOURCES_FOUND)
    add_library(vst3_hosting STATIC ${VST3_HOSTING_SOURCES_FOUND})
else()
    # Fallback INTERFACE target so that downstream targets can still link
    # without errors; the real sources will appear after FetchContent runs
    # on a subsequent cmake invocation.
    message(WARNING
        "audiodsp.vsthost: VST3 hosting source files not yet present under "
        "${VST3SDK_DIR}. Configure again after the SDK has been populated "
        "(git submodule update --init --recursive, or let FetchContent finish).")
    add_library(vst3_hosting INTERFACE)
endif()

if(TARGET vst3_hosting)
    get_target_property(_vst3_hosting_type vst3_hosting TYPE)
    if(_vst3_hosting_type STREQUAL "INTERFACE_LIBRARY")
        target_include_directories(vst3_hosting INTERFACE "${VST3SDK_DIR}")
    else()
        target_include_directories(vst3_hosting PUBLIC "${VST3SDK_DIR}")
    endif()
endif()

if(NOT TARGET vst3_hosting OR NOT VST3_HOSTING_SOURCES_FOUND)
    # Nothing more to configure for the hosting lib.
else()
    target_compile_features(vst3_hosting PUBLIC cxx_std_17)
    if(WIN32)
        target_compile_definitions(vst3_hosting PUBLIC WIN32 UNICODE _UNICODE)
    endif()
endif()

# ---------------------------------------------------------------------------
# Collect main plugin sources
# (src/ subdirectories: bridge, dsp, plugin, settings, util, vst2, vst3)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE PLUGIN_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

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
        "${VST3SDK_DIR}"
        "${KODI_ADSP_SDK_DIR}"
)

target_link_libraries(audiodsp.vsthost
    PRIVATE
        vst3_hosting
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
            "${VST3SDK_DIR}"
    )

    target_link_libraries(vstscanner
        PRIVATE
            vst3_hosting
    )

    if(WIN32)
        target_compile_definitions(vstscanner PRIVATE
            NOMINMAX
            WIN32
            UNICODE
            _UNICODE
        )
    endif()

    # When the VST3 hosting library sources are present, tell vstscanner to
    # use the full SDK path (VST3::Hosting::Module) rather than the raw vtable
    # fallback.  The fallback still compiles when SDK is unavailable.
    if(VST3_HOSTING_SOURCES_FOUND)
        target_compile_definitions(vstscanner PRIVATE VST3SDK_AVAILABLE)
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
message(STATUS "  VST3 SDK dir     : ${VST3SDK_DIR}")
message(STATUS "  Kodi ADSP SDK    : ${KODI_ADSP_SDK_DIR}")
message(STATUS "  Platform         : ${CMAKE_SYSTEM_NAME}")
message(STATUS "------------------------------------------------------------")
