#.rst:
# FindGLX
# -----
# Finds the GLX library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::GLX    - The GLX library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig ${SEARCH_QUIET})

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GLX glx ${SEARCH_QUIET})
  endif()

  find_path(GLX_INCLUDE_DIR NAMES GL/glx.h
                            HINTS ${PC_GLX_INCLUDEDIR})
  find_library(GLX_LIBRARY NAMES GL
                           HINTS ${PC_GLX_LIBDIR})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(GLX
                                    REQUIRED_VARS GLX_LIBRARY GLX_INCLUDE_DIR)

  if(GLX_FOUND)
    list(APPEND GL_INTERFACES_LIST glx)
    set(GL_INTERFACES_LIST ${GL_INTERFACES_LIST} PARENT_SCOPE)

    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${GLX_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${GLX_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAS_GLX)
  endif()
endif()
