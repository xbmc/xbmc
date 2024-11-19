#.rst:
# FindGLU
# -----
# Finds the GLU library
#
# This will define the following variables::
#
# GLU_FOUND - system has GLU
# GLU_INCLUDE_DIRS - the GLU include directory
# GLU_LIBRARIES - the GLU libraries
# GLU_DEFINITIONS - the GLU definitions
#

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_GLU glu QUIET)
endif()

find_path(GLU_INCLUDE_DIR NAMES GL/glu.h
                          HINTS ${PC_GLU_INCLUDEDIR})
find_library(GLU_LIBRARY NAMES GLU
                         HINTS ${PC_GLU_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLU
                                  REQUIRED_VARS GLU_LIBRARY GLU_INCLUDE_DIR)

if(GLU_FOUND)
  set(GLU_LIBRARIES ${GLU_LIBRARY})
  set(GLU_INCLUDE_DIRS ${GLU_INCLUDE_DIR})
  set(GLU_DEFINITIONS -DHAS_GLU=1)
endif()

mark_as_advanced(GLU_INCLUDE_DIR GLU_LIBRARY)
