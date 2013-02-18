
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

if(APPLE)
  include(CMakeConfigOSX)
elseif(WIN32)
  include(CMakeConfigWin)
elseif(UNIX)
  include(CMakeConfigLinux)
endif()

if(UNIX)
  include(CMakeConfigPOSIX)
endif(UNIX)

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
