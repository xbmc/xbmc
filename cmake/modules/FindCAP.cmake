#.rst:
# FindCAP
# -----------
# Finds the POSIX 1003.1e capabilities library
#
# This will define the following target:
#
# ${APP_NAME_LC}::CAP - The LibCap library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_CAP libcap QUIET)
  endif()

  find_path(CAP_INCLUDE_DIR NAMES sys/capability.h
                            HINTS ${PC_CAP_INCLUDEDIR})
  find_library(CAP_LIBRARY NAMES cap libcap
                           HINTS ${PC_CAP_LIBDIR})

  set(CAP_VERSION ${PC_CAP_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(CAP
                                    REQUIRED_VARS CAP_LIBRARY CAP_INCLUDE_DIR
                                    VERSION_VAR CAP_VERSION)

  if(CAP_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${CAP_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${CAP_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAVE_LIBCAP)
  endif()
endif()
