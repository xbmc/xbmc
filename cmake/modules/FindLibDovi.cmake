# FindDovi
# -------
# Finds the libdovi library
#
# This will define the following variables::
#
# LIBDOVI_FOUND - system has libdovi
# LIBDOVI_INCLUDE_DIRS - the libdovi include directories
# LIBDOVI_LIBRARIES - the libdovi libraries
# LIBDOVI_DEFINITIONS - the libdovi compile definitions

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_LIBDOVI libdovi QUIET)
endif()

find_library(LIBDOVI_LIBRARY NAMES dovi libdovi
                             HINTS ${PC_LIBDOVI_LIBDIR}
)
find_path(LIBDOVI_INCLUDE_DIR NAMES libdovi/rpu_parser.h
                              HINTS ${PC_LIBDOVI_INCLUDEDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibDovi
                                  REQUIRED_VARS LIBDOVI_LIBRARY LIBDOVI_INCLUDE_DIR)

if(LIBDOVI_FOUND)
  set(LIBDOVI_INCLUDE_DIRS ${LIBDOVI_INCLUDE_DIR})
  set(LIBDOVI_LIBRARIES ${LIBDOVI_LIBRARY})
  set(LIBDOVI_DEFINITIONS -DHAVE_LIBDOVI=1)
endif()

mark_as_advanced(LIBDOVI_INCLUDE_DIR LIBDOVI_LIBRARY)
