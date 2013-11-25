
if(NOT WIN32)
  include(CheckSystemIncludes)
  include(CheckSystemFunctions)
endif(NOT WIN32)

set(PLEX_TARGET_NAME PlexHomeTheater)

set(CONFIG_INTERNAL_LIBS
  lib_hts
  lib_squish
  lib_upnp
)

OPTION(ENABLE_DVD_DRIVE "Enable the DVD drive" OFF)
OPTION(ENABLE_PYTHON "Enable Python addon support" OFF)
OPTION(CREATE_BUNDLE "Create the finished bundle" ON)
OPTION(COMPRESS_TEXTURES "If we should compress the textures or not" ON)
OPTION(ENABLE_NEW_SKIN "Enable the new Plex skin" ON)
OPTION(ENABLE_AUTOUPDATE "Enable the cool autoupdate system" OFF)

if(ENABLE_NEW_SKIN)
  add_definitions(-DPLEX_NEW_SKIN=1)
endif(ENABLE_NEW_SKIN)

if(NOT DEFINED TARGET_PLATFORM)
  if(APPLE)
    set(TARGET_PLATFORM "OSX")
  elseif(WIN32)
    set(TARGET_PLATFORM "WIN32")
  elseif(UNIX)
    set(TARGET_PLATFORM "LINUX")
  endif()
endif()

string(TOUPPER ${TARGET_PLATFORM} TARGET_PLATFORM)

if(${TARGET_PLATFORM} STREQUAL "OSX")
  set(TARGET_OSX 1 CACHE BOOL "Target is OSX")
  set(TARGET_COMMON_DARWIN 1 CACHE BOOL "Common Darwin platforms")
  set(TARGET_POSIX 1 CACHE BOOL "POSIX platform")
elseif(${TARGET_PLATFORM} STREQUAL "WIN32")
  set(TARGET_WIN32 1 CACHE BOOL "Target is Windows")
elseif(${TARGET_PLATFORM} STREQUAL "LINUX")
  set(TARGET_COMMON_LINUX 1 CACHE BOOL "Common Linux platforms")
  set(TARGET_LINUX 1 CACHE BOOL "Target is Linux")
  set(TARGET_POSIX 1 CACHE BOOL "POSIX platform")
elseif(${TARGET_PLATFORM} STREQUAL "RPI")
  set(TARGET_RPI 1 CACHE BOOL "Target in RaspberryPI")
  set(TARGET_COMMON_LINUX 1 CACHE BOOL "Common Linux platforms")
  set(TARGET_POSIX 1 CACHE BOOL "POSIX platform")
elseif(${TARGET_PLATFORM} STREQUAL "IOS")
  set(TARGET_IOS 1 CACHE BOOL "Target is iOS")
  set(TARGET_COMMON_DARWIN 1 CACHE BOOL "Common Darwin platforms")
  set(TARGET_POSIX 1 CACHE BOOL "POSIX Platform")
  message(WARNING "TARGET_IOS not yet supported")
else()
  message(FATAL_ERROR "Unknown platform target ${TARGET_PLATFORM}")
endif()

set(TARGET_PLATFORM ${TARGET_PLATFORM} CACHE STRING "Platform string")

message(STATUS "Building for target ${TARGET_PLATFORM}")

include(PlatformConfig${TARGET_PLATFORM})

if(TARGET_POSIX)
  include(PlatformConfigPOSIX)
endif(TARGET_POSIX)

############ global definitions set for all platforms

if(TARGET_OSX)
  set(BUILD_TAG "macosx-${OSX_ARCH}")
elseif(TARGET_WIN32)
  set(BUILD_TAG "windows-x86")
elseif(TARGET_LINUX)
  set(BUILD_TAG "linux-${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif(TARGET_OSX)

add_definitions(-D__PLEX__ -D__PLEX__XBMC__ -DPLEX_BUILD_TAG="${BUILD_TAG}" -DPLEX_TARGET_NAME="${EXECUTABLE_NAME}" -DENABLE_DVDINPUTSTREAM_STACK)
set_directory_properties(PROPERTIES COMPILE_DEFINITIONS_DEBUG "_DEBUG")

include(CheckFFmpegIncludes)
if (NOT TARGET_RPI)
  include(CheckCrystalHDInclude)
endif()
include(CheckLibshairportConfig)

if(DEFINED SDL_FOUND)
  set(HAVE_SDL 1)
endif()

if(ENABLE_PYTHON)
  set(HAS_PYTHON 1)
endif()

set(USE_UPNP 1)

if(ENABLE_DVD_DRIVE)
  set(HAS_DVD_DRIVE 1)
endif(ENABLE_DVD_DRIVE)

configure_file(${root}/xbmc/DllPaths_generated.h.in ${CMAKE_BINARY_DIR}/xbmc/DllPaths_generated.h)

find_package(SSE)
if(NOT TARGET_WIN32)
  if(SSSE3_FOUND)
    set(CMAKE_SSE_CFLAGS "-mssse3")
  elseif(SSE3_FOUND)
    set(CMAKE_SSE_CFLAGS "-msse3")
  elseif(SSE2_FOUND)
    set(CMAKE_SSE_CFLAGS "-msse2")
  endif(SSSE3_FOUND)
else(NOT TARGET_WIN32)
  set(CMAKE_SSE_CFLAGS "/arch:SSE2")
endif(NOT TARGET_WIN32)

find_package(Breakpad)
if(HAVE_BREAKPAD)
  include_directories(${BREAKPAD_INC_DIR})
  add_definitions(-DHAVE_BREAKPAD)
endif(HAVE_BREAKPAD)

# check some compiler Intrinsics
find_package(Intrinsics)

# this file is not needed on windows
if(NOT WIN32)
  configure_file(${plexdir}/config.h.in ${CMAKE_BINARY_DIR}/xbmc/config.h)
  set_source_files_properties(xbmc/config.h PROPERTIES GENERATED TRUE)
endif(NOT WIN32)

message(STATUS "-- Configuration Summary:")
message(STATUS "Enable DVD drive: " ${ENABLE_DVD_DRIVE})
message(STATUS "Enable Python support: " ${ENABLE_PYTHON})
message(STATUS "Enabling bundling: " ${CREATE_BUNDLE})
message(STATUS "Compress textures: " ${COMPRESS_TEXTURES})
message(STATUS "Enable AutoUpdate: " ${ENABLE_AUTOUPDATE})
if(CMAKE_SSE_CFLAGS)
  message(STATUS "SSE CFLAGS: ${CMAKE_SSE_CFLAGS}")
else(CMAKE_SSE_CFLAGS)
  message(STATUS "SSE CFLAGS: none")
endif(CMAKE_SSE_CFLAGS)

if(HAVE_BREAKPAD)
  message(STATUS "Enable CrashReports: yes")
else(HAVE_BREAKPAD)
  message(STATUS "Enable CrashReports: no")
endif(HAVE_BREAKPAD)
