#.rst:
# FindX
# -----
# Finds the X11 library
#
# This will will define the following variables::
#
# X_FOUND - system has X11
# X_INCLUDE_DIRS - the X11 include directory
# X_LIBRARIES - the X11 libraries
# X_DEFINITIONS - the X11 definitions
#
# and the following imported targets::
#
#   X::X    - The X11 library
#   X::Xext - The X11 extension library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_X x11 xext QUIET)
endif()

find_path(X_INCLUDE_DIR NAMES X11/Xlib.h
                        PATHS ${PC_X_x11_INCLUDEDIR})
find_library(X_LIBRARY NAMES X11
                       PATHS ${PC_X_x11_LIBDIR})
find_library(X_EXT_LIBRARY NAMES Xext
                           PATHS ${PC_X_xext_LIBDIR})

set(X_VERSION ${PC_X_x11_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(X
                                  REQUIRED_VARS X_LIBRARY X_EXT_LIBRARY X_INCLUDE_DIR
                                  VERSION_VAR X_VERSION)

if(X_FOUND)
  set(X_LIBRARIES ${X_LIBRARY} ${X_EXT_LIBRARY})
  set(X_INCLUDE_DIRS ${X_INCLUDE_DIR})
  set(X_DEFINITIONS -DHAVE_X11=1)

  if(NOT TARGET X::X)
    add_library(X::X UNKNOWN IMPORTED)
    set_target_properties(X::X PROPERTIES
                               IMPORTED_LOCATION "${X_LIBRARY}"
                               INTERFACE_INCLUDE_DIRECTORIES "${X_INCLUDE_DIR}"
                               INTERFACE_COMPILE_DEFINITIONS HAVE_X11=1)
  endif()
  if(NOT TARGET X::Xext)
    add_library(X::Xext UNKNOWN IMPORTED)
    set_target_properties(X::Xext PROPERTIES
                                  IMPORTED_LOCATION "${X_EXT_LIBRARY}"
                                  INTERFACE_INCLUDE_DIRECTORIES "${X_INCLUDE_DIR}"
                                  INTERFACE_LINK_LIBRARIES X::X)
  endif()
endif()

mark_as_advanced(X_INCLUDE_DIR X_LIBRARY X_EXT_LIBRARY)
