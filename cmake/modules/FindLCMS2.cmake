#.rst:
# FindLCMS2
# -----------
# Finds the LCMS Color Management library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::LCMS2 - The LCMS Color Management library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  SETUP_FIND_SPECS()

  find_package(PkgConfig ${SEARCH_QUIET})
  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
    pkg_check_modules(PC_LCMS2 lcms2${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET})
  endif()

  find_path(LCMS2_INCLUDE_DIR NAMES lcms2.h
                              HINTS ${DEPENDS_PATH}/include ${PC_LCMS2_INCLUDEDIR})
  find_library(LCMS2_LIBRARY NAMES lcms2 liblcms2
                             HINTS ${DEPENDS_PATH}/lib ${PC_LCMS2_LIBDIR})

  set(LCMS2_VERSION ${PC_LCMS2_VERSION})

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LCMS2
                                    REQUIRED_VARS LCMS2_LIBRARY LCMS2_INCLUDE_DIR
                                    VERSION_VAR LCMS2_VERSION)

  if(LCMS2_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${LCMS2_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${LCMS2_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS "HAVE_LCMS2;CMS_NO_REGISTER_KEYWORD")
  endif()
endif()
