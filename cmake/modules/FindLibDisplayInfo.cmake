#.rst:
# FindLibDisplayInfo
# -------
# Finds the libdisplay-info library
#
# This will define the following variables::
#
# LIBDISPLAYINFO_FOUND - system has LIBDISPLAY-INFO
# LIBDISPLAYINFO_INCLUDE_DIRS - the LIBDISPLAY-INFO include directory
# LIBDISPLAYINFO_LIBRARIES - the LIBDISPLAY-INFO libraries
# LIBDISPLAYINFO_DEFINITIONS - the LIBDISPLAY-INFO definitions
#

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_LIBDISPLAYINFO libdisplay-info QUIET)
endif()

find_path(LIBDISPLAYINFO_INCLUDE_DIR libdisplay-info/edid.h
                          HINTS ${PC_LIBDISPLAYINFO_INCLUDEDIR})

find_library(LIBDISPLAYINFO_LIBRARY NAMES display-info
                         HINTS ${PC_LIBDISPLAYINFO_LIBDIR})

set(LIBDISPLAYINFO_VERSION ${PC_LIBDISPLAYINFO_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibDisplayInfo
                                  REQUIRED_VARS LIBDISPLAYINFO_LIBRARY LIBDISPLAYINFO_INCLUDE_DIR
                                  VERSION_VAR LIBDISPLAYINFO_VERSION)

if(LIBDISPLAYINFO_FOUND)
  set(LIBDISPLAYINFO_LIBRARIES ${LIBDISPLAYINFO_LIBRARY})
  set(LIBDISPLAYINFO_INCLUDE_DIRS ${LIBDISPLAYINFO_INCLUDE_DIR})
endif()

mark_as_advanced(LIBDISPLAYINFO_INCLUDE_DIR LIBDISPLAYINFO_LIBRARY)
