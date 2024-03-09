#.rst:
# FindPlayerAPIs
# --------
# Finds the PlayerAPIs library
#
# This will define the following target:
#
#   PLAYERAPIS::PLAYERAPIS   - The playerAPIs library

if(NOT TARGET PLAYERAPIS::PLAYERAPIS)

  if(PlayerAPIs_FIND_VERSION)
    if(PlayerAPIs_FIND_VERSION_EXACT)
      set(PlayerAPIs_FIND_SPEC "=${PlayerAPIs_FIND_VERSION_COMPLETE}")
    else()
      set(PlayerAPIs_FIND_SPEC ">=${PlayerAPIs_FIND_VERSION_COMPLETE}")
    endif()
  endif()

  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_PLAYERAPIS libplayerAPIs${PlayerAPIs_FIND_SPEC} QUIET)
  endif()

  find_path(PLAYERAPIS_INCLUDE_DIR NAMES starfish-media-pipeline/StarfishMediaAPIs.h
                                   HINTS ${PC_PLAYERAPIS_INCLUDEDIR}
                                   NO_CACHE)
  find_library(PLAYERAPIS_LIBRARY NAMES playerAPIs
                                  HINTS ${PC_PLAYERAPIS_LIBDIR}
                                  NO_CACHE)

  set(PLAYERAPIS_VERSION ${PC_PLAYERAPIS_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PlayerAPIs
                                    REQUIRED_VARS PLAYERAPIS_LIBRARY PLAYERAPIS_INCLUDE_DIR
                                    VERSION_VAR PLAYERAPIS_VERSION)

  if(PLAYERAPIS_FOUND)
    add_library(PLAYERAPIS::PLAYERAPIS UNKNOWN IMPORTED)
    set_target_properties(PLAYERAPIS::PLAYERAPIS PROPERTIES
                                                 IMPORTED_LOCATION "${PLAYERAPIS_LIBRARY}"
                                                 INTERFACE_INCLUDE_DIRECTORIES "${PLAYERAPIS_INCLUDE_DIR}")
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP PLAYERAPIS::PLAYERAPIS)
  endif()
endif()
