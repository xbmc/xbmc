#.rst:
# FindEGL
# -------
# Finds the EGL library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::EGL   - The EGL library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC egl)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    list(APPEND GL_INTERFACES_LIST egl egl-pb)
    set(GL_INTERFACES_LIST ${GL_INTERFACES_LIST} PARENT_SCOPE)

    set(CMAKE_REQUIRED_INCLUDES "${EGL_INCLUDE_DIR}")
    include(CheckIncludeFiles)
    check_include_files("EGL/egl.h;EGL/eglext.h;EGL/eglext_angle.h" HAVE_EGLEXTANGLE)
    unset(CMAKE_REQUIRED_INCLUDES)

    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    set_target_properties(PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} PROPERTIES
                                                                               IMPORTED_NO_SONAME TRUE)

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_EGL)

    if(HAVE_EGLEXTANGLE)
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAVE_EGLEXTANGLE)
    endif()
    ADD_TARGET_COMPILE_DEFINITION()
  endif()
endif()
