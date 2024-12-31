#.rst:
# FindPlist
# ---------
# Finds the Plist library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Plist - The Plist library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
    pkg_search_module(PC_PLIST libplist-2.0 libplist QUIET)
  endif()

  find_path(PLIST_INCLUDE_DIR plist/plist.h
                              HINTS ${DEPENDS_PATH}/include ${PC_PLIST_INCLUDEDIR}
                              ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})
  find_library(PLIST_LIBRARY NAMES plist-2.0 plist libplist-2.0 libplist
                             HINTS ${DEPENDS_PATH}/lib ${PC_PLIST_LIBDIR}
                             ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  set(PLIST_VERSION ${PC_PLIST_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Plist
                                    REQUIRED_VARS PLIST_LIBRARY PLIST_INCLUDE_DIR
                                    VERSION_VAR PLIST_VERSION)

  if(PLIST_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${PLIST_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${PLIST_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAS_AIRPLAY)
  endif()
endif()
