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
  list(APPEND GL_INTERFACES_LIST glx)
  set(GL_INTERFACES_LIST ${GL_INTERFACES_LIST} PARENT_SCOPE)

  set(GLX_LIBRARIES ${GLX_LIBRARY})
  set(GLX_INCLUDE_DIRS ${GLX_INCLUDE_DIR})
  set(GLX_DEFINITIONS -DHAS_GLX=1)
endif()

mark_as_advanced(GLX_INCLUDE_DIR GLX_LIBRARY)
