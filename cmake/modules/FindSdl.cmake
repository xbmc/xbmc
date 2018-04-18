#.rst:
# FindSDL
# -------
# Finds the SDL library
#
# This will define the following variables::
#
# SDL_FOUND - system has SDL
# SDL_INCLUDE_DIRS - the SDL include directory
# SDL_LIBRARIES - the SDL libraries
# SDL_DEFINITIONS - the SDL compile definitions

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_SDL sdl QUIET)
endif()

find_path(SDL_INCLUDE_DIR SDL/SDL.h PATHS ${PC_SDL_INCLUDE_DIR})
find_library(SDL_LIBRARY NAMES SDL PATHS ${PC_SDL_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sdl REQUIRED_VARS SDL_LIBRARY SDL_INCLUDE_DIR)

if(SDL_FOUND)
  set(SDL_LIBRARIES ${SDL_LIBRARY})
  set(SDL_INCLUDE_DIRS ${SDL_INCLUDE_DIR})
  set(SDL_DEFINITIONS -DHAVE_SDL=1)
endif()

mark_as_advanced(SDL_LIBRARY SDL_INCLUDE_DIR)
