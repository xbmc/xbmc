# FindXkbcommon
# -----------
# Finds the libxkbcommon library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Xkbcommon   - The libxkbcommon library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_XKBCOMMON xkbcommon QUIET)
  endif()

  find_path(XKBCOMMON_INCLUDE_DIR NAMES xkbcommon/xkbcommon.h
                                  HINTS ${PC_XKBCOMMON_INCLUDEDIR})
  find_library(XKBCOMMON_LIBRARY NAMES xkbcommon
                                 HINTS ${PC_XKBCOMMON_LIBDIR})

  set(XKBCOMMON_VERSION ${PC_XKBCOMMON_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Xkbcommon
                                    REQUIRED_VARS XKBCOMMON_LIBRARY XKBCOMMON_INCLUDE_DIR
                                    VERSION_VAR XKBCOMMON_VERSION)

  if(XKBCOMMON_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${XKBCOMMON_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${XKBCOMMON_INCLUDE_DIR}")
  endif()
endif()
