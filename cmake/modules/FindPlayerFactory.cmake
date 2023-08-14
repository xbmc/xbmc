#.rst:
# FindPlayerFactory
# --------
# Finds the PlayerFactory library
#
# This will define the following variables::
#
# PLAYERFACTORY_FOUND - system has PlayerFactory
# PLAYERFACTORY_INCLUDE_DIRS - the PlayerFactory include directory
# PLAYERFACTORY_LIBRARIES - the PlayerFactory libraries
# PLAYERFACTORY_DEFINITIONS - the PlayerFactory compile definitions
#
# and the following imported targets::
#
#   PLAYERFACTORY::PLAYERFACTORY   - The PlayerFactory library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_PLAYERFACTORY libpf-1.0>=1.0.0 QUIET)
endif()

find_path(PLAYERFACTORY_INCLUDE_DIR NAMES player-factory/common.hpp
        PATHS ${PC_PLAYERFACTORY_INCLUDEDIR})
find_library(PLAYERFACTORY_LIBRARY NAMES pf-1.0
        PATHS ${PC_PLAYERFACTORY_LIBDIR})

set(PLAYERFACTORY_VERSION ${PC_PLAYERFACTORY_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PlayerFactory
                                  REQUIRED_VARS PLAYERFACTORY_LIBRARY PLAYERFACTORY_INCLUDE_DIR
                                  VERSION_VAR PLAYERFACTORY_VERSION)

if(PLAYERFACTORY_FOUND)
  set(PLAYERFACTORY_INCLUDE_DIRS ${PLAYERFACTORY_INCLUDE_DIR})
  set(PLAYERFACTORY_LIBRARIES ${PLAYERFACTORY_LIBRARY})

  if(NOT TARGET PLAYERFACTORY::PLAYERFACTORY)
    add_library(PLAYERFACTORY::PLAYERFACTORY UNKNOWN IMPORTED)
    set_target_properties(PLAYERFACTORY::PLAYERFACTORY PROPERTIES
                                     IMPORTED_LOCATION "${PLAYERFACTORY_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${PLAYERFACTORY_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(PLAYERFACTORY_INCLUDE_DIR PLAYERFACTORY_LIBRARY)
