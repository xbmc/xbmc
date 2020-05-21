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

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CDIO libcdio>=0.80 QUIET)
endif()

find_path(CDIO_INCLUDE_DIR NAMES cdio/cdio.h
                           PATHS ${PC_CDIO_INCLUDEDIR})

find_library(CDIO_LIBRARY NAMES cdio libcdio
                          PATHS ${PC_CDIO_LIBDIR})

set(CDIO_VERSION ${PC_CDIO_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cdio
                                  REQUIRED_VARS CDIO_LIBRARY CDIO_INCLUDE_DIR
                                  VERSION_VAR CDIO_VERSION)

if(CDIO_FOUND)
  set(CDIO_LIBRARIES ${CDIO_LIBRARY})
  set(CDIO_INCLUDE_DIRS ${CDIO_INCLUDE_DIR})
endif()

mark_as_advanced(CDIO_INCLUDE_DIR CDIO_LIBRARY)
