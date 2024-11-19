#.rst:
# FindPlayerFactory
# --------
# Finds the PlayerFactory library
#
# This will define the following target:
#
#   PLAYERFACTORY::PLAYERFACTORY   - The PlayerFactory library

if(NOT TARGET PLAYERFACTORY::PLAYERFACTORY)

  if(PlayerFactory_FIND_VERSION)
    if(PlayerFactory_FIND_VERSION_EXACT)
      set(PlayerFactory_FIND_SPEC "=${PlayerFactory_FIND_VERSION_COMPLETE}")
    else()
      set(PlayerFactory_FIND_SPEC ">=${PlayerFactory_FIND_VERSION_COMPLETE}")
    endif()
  endif()

  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_PLAYERFACTORY libpf-1.0${PlayerFactory_FIND_SPEC} QUIET)
  endif()

  find_path(PLAYERFACTORY_INCLUDE_DIR NAMES player-factory/common.hpp
                                      HINTS ${PC_PLAYERFACTORY_INCLUDEDIR}
                                      NO_CACHE)
  find_library(PLAYERFACTORY_LIBRARY NAMES pf-1.0
                                     HINTS ${PC_PLAYERFACTORY_LIBDIR}
                                     NO_CACHE)

  set(PLAYERFACTORY_VERSION ${PC_PLAYERFACTORY_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PlayerFactory
                                    REQUIRED_VARS PLAYERFACTORY_LIBRARY PLAYERFACTORY_INCLUDE_DIR
                                    VERSION_VAR PLAYERFACTORY_VERSION)

  if(PLAYERFACTORY_FOUND)
    add_library(PLAYERFACTORY::PLAYERFACTORY UNKNOWN IMPORTED)
    set_target_properties(PLAYERFACTORY::PLAYERFACTORY PROPERTIES
                                                       IMPORTED_LOCATION "${PLAYERFACTORY_LIBRARY}"
                                                       INTERFACE_INCLUDE_DIRECTORIES "${PLAYERFACTORY_INCLUDE_DIR}")
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP PLAYERFACTORY::PLAYERFACTORY)
  endif()
endif()
