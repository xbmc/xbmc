#.rst:
# FindGLU
# -----
# Finds the GLU library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::GLU   - The GLU library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  find_package(PkgConfig ${SEARCH_QUIET})

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GLU glu ${SEARCH_QUIET})
  endif()

  find_path(GLU_INCLUDE_DIR NAMES GL/glu.h
                            HINTS ${PC_GLU_INCLUDEDIR})
  find_library(GLU_LIBRARY NAMES GLU
                           HINTS ${PC_GLU_LIBDIR})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(GLU
                                    REQUIRED_VARS GLU_LIBRARY GLU_INCLUDE_DIR)

  if(GLU_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${GLU_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${GLU_INCLUDE_DIR}")
  endif()

endif()
