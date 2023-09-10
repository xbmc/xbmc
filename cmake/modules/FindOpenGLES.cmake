#.rst:
# FindOpenGLES
# ------------
# Finds the OpenGLES library
#
# This will define the following target:
#
#   OpenGL::GLES - The OpenGLES IMPORTED library

if(NOT TARGET OpenGL::GLES)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_OPENGLES glesv2 QUIET)
  endif()

  find_library(OPENGLES_gl_LIBRARY NAMES GLESv2 OpenGLES
                                   HINTS ${PC_OPENGLES_LIBDIR} ${CMAKE_OSX_SYSROOT}/System/Library
                                   PATH_SUFFIXES Frameworks
                                   NO_CACHE)
  find_path(OPENGLES_INCLUDE_DIR NAMES GLES2/gl2.h ES2/gl.h
                                 HINTS ${PC_OPENGLES_INCLUDEDIR} ${OPENGLES_gl_LIBRARY}/Headers
                                 NO_CACHE)
  find_path(OPENGLES3_INCLUDE_DIR NAMES GLES3/gl3.h ES3/gl.h
                                  HINTS ${PC_OPENGLES_INCLUDEDIR} ${OPENGLES_gl_LIBRARY}/Headers
                                  NO_CACHE)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(OpenGLES
                                    REQUIRED_VARS OPENGLES_gl_LIBRARY OPENGLES_INCLUDE_DIR)

  if(OPENGLES_FOUND)
    if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
      # Cmake only added support for Frameworks as the IMPORTED_LOCATION as of 3.28
      # https://gitlab.kitware.com/cmake/cmake/-/merge_requests/8586
      # Until we move to cmake 3.28 as minimum, explicitly set to binary inside framework
      if(OPENGLES_gl_LIBRARY MATCHES "/([^/]+)\\.framework$")
        set(_gles_fw "${OPENGLES_gl_LIBRARY}/${CMAKE_MATCH_1}")
        if(EXISTS "${_gles_fw}.tbd")
          string(APPEND _gles_fw ".tbd")
        endif()
        set(OPENGLES_gl_LIBRARY ${_gles_fw})
      endif()
    endif()

    add_library(OpenGL::GLES UNKNOWN IMPORTED)
    set_target_properties(OpenGL::GLES PROPERTIES
                                       IMPORTED_LOCATION "${OPENGLES_gl_LIBRARY}"
                                       INTERFACE_INCLUDE_DIRECTORIES "${OPENGLES_INCLUDE_DIR}")

    if(OPENGLES3_INCLUDE_DIR)
      set_property(TARGET OpenGL::GLES APPEND PROPERTY
                                       INTERFACE_INCLUDE_DIRECTORIES "${OPENGLES3_INCLUDE_DIR}")
      set_target_properties(OpenGL::GLES PROPERTIES
                                         INTERFACE_COMPILE_DEFINITIONS HAS_GLES=3)
    else()
      set_target_properties(OpenGL::GLES PROPERTIES
                                         INTERFACE_COMPILE_DEFINITIONS HAS_GLES=2)
    endif()

    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP OpenGL::GLES)
  endif()
endif()
