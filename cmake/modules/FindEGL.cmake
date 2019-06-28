#.rst:
# FindEGL
# -------
# Finds the EGL library
#
# This will define the following variables::
#
# EGL_FOUND - system has EGL
# EGL_INCLUDE_DIRS - the EGL include directory
# EGL_LIBRARIES - the EGL libraries
# EGL_DEFINITIONS - the EGL definitions
#
# and the following imported targets::
#
#   EGL::EGL   - The EGL library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_EGL egl QUIET)
endif()

find_path(EGL_INCLUDE_DIR EGL/egl.h
                          PATHS ${PC_EGL_INCLUDEDIR})

find_library(EGL_LIBRARY NAMES EGL egl
                         PATHS ${PC_EGL_LIBDIR})

set(EGL_VERSION ${PC_EGL_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EGL
                                  REQUIRED_VARS EGL_LIBRARY EGL_INCLUDE_DIR
                                  VERSION_VAR EGL_VERSION)

if(EGL_FOUND)
  set(EGL_LIBRARIES ${EGL_LIBRARY})
  set(EGL_INCLUDE_DIRS ${EGL_INCLUDE_DIR})
  set(EGL_DEFINITIONS -DHAS_EGL=1)

  if(NOT TARGET EGL::EGL)
    add_library(EGL::EGL UNKNOWN IMPORTED)
    set_target_properties(EGL::EGL PROPERTIES
                                   IMPORTED_LOCATION "${EGL_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${EGL_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAS_EGL=1)
  endif()
endif()

mark_as_advanced(EGL_INCLUDE_DIR EGL_LIBRARY)
