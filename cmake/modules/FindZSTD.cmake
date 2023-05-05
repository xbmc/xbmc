#.rst:
# FindZSTD
# --------
# Finds the ZSTD library
#
# This will define the following variables::
#
# ZSTD_FOUND - system has ZSTD
# ZSTD_INCLUDE_DIRS - the ZSTD include directory
# ZSTD_LIBRARIES - the ZSTD libraries
#

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_LIBZSTD libzstd QUIET)
endif()

find_path(ZSTD_INCLUDE_DIR NAMES zstd.h
                           PATHS ${PC_LIBZSTD_INCLUDEDIR})

find_library(ZSTD_LIBRARY NAMES zstd libzstd
                          PATHS ${PC_LIBZSTD_LIBDIR})

set(ZSTD_VERSION ${PC_LIBZSTD_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZSTD
                                  REQUIRED_VARS ZSTD_LIBRARY ZSTD_INCLUDE_DIR
                                  VERSION_VAR ZSTD_VERSION)

if(ZSTD_FOUND)
  set(ZSTD_LIBRARIES ${ZSTD_LIBRARY})
  set(ZSTD_INCLUDE_DIRS ${ZSTD_INCLUDE_DIR})
endif()

mark_as_advanced(ZSTD_INCLUDE_DIR ZSTD_LIBRARY)
