#.rst:
# FindLibinput
# --------
# Finds the libinput library
#
# This will define the following variables::
#
# LIBINPUT_FOUND - system has libinput
# LIBINPUT_INCLUDE_DIRS - the libinput include directory
# LIBINPUT_LIBRARIES - the libinput libraries
# LIBINPUT_DEFINITIONS - the libinput compile definitions
#

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_LIBINPUT libinput QUIET)
endif()

find_path(LIBINPUT_INCLUDE_DIR NAMES libinput.h
                               HINTS ${PC_LIBINPUT_INCLUDEDIR})

find_library(LIBINPUT_LIBRARY NAMES input
                              HINTS ${PC_LIBINPUT_LIBDIR})

set(LIBINPUT_VERSION ${PC_LIBINPUT_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibInput
                                  REQUIRED_VARS LIBINPUT_LIBRARY LIBINPUT_INCLUDE_DIR
                                  VERSION_VAR LIBINPUT_VERSION)

if(LIBINPUT_FOUND)
  set(LIBINPUT_INCLUDE_DIRS ${LIBINPUT_INCLUDE_DIR})
  set(LIBINPUT_LIBRARIES ${LIBINPUT_LIBRARY})
endif()

mark_as_advanced(LIBINPUT_INCLUDE_DIR LIBINPUT_LIBRARY)
