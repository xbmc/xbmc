#.rst:
# FindLibDisplayInfo
# -------
# Finds the libdisplay-info library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::LibDisplayInfo   - The LibDisplayInfo library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  find_package(PkgConfig)

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_LIBDISPLAYINFO libdisplay-info QUIET)
  endif()

  find_path(LIBDISPLAYINFO_INCLUDE_DIR libdisplay-info/edid.h
                            HINTS ${PC_LIBDISPLAYINFO_INCLUDEDIR})

  find_library(LIBDISPLAYINFO_LIBRARY NAMES display-info
                           HINTS ${PC_LIBDISPLAYINFO_LIBDIR})

  set(LIBDISPLAYINFO_VERSION ${PC_LIBDISPLAYINFO_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LibDisplayInfo
                                    REQUIRED_VARS LIBDISPLAYINFO_LIBRARY LIBDISPLAYINFO_INCLUDE_DIR
                                    VERSION_VAR LIBDISPLAYINFO_VERSION)

  if(LIBDISPLAYINFO_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${LIBDISPLAYINFO_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${LIBDISPLAYINFO_INCLUDE_DIR}")
  else()
    if(LibDisplayInfo_FIND_REQUIRED)
      message(FATAL_ERROR "Libdisplayinfo libraries were not found.")
    endif()
  endif()
endif()
