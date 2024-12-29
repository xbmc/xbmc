#.rst:
# FindEGL
# -------
# Finds the EGL library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::EGL   - The EGL library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_EGL egl QUIET)
  endif()

  find_path(EGL_INCLUDE_DIR EGL/egl.h
                            HINTS ${PC_EGL_INCLUDEDIR})

  find_library(EGL_LIBRARY NAMES EGL egl
                           HINTS ${PC_EGL_LIBDIR})

  set(EGL_VERSION ${PC_EGL_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(EGL
                                    REQUIRED_VARS EGL_LIBRARY EGL_INCLUDE_DIR
                                    VERSION_VAR EGL_VERSION)

  if(EGL_FOUND)
    list(APPEND GL_INTERFACES_LIST egl egl-pb)
    set(GL_INTERFACES_LIST ${GL_INTERFACES_LIST} PARENT_SCOPE)

    set(CMAKE_REQUIRED_INCLUDES "${EGL_INCLUDE_DIR}")
    include(CheckIncludeFiles)
    check_include_files("EGL/egl.h;EGL/eglext.h;EGL/eglext_angle.h" HAVE_EGLEXTANGLE)
    unset(CMAKE_REQUIRED_INCLUDES)

    if(${EGL_LIBRARY} MATCHES ".+\.so$")
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} SHARED IMPORTED)
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    endif()

    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${EGL_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${EGL_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAS_EGL
                                                                     IMPORTED_NO_SONAME TRUE)

    if(HAVE_EGLEXTANGLE)
      set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                            INTERFACE_COMPILE_DEFINITIONS HAVE_EGLEXTANGLE)
    endif()
  endif()
endif()
