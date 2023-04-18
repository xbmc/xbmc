#.rst:
# FindPlayerAPIs
# --------
# Finds the PlayerAPIs library
#
# This will define the following variables::
#
# PLAYERAPIS_FOUND - system has PlayerAPIs
# PLAYERAPIS_INCLUDE_DIRS - the PlayerAPIs include directory
# PLAYERAPIS_LIBRARIES - the PlayerAPIs libraries
# PLAYERAPIS_DEFINITIONS - the PlayerAPIs compile definitions
#
# and the following imported targets::
#
#   PLAYERAPIS::PLAYERAPIS   - The playerAPIs library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_PLAYERAPIS libplayerAPIs>=1.0.0 QUIET)
endif()

find_path(PLAYERAPIS_INCLUDE_DIR NAMES starfish-media-pipeline/StarfishMediaAPIs.h
        PATHS ${PC_PLAYERAPIS_INCLUDEDIR})
find_library(PLAYERAPIS_LIBRARY NAMES playerAPIs
        PATHS ${PC_PLAYERAPIS_LIBDIR})

set(PLAYERAPIS_VERSION 1.0.0)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PlayerAPIs
                                  REQUIRED_VARS PLAYERAPIS_LIBRARY PLAYERAPIS_INCLUDE_DIR
                                  VERSION_VAR PLAYERAPIS_VERSION)

if(PLAYERAPIS_FOUND)
  set(PLAYERAPIS_INCLUDE_DIRS ${PLAYERAPIS_INCLUDE_DIR})
  set(PLAYERAPIS_LIBRARIES ${PLAYERAPIS_LIBRARY})

  if(NOT TARGET PLAYERAPIS::PLAYERAPIS)
    add_library(PLAYERAPIS::PLAYERAPIS UNKNOWN IMPORTED)
    set_target_properties(PLAYERAPIS::PLAYERAPIS PROPERTIES
                                     IMPORTED_LOCATION "${PLAYERAPIS_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${PLAYERAPIS_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(PLAYERAPIS_INCLUDE_DIR PLAYERAPIS_LIBRARY)
