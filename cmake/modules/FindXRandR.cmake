#.rst:
# FindXRandR
# ----------
# Finds the XRandR library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::XRandR   - The XRANDR library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig QUIET)

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_XRANDR xrandr QUIET)
  endif()

  find_path(XRANDR_INCLUDE_DIR NAMES X11/extensions/Xrandr.h
                               HINTS ${PC_XRANDR_INCLUDEDIR})
  find_library(XRANDR_LIBRARY NAMES Xrandr
                              HINTS ${PC_XRANDR_LIBDIR})

  set(XRANDR_VERSION ${PC_XRANDR_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(XRandR
                                    REQUIRED_VARS XRANDR_LIBRARY XRANDR_INCLUDE_DIR
                                    VERSION_VAR XRANDR_VERSION)

  if(XRANDR_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${XRANDR_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${XRANDR_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAVE_LIBXRANDR)
  endif()
endif()
