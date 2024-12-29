#.rst:
# FindAcbAPI
# --------
# Finds the AcbAPI library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::AcbAPI   - The acbAPI library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_ACBAPI libAcbAPI QUIET)
  endif()

  find_path(ACBAPI_INCLUDE_DIR NAMES appswitching-control-block/AcbAPI.h
                               HINTS ${PC_ACBAPI_INCLUDEDIR}
                               NO_CACHE)
  find_library(ACBAPI_LIBRARY NAMES AcbAPI
                              HINTS ${PC_ACBAPI_LIBDIR}
                              NO_CACHE)

  set(ACBAPI_VERSION ${PC_ACBAPI_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(AcbAPI
                                    REQUIRED_VARS ACBAPI_LIBRARY ACBAPI_INCLUDE_DIR
                                    VERSION_VAR ACBAPI_VERSION)

  if(ACBAPI_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${ACBAPI_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${ACBAPI_INCLUDE_DIR}")

    # creates an empty library to install on webOS 5+ devices
    file(TOUCH dummy.c)
    add_library(AcbAPI SHARED dummy.c)
    set_target_properties(AcbAPI PROPERTIES VERSION 1.0.0 SOVERSION 1)
  else()
    if(AcbAPI_FIND_REQUIRED)
      message(FATAL_ERROR "AcbAPI libraries were not found.")
    endif()
  endif()
endif()
