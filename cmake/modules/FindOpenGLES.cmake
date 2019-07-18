#.rst:
# FindOpenGLES
# ------------
# Finds the OpenGLES2 library
#
# This will define the following variables::
#
# OPENGLES_FOUND - system has OpenGLES
# OPENGLES_INCLUDE_DIRS - the OpenGLES include directory
# OPENGLES_LIBRARIES - the OpenGLES libraries
# OPENGLES_DEFINITIONS - the OpenGLES definitions

if(CORE_PLATFORM_NAME_LC STREQUAL rbpi)
    set(_brcmprefix brcm)
endif()

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_OPENGLES ${_brcmprefix}glesv2 QUIET)
endif()

if(NOT CORE_SYSTEM_NAME STREQUAL darwin_embedded)
  find_path(OPENGLES_INCLUDE_DIR GLES2/gl2.h
                                 PATHS ${PC_OPENGLES_INCLUDEDIR})
  find_library(OPENGLES_gl_LIBRARY NAMES ${_brcmprefix}GLESv2
                                   PATHS ${PC_OPENGLES_LIBDIR})
else()
  find_library(OPENGLES_gl_LIBRARY NAMES OpenGLES
                                   PATHS ${CMAKE_OSX_SYSROOT}/System/Library
                                   PATH_SUFFIXES Frameworks
                                   NO_DEFAULT_PATH)
  set(OPENGLES_INCLUDE_DIR ${OPENGLES_gl_LIBRARY}/Headers)
endif()

find_path(OPENGLES3_INCLUDE_DIR GLES3/gl3.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenGLES
                                  REQUIRED_VARS OPENGLES_gl_LIBRARY OPENGLES_INCLUDE_DIR)

find_path(OPENGLES3_INCLUDE_DIR GLES3/gl3.h
                                PATHS ${PC_OPENGLES_INCLUDEDIR})

if(OPENGLES_FOUND)
  set(OPENGLES_LIBRARIES ${OPENGLES_gl_LIBRARY})
  if(OPENGLES3_INCLUDE_DIR)
    set(OPENGLES_INCLUDE_DIRS ${OPENGLES_INCLUDE_DIR} ${OPENGLES3_INCLUDE_DIR})
    set(OPENGLES_DEFINITIONS -DHAS_GLES=3)
    mark_as_advanced(OPENGLES_INCLUDE_DIR OPENGLES3_INCLUDE_DIR OPENGLES_gl_LIBRARY)
  else()
    set(OPENGLES_INCLUDE_DIRS ${OPENGLES_INCLUDE_DIR})
    set(OPENGLES_DEFINITIONS -DHAS_GLES=2)
    mark_as_advanced(OPENGLES_INCLUDE_DIR OPENGLES_gl_LIBRARY)
  endif()
endif()
