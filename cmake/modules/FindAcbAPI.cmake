#.rst:
# FindAcbAPI
# --------
# Finds the AcbAPI library
#
# This will define the following target:
#
#   ACBAPI::ACBAPI   - The acbAPI library

if(NOT TARGET ACBAPI::ACBAPI)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_ACBAPI libAcbAPI QUIET)
  endif()

  find_path(ACBAPI_INCLUDE_DIR NAMES appswitching-control-block/AcbAPI.h
                                   PATHS ${PC_ACBAPI_INCLUDEDIR}
                                   NO_CACHE)
  find_library(ACBAPI_LIBRARY NAMES AcbAPI
                                  PATHS ${PC_ACBAPI_LIBDIR}
                                  NO_CACHE)

  set(ACBAPI_VERSION ${PC_ACBAPI_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(AcbAPI
                                    REQUIRED_VARS ACBAPI_LIBRARY ACBAPI_INCLUDE_DIR
                                    VERSION_VAR ACBAPI_VERSION)

  if(ACBAPI_FOUND)
    add_library(ACBAPI::ACBAPI UNKNOWN IMPORTED)
    set_target_properties(ACBAPI::ACBAPI PROPERTIES
                                                 IMPORTED_LOCATION "${ACBAPI_LIBRARY}"
                                                 INTERFACE_INCLUDE_DIRECTORIES "${ACBAPI_INCLUDE_DIR}")
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP ACBAPI::ACBAPI)

    # creates an empty library to install on webOS 5+ devices
    file(TOUCH dummy.c)
    add_library(AcbAPI SHARED dummy.c)
    set_target_properties(AcbAPI PROPERTIES VERSION 1.0.0 SOVERSION 1)
  endif()
endif()
