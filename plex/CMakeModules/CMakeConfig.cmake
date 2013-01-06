
include(CheckSystemIncludes)
include(CheckSystemFunctions)

set(PLEX_TARGET_NAME PlexHomeTheater)

set(CONFIG_INTERNAL_LIBS
  lib_hts
  lib_squish
  lib_upnp
)

if(APPLE)
  include(CMakeConfigOSX)
elseif(WIN32)
  include(CMakeConfigWin)
elseif(UNIX)
  include(CMakeConfigLinux)
endif()

include(CheckFFmpegIncludes)
include(CheckCrystalHDInclude)

if(DEFINED SDL_FOUND)
  set(HAVE_SDL 1)
endif()

set(USE_UPNP 1)
set(HAS_DVD_DRIVE 0)
set(HAS_LIBRTMP 1)

configure_file(${root}/xbmc/DllPaths_generated.h.in ${CMAKE_BINARY_DIR}/xbmc/DllPaths_generated.h)
configure_file(${plexdir}/config.h.in ${CMAKE_BINARY_DIR}/xbmc/config.h)
