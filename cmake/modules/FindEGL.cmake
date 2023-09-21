#.rst:
# FindEGL
# -------
# Finds the EGL library
#
# This will define the following target:
#
#   EGL::EGL   - The EGL library

if(NOT TARGET EGL::EGL)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_EGL egl QUIET)
  endif()

  find_path(EGL_INCLUDE_DIR EGL/egl.h
                            HINTS ${PC_EGL_INCLUDEDIR}
                            NO_CACHE)

  find_library(EGL_LIBRARY NAMES EGL egl
                           HINTS ${PC_EGL_LIBDIR}
                           NO_CACHE)

  set(EGL_VERSION ${PC_EGL_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(EGL
                                    REQUIRED_VARS EGL_LIBRARY EGL_INCLUDE_DIR
                                    VERSION_VAR EGL_VERSION)

  if(EGL_FOUND)
    include(CheckIncludeFiles)
    check_include_files("EGL/egl.h;EGL/eglext.h;EGL/eglext_angle.h" HAVE_EGLEXTANGLE)

    add_library(EGL::EGL UNKNOWN IMPORTED)
    set_target_properties(EGL::EGL PROPERTIES
                                   IMPORTED_LOCATION "${EGL_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${EGL_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAS_EGL=1)

    if(HAVE_EGLEXTANGLE)
      set_property(TARGET EGL::EGL APPEND PROPERTY
                                          INTERFACE_COMPILE_DEFINITIONS HAVE_EGLEXTANGLE=1)
    endif()
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP EGL::EGL)
  endif()
endif()
