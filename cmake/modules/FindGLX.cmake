#.rst:
# FindGLX
# -----
# Finds the GLX library
#
# This will define the following variables::
#
# GLX_FOUND - system has GLX
# GLX_INCLUDE_DIRS - the GLX include directory
# GLX_LIBRARIES - the GLX libraries
# GLX_DEFINITIONS - the GLX definitions
#
# and the following imported targets::
#
#   GLX::GLX    - The GLX library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_GLX glx QUIET)
endif()

find_path(GLX_INCLUDE_DIR NAMES GL/glx.h
                          PATHS ${PC_GLX_INCLUDEDIR})
find_library(GLX_LIBRARY NAMES GL
                         PATHS ${PC_GLX_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLX
                                  REQUIRED_VARS GLX_LIBRARY GLX_INCLUDE_DIR)

if(GLX_FOUND)
  set(GLX_LIBRARIES ${GLX_LIBRARY})
  set(GLX_INCLUDE_DIRS ${GLX_INCLUDE_DIR})
  set(GLX_DEFINITIONS -DHAS_GLX=1)

  if(NOT TARGET GLX::GLX)
    add_library(GLX::GLX UNKNOWN IMPORTED)
    set_target_properties(GLX::GLX PROPERTIES
                               IMPORTED_LOCATION "${GLX_LIBRARY}"
                               INTERFACE_INCLUDE_DIRECTORIES "${GLX_INCLUDE_DIR}"
                               INTERFACE_COMPILE_DEFINITIONS HAS_GLX=1)
  endif()
endif()

mark_as_advanced(GLX_INCLUDE_DIR GLX_LIBRARY)
