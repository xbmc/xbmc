#.rst:
# FindX
# -----
# Finds the X11 library
#
# This will define the following targets:
#
#   X::X    - The X11 library
#   X::Xext - The X11 extension library

if(NOT TARGET X::X)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_X x11 xext QUIET)
  endif()

  find_path(X_INCLUDE_DIR NAMES X11/Xlib.h
                          HINTS ${PC_X_x11_INCLUDEDIR}
                          NO_CACHE)
  find_library(X_LIBRARY NAMES X11
                         HINTS ${PC_X_x11_LIBDIR}
                         NO_CACHE)
  find_library(X_EXT_LIBRARY NAMES Xext
                             HINTS ${PC_X_xext_LIBDIR}
                             NO_CACHE)

  set(X_VERSION ${PC_X_x11_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(X
                                    REQUIRED_VARS X_LIBRARY X_EXT_LIBRARY X_INCLUDE_DIR
                                    VERSION_VAR X_VERSION)

  if(X_FOUND)
    add_library(X::Xext UNKNOWN IMPORTED)
    set_target_properties(X::Xext PROPERTIES
                                  IMPORTED_LOCATION "${X_EXT_LIBRARY}"
                                  INTERFACE_INCLUDE_DIRECTORIES "${X_INCLUDE_DIR}")
    add_library(X::X UNKNOWN IMPORTED)
    set_target_properties(X::X PROPERTIES
                               IMPORTED_LOCATION "${X_LIBRARY}"
                               INTERFACE_INCLUDE_DIRECTORIES "${X_INCLUDE_DIR}"
                               INTERFACE_COMPILE_DEFINITIONS HAVE_X11=1
                               INTERFACE_LINK_LIBRARIES X::Xext)

    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP X::X)
  endif()
endif()
