#.rst:
# FindPlayerAPIs
# --------
# Finds the PlayerAPIs library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::PlayerAPIs   - The playerAPIs library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    if(PlayerAPIs_FIND_VERSION)
      if(PlayerAPIs_FIND_VERSION_EXACT)
        set(PlayerAPIs_FIND_SPEC "=${PlayerAPIs_FIND_VERSION_COMPLETE}")
      else()
        set(PlayerAPIs_FIND_SPEC ">=${PlayerAPIs_FIND_VERSION_COMPLETE}")
      endif()
    endif()

    pkg_check_modules(PC_PLAYERAPIS libplayerAPIs${PlayerAPIs_FIND_SPEC} QUIET)
  endif()

  find_path(PLAYERAPIS_INCLUDE_DIR NAMES starfish-media-pipeline/StarfishMediaAPIs.h
                                   HINTS ${PC_PLAYERAPIS_INCLUDEDIR})
  find_library(PLAYERAPIS_LIBRARY NAMES playerAPIs
                                  HINTS ${PC_PLAYERAPIS_LIBDIR})

  set(PLAYERAPIS_VERSION ${PC_PLAYERAPIS_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PlayerAPIs
                                    REQUIRED_VARS PLAYERAPIS_LIBRARY PLAYERAPIS_INCLUDE_DIR
                                    VERSION_VAR PLAYERAPIS_VERSION)

  if(PLAYERAPIS_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${PLAYERAPIS_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${PLAYERAPIS_INCLUDE_DIR}")
  else()
    if(PlayerAPIs_FIND_REQUIRED)
      message(FATAL_ERROR "PlayerAPIs library not found.")
    endif()
  endif()
endif()
