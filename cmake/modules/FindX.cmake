#.rst:
# FindX
# -----
# Finds the X11 library
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::X    - The X11 library
#   ${APP_NAME_LC}::Xext - The X11 extension library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig ${SEARCH_QUIET})
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_X x11 xext ${SEARCH_QUIET})
  endif()

  find_path(X_INCLUDE_DIR NAMES X11/Xlib.h
                          HINTS ${PC_X_x11_INCLUDEDIR})
  find_library(X_LIBRARY NAMES X11
                         HINTS ${PC_X_x11_LIBDIR})
  find_library(X_EXT_LIBRARY NAMES Xext
                             HINTS ${PC_X_xext_LIBDIR})

  set(X_VERSION ${PC_X_x11_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(X
                                    REQUIRED_VARS X_LIBRARY X_EXT_LIBRARY X_INCLUDE_DIR
                                    VERSION_VAR X_VERSION)

  if(X_FOUND)
    add_library(${APP_NAME_LC}::Xext UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::Xext PROPERTIES
                                               IMPORTED_LOCATION "${X_EXT_LIBRARY}"
                                               INTERFACE_INCLUDE_DIRECTORIES "${X_INCLUDE_DIR}")
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${X_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${X_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAVE_X11
                                                                     INTERFACE_LINK_LIBRARIES ${APP_NAME_LC}::Xext)
  endif()
endif()
