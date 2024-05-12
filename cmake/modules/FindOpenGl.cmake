#.rst:
# FindOpenGl
# ----------
# Finds the FindOpenGl library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::OpenGl - The OpenGL library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_OPENGL gl QUIET)
  endif()

  find_library(OPENGL_gl_LIBRARY NAMES GL OpenGL
                                 HINTS ${PC_OPENGL_gl_LIBDIR} ${CMAKE_OSX_SYSROOT}/System/Library
                                 PATH_SUFFIXES Frameworks)
  find_path(OPENGL_INCLUDE_DIR NAMES GL/gl.h gl.h
                               HINTS ${PC_OPENGL_gl_INCLUDEDIR} ${OPENGL_gl_LIBRARY}/Headers)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(OpenGl
                                    REQUIRED_VARS OPENGL_gl_LIBRARY OPENGL_INCLUDE_DIR)

  if(OPENGL_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${OPENGL_gl_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${OPENGL_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAS_GL)
  endif()
endif()
