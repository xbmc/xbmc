#.rst:
# FindCWiid
# ---------
# Finds the CWiid library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::CWiid   - The CWiid library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_CWIID cwiid QUIET)
  endif()

  find_path(CWIID_INCLUDE_DIR NAMES cwiid.h
                              PATHS ${PC_CWIID_INCLUDEDIR})
  find_library(CWIID_LIBRARY NAMES cwiid
                             PATHS ${PC_CWIID_LIBDIR})

  set(CWIID_VERSION ${PC_CWIID_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(CWiid
                                    REQUIRED_VARS CWIID_LIBRARY CWIID_INCLUDE_DIR
                                    VERSION_VAR CWIID_VERSION)

  if(CWIID_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${CWIID_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${CWIID_INCLUDE_DIR}")

  endif()
endif()
