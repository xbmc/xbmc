#.rst:
# FindSDL
# -------
# Finds the SDL library
#
# This will will define the following variables::
#
# SDL_FOUND - system has SDL
# SDL_INCLUDE_DIRS - the SDL include directory
# SDL_LIBRARIES - the SDL libraries
# SDL_DEFINITIONS - the SDL compile definitions

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_SDL2 sdl2 QUIET)
  if(PC_SDL2_FOUND)
    set(SDL_VERSION ${PC_SDL2_VERSION})
  else()
    pkg_check_modules(PC_SDL1 sdl QUIET)
    if(PC_SDL1_FOUND)
      set(SDL_VERSION ${PC_SDL1_VERSION})
    endif()
  endif()
endif()

find_path(SDL_INCLUDE_DIR SDL/SDL.h
                          PATHS ${PC_SDL2_INCLUDE_DIR} ${PC_SDL1_INCLUDE_DIR})
find_library(SDL_LIBRARY NAMES SDL2 SDL
                         PATHS ${PC_SDL2_LIBDIR} ${PC_SDL1_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sdl REQUIRED_VARS SDL_LIBRARY SDL_INCLUDE_DIR
                                      VERSION_VAR SDL_VERSION)

if(SDL_FOUND)
  set(SDL_LIBRARIES ${SDL_LIBRARY})
  set(SDL_INCLUDE_DIRS ${SDL_INCLUDE_DIR})
  set(SDL_DEFINITIONS -DHAVE_SDL=1)

  if(SDL_VERSION VERSION_GREATER 2)
    list(APPEND SDL_DEFINITIONS -DHAVE_SDL_VERSION=2)
  elseif(SDL_VERSION VERSION_GREATER 1)
    list(APPEND SDL_DEFINITIONS -DHAVE_SDL_VERSION=1)
  endif()
endif()

mark_as_advanced(SDL_LIBRARY SDL_INCLUDE_DIR)
