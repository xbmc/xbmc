#.rst:
# FindOpenGLES
# ------------
# Finds the OpenGLES2 library
#
# This will will define the following variables::
#
# OPENGLES_FOUND - system has OpenGLES
# OPENGLES_INCLUDE_DIRS - the OpenGLES include directory
# OPENGLES_LIBRARIES - the OpenGLES libraries
# OPENGLES_DEFINITIONS - the OpenGLES definitions

find_package(EMBEDDED)

if(PKG_CONFIG_FOUND AND NOT PLATFORM STREQUAL "raspberry-pi")
  pkg_check_modules(PC_OPENGLES glesv2 QUIET)
  if(NOT OPENGLES_FOUND AND EMBEDDED_FOUND)
    set(CMAKE_PREFIX_PATH ${EMBEDDED_FOUND} ${CMAKE_PREFIX_PATH})
  endif()
endif()

if(NOT CORE_SYSTEM_NAME STREQUAL ios)
  find_path(OPENGLES_INCLUDE_DIR GLES2/gl2.h
                                 PATHS ${PC_OPENGLES_INCLUDEDIR})
  find_library(OPENGLES_gl_LIBRARY NAMES GLESv2
                                   PATHS ${PC_OPENGLES_LIBDIR})
  find_library(OPENGLES_egl_LIBRARY NAMES EGL
                                    PATHS ${PC_OPENGLES_LIBDIR})
else()
  find_library(OPENGLES_gl_LIBRARY NAMES OpenGLES
                                   PATHS ${CMAKE_OSX_SYSROOT}/System/Library
                                   PATH_SUFFIXES Frameworks
                                   NO_DEFAULT_PATH)
  set(OPENGLES_INCLUDE_DIR ${OPENGLES_gl_LIBRARY}/Headers)
  set(OPENGLES_egl_LIBRARY ${OPENGLES_gl_LIBRARY})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenGLES
                                  REQUIRED_VARS OPENGLES_gl_LIBRARY OPENGLES_egl_LIBRARY OPENGLES_INCLUDE_DIR)

if(OPENGLES_FOUND)
  set(OPENGLES_INCLUDE_DIRS ${OPENGLES_INCLUDE_DIR})
  set(OPENGLES_LIBRARIES ${OPENGLES_gl_LIBRARY} ${OPENGLES_egl_LIBRARY})
  set(OPENGLES_DEFINITIONS -DHAVE_LIBGLESV2 -DHAVE_LIBEGL=1)
endif()

mark_as_advanced(OPENGLES_INCLUDE_DIR OPENGLES_gl_LIBRARY OPENGLES_egl_LIBRARY)
