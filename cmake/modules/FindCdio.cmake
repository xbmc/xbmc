#.rst:
# FindCdio
# --------
# Finds the cdio library
#
# This will define the following target:
#
# ${APP_NAME_LC}::Cdio - The LibCap library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig ${SEARCH_QUIET})

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_CDIO libcdio>=0.80 ${SEARCH_QUIET})
    pkg_check_modules(PC_CDIOPP libcdio++>=2.1.0 ${SEARCH_QUIET})
  endif()

  find_path(CDIO_INCLUDE_DIR NAMES cdio/cdio.h
                             HINTS ${PC_CDIO_INCLUDEDIR})

  find_library(CDIO_LIBRARY NAMES cdio libcdio
                            HINTS ${PC_CDIO_LIBDIR})

  if(DEFINED PC_CDIO_VERSION AND DEFINED PC_CDIOPP_VERSION AND NOT "${PC_CDIO_VERSION}" VERSION_EQUAL "${PC_CDIOPP_VERSION}")
    message(WARNING "Detected libcdio (${PC_CDIO_VERSION}) and libcdio++ (${PC_CDIOPP_VERSION}) version mismatch. libcdio++ will not be used.")
  else()
    find_path(CDIOPP_INCLUDE_DIR NAMES cdio++/cdio.hpp
                                 HINTS ${PC_CDIOPP_INCLUDEDIR} ${CDIO_INCLUDE_DIR})

    set(CDIO_VERSION ${PC_CDIO_VERSION})
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Cdio
                                    REQUIRED_VARS CDIO_LIBRARY CDIO_INCLUDE_DIR
                                    VERSION_VAR CDIO_VERSION)

  if(CDIO_FOUND)

    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${CDIO_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${CDIO_INCLUDE_DIR}")

    if(CDIOPP_INCLUDE_DIR)
      add_library(${APP_NAME_LC}::CdioPP INTERFACE IMPORTED)
      set_target_properties(${APP_NAME_LC}::CdioPP PROPERTIES
                                                   INTERFACE_INCLUDE_DIRECTORIES "${CDIOPP_INCLUDE_DIR}")
      set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                            INTERFACE_LINK_LIBRARIES "${APP_NAME_LC}::CdioPP")
    endif()
  else()
    if(Cdio_FIND_REQUIRED)
      message(FATAL_ERROR "cdio library not found.")
    endif()
  endif()
endif()
