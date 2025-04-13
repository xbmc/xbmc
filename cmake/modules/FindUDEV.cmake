#.rst:
# FindUDEV
# -------
# Finds the UDEV library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::UDEV   - The UDEV library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig ${SEARCH_QUIET})
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_UDEV libudev ${SEARCH_QUIET})
  endif()

  find_path(UDEV_INCLUDE_DIR NAMES libudev.h
                             HINTS ${PC_UDEV_INCLUDEDIR})
  find_library(UDEV_LIBRARY NAMES udev
                            HINTS ${PC_UDEV_LIBDIR})

  set(UDEV_VERSION ${PC_UDEV_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(UDEV
                                    REQUIRED_VARS UDEV_LIBRARY UDEV_INCLUDE_DIR
                                    VERSION_VAR UDEV_VERSION)

  if(UDEV_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${UDEV_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${UDEV_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAVE_LIBUDEV)
  endif()
endif()
