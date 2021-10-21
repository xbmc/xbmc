#.rst:
# FindOpenGl
# ----------
# Finds the FindOpenGl library
#
# This will define the following variables::
#
# OPENGL_FOUND - system has OpenGl
# OPENGL_INCLUDE_DIRS - the OpenGl include directory
# OPENGL_LIBRARIES - the OpenGl libraries
# OPENGL_DEFINITIONS - the OpenGl definitions

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_OPENGL gl QUIET)
endif()

if(NOT CORE_SYSTEM_NAME STREQUAL osx)
  find_path(OPENGL_INCLUDE_DIR GL/gl.h
                               PATHS ${PC_OPENGL_gl_INCLUDEDIR})
  find_library(OPENGL_gl_LIBRARY NAMES GL
                                 PATHS ${PC_OPENGL_gl_LIBDIR})
else()
  find_library(OPENGL_gl_LIBRARY NAMES OpenGL
                                 PATHS ${CMAKE_OSX_SYSROOT}/System/Library
                                 PATH_SUFFIXES Frameworks
                                 NO_DEFAULT_PATH)
  set(OPENGL_INCLUDE_DIR ${OPENGL_gl_LIBRARY}/Headers)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenGl
                                  REQUIRED_VARS OPENGL_gl_LIBRARY OPENGL_INCLUDE_DIR)

if(OPENGL_FOUND)
  set(OPENGL_INCLUDE_DIRS ${OPENGL_INCLUDE_DIR})
  set(OPENGL_LIBRARIES ${OPENGL_gl_LIBRARY})
  set(OPENGL_DEFINITIONS -DHAS_GL=1)
endif()

mark_as_advanced(OPENGL_INCLUDE_DIR OPENGL_gl_LIBRARY)
