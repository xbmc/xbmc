#.rst:
# FindWebOSHelpers
# --------
# Finds the WebOSHelpers library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::WebOSHelpers   - The webOS helpers library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    if(WebOSHelpers_FIND_VERSION)
      if(WebOSHelpers_FIND_VERSION_EXACT)
        set(WebOSHelpers_FIND_SPEC "=${WebOSHelpers_FIND_VERSION_COMPLETE}")
      else()
        set(WebOSHelpers_FIND_SPEC ">=${WebOSHelpers_FIND_VERSION_COMPLETE}")
      endif()
    endif()

    pkg_check_modules(PC_WEBOSHELPERS helpers${WebOSHelpers_FIND_SPEC} QUIET)
  endif()

  find_path(WEBOSHELPERS_INCLUDE_DIR NAMES webos-helpers/libhelpers.h
                                     HINTS ${PC_WEBOSHELPERS_INCLUDEDIR})
  find_library(WEBOSHELPERS_LIBRARY NAMES helpers
                                    HINTS ${PC_WEBOSHELPERS_LIBDIR})

  set(WEBOSHELPERS_VERSION ${PC_WEBOSHELPERS_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(WebOSHelpers
                                    REQUIRED_VARS WEBOSHELPERS_LIBRARY WEBOSHELPERS_INCLUDE_DIR
                                    VERSION_VAR WEBOSHELPERS_VERSION)

  if(WEBOSHELPERS_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${WEBOSHELPERS_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${WEBOSHELPERS_INCLUDE_DIR}")
  else()
    if(WebOSHelpers_FIND_REQUIRED)
      message(FATAL_ERROR "WebOSHelpers libraries were not found.")
    endif()
  endif()
endif()
