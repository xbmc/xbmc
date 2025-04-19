#.rst:
# FindPlayerFactory
# --------
# Finds the PlayerFactory library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::PlayerFactory   - The PlayerFactory library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  SETUP_FIND_SPECS()

  find_package(PkgConfig ${SEARCH_QUIET})
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_PLAYERFACTORY libpf-1.0${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET})
  endif()

  find_path(PLAYERFACTORY_INCLUDE_DIR NAMES player-factory/common.hpp
                                      HINTS ${PC_PLAYERFACTORY_INCLUDEDIR})
  find_library(PLAYERFACTORY_LIBRARY NAMES pf-1.0
                                     HINTS ${PC_PLAYERFACTORY_LIBDIR})

  set(PLAYERFACTORY_VERSION ${PC_PLAYERFACTORY_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PlayerFactory
                                    REQUIRED_VARS PLAYERFACTORY_LIBRARY PLAYERFACTORY_INCLUDE_DIR
                                    VERSION_VAR PLAYERFACTORY_VERSION)

  if(PLAYERFACTORY_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${PLAYERFACTORY_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${PLAYERFACTORY_INCLUDE_DIR}")
  else()
    if(PlayerFactory_FIND_REQUIRED)
      message(FATAL_ERROR "PlayerFactory library not found.")
    endif()
  endif()
endif()
