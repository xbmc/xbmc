#.rst:
# FindCdio
# --------
# Finds the cdio library
#
# This will define the following variables::
#
# CDIO_FOUND - system has cdio
# CDIO_INCLUDE_DIRS - the cdio include directory
# CDIO_LIBRARIES - the cdio libraries

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CDIO libcdio>=0.80 QUIET)
  pkg_check_modules(PC_CDIOPP libcdio++>=2.1.0 QUIET)
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
  set(CDIO_LIBRARIES ${CDIO_LIBRARY})
  set(CDIO_INCLUDE_DIRS ${CDIO_INCLUDE_DIR})
endif()

mark_as_advanced(CDIO_INCLUDE_DIR CDIOPP_INCLUDE_DIR CDIO_LIBRARY)
