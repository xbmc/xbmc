#.rst:
# FindOpenGLES
# ------------
# Finds the OpenGLES library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::OpenGLES - The OpenGLES IMPORTED library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig ${SEARCH_QUIET})
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_OPENGLES glesv2 ${SEARCH_QUIET})
  endif()

  find_library(OPENGLES_gl_LIBRARY NAMES GLESv2 OpenGLES
                                   HINTS ${PC_OPENGLES_LIBDIR} ${CMAKE_OSX_SYSROOT}/System/Library
                                   PATH_SUFFIXES Frameworks)
  find_path(OPENGLES_INCLUDE_DIR NAMES GLES2/gl2.h ES2/gl.h
                                 HINTS ${PC_OPENGLES_INCLUDEDIR} ${OPENGLES_gl_LIBRARY}/Headers)
  find_path(OPENGLES3_INCLUDE_DIR NAMES GLES3/gl3.h ES3/gl.h
                                  HINTS ${PC_OPENGLES_INCLUDEDIR} ${OPENGLES_gl_LIBRARY}/Headers)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(OpenGLES
                                    REQUIRED_VARS OPENGLES_gl_LIBRARY OPENGLES_INCLUDE_DIR)

  if(OPENGLES_FOUND)
    if(${OPENGLES_gl_LIBRARY} MATCHES ".+\.so$")
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} SHARED IMPORTED)
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    endif()

    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${OPENGLES_gl_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${OPENGLES_INCLUDE_DIR}"
                                                                     IMPORTED_NO_SONAME TRUE)

    if(OPENGLES3_INCLUDE_DIR)
      set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                            INTERFACE_INCLUDE_DIRECTORIES "${OPENGLES3_INCLUDE_DIR}")
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_COMPILE_DEFINITIONS HAS_GLES=3)
    else()
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_COMPILE_DEFINITIONS HAS_GLES=2)
    endif()
  endif()
endif()
