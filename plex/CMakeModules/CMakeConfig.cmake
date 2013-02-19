
include(CheckSystemIncludes)
include(CheckSystemFunctions)

set(PLEX_TARGET_NAME PlexHomeTheater)

set(CONFIG_INTERNAL_LIBS
  lib_hts
  lib_squish
  lib_upnp
)

OPTION(ENABLE_DVD_DRIVE "Enable the DVD drive" ON)
OPTION(ENABLE_PYTHON "Enable Python addon support" OFF)
OPTION(CREATE_BUNDLE "Create the finished bundle" ON)

set(compress_default OFF)
if(CREATE_BUNDLE AND NOT WIN32)
  set(compress_default ON)
endif()

OPTION(COMPRESS_TEXTURES "If we should compress the textures or not" compress_default)
  
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

if(TARGET_PLATFORM STREQUAL "OSX")
  set(TARGET_OSX 1)
  set(TARGET_COMMON_DARWIN 1)
  set(TARGET_POSIX 1)
elseif(TARGET_PLATFORM STREQUAL "WIN32")
  set(TARGET_WIN32 1)
elseif(TARGET_PLATFORM STREQUAL "LINUX")
  set(TARGET_COMMON_LINUX 1)
  set(TARGET_LINUX 1)
  set(TARGET_POSIX 1)
elseif(TARGET_PLATFORM STREQUAL "RPI")
  set(TARGET_RPI 1)
  set(TARGET_COMMON_LINUX 1)
  set(TARGET_POSIX 1)
elseif(TARGET_PLATFORM STREQUAL "IOS")
  set(TARGET_IOS 1)
  set(TARGET_COMMON_DARWIN 1)
  set(TARGET_POSIX 1)
  message(WARNING "TARGET_IOS not yet supported")
else()
  message(FATAL_ERROR "Unknown platform target ${TARGET_PLATFORM}")
endif()

message(STATUS "Building for target ${TARGET_PLATFORM}")

include(PlatformConfig${TARGET_PLATFORM})

if(TARGET_POSIX)
  include(PlatformConfigPOSIX)
endif(TARGET_POSIX)

############ global definitions set for all platforms
add_definitions(-D__PLEX__ -D__PLEX__XBMC__ -DPLEX_TARGET_NAME="${EXECUTABLE_NAME}" -DPLEX_VERSION="${PLEX_VERSION_STRING_SHORT_BUILD}")

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  add_definitions(-DDEBUG)
else()
  add_definitions(-DNDEBUG)
endif()

include(CheckFFmpegIncludes)
include(CheckCrystalHDInclude)
include(CheckLibshairportConfig)

if(DEFINED SDL_FOUND)
  set(HAVE_SDL 1)
endif()

if(ENABLE_PYTHON)
  set(HAS_PYTHON 1)
endif()

set(USE_UPNP 1)
set(HAS_LIBRTMP 1)

if(ENABLE_DVD_DRIVE)
  set(HAS_DVD_DRIVE 1)
endif(ENABLE_DVD_DRIVE)

configure_file(${root}/xbmc/DllPaths_generated.h.in ${CMAKE_BINARY_DIR}/xbmc/DllPaths_generated.h)
configure_file(${plexdir}/config.h.in ${CMAKE_BINARY_DIR}/xbmc/config.h)
set_source_files_properties(xbmc/config.h PROPERTIES GENERATED TRUE)

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
  set(CMAKE_SSE_CFLAGS "/arch:sse2")
endif(NOT TARGET_WIN32)

message(STATUS "-- Configuration Summary:")
message(STATUS "Enable DVD drive: " ${ENABLE_DVD_DRIVE})
message(STATUS "Enable Python support: " ${ENABLE_PYTHON})
message(STATUS "Enabling bundling: " ${CREATE_BUNDLE})
message(STATUS "Compress textures: " ${COMPRESS_TEXTURES})
if(CMAKE_SSE_CFLAGS)
  message(STATUS "SSE CFLAGS: ${CMAKE_SSE_CFLAGS}")
else(CMAKE_SSE_CFLAGS)
  message(STATUS "SSE CFLAGS: none")
endif(CMAKE_SSE_CFLAGS)