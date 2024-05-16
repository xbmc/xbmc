# FindDovi
# -------
# Finds the libdovi library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::LibDovi   - The libDovi library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  find_package(PkgConfig)

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_LIBDOVI libdovi QUIET)
  endif()

  find_library(LIBDOVI_LIBRARY NAMES dovi libdovi
                               HINTS ${PC_LIBDOVI_LIBDIR})
  find_path(LIBDOVI_INCLUDE_DIR NAMES libdovi/rpu_parser.h
                                HINTS ${PC_LIBDOVI_INCLUDEDIR})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LibDovi
                                    REQUIRED_VARS LIBDOVI_LIBRARY LIBDOVI_INCLUDE_DIR)

  if(LIBDOVI_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${LIBDOVI_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${LIBDOVI_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAVE_LIBDOVI)
  endif()
endif()
