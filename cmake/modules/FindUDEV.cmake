#.rst:
# FindUDEV
# -------
# Finds the UDEV library
#
# This will define the following variables::
#
# UDEV_FOUND - system has UDEV
# UDEV_INCLUDE_DIRS - the UDEV include directory
# UDEV_LIBRARIES - the UDEV libraries
# UDEV_DEFINITIONS - the UDEV definitions
#
# and the following imported targets::
#
#   UDEV::UDEV   - The UDEV library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_UDEV libudev QUIET)
endif()

find_path(UDEV_INCLUDE_DIR NAMES libudev.h
                           PATHS ${PC_UDEV_INCLUDEDIR})
find_library(UDEV_LIBRARY NAMES udev
                          PATHS ${PC_UDEV_LIBDIR})

set(UDEV_VERSION ${PC_UDEV_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UDEV
                                  REQUIRED_VARS UDEV_LIBRARY UDEV_INCLUDE_DIR
                                  VERSION_VAR UDEV_VERSION)

if(UDEV_FOUND)
  set(UDEV_LIBRARIES ${UDEV_LIBRARY})
  set(UDEV_INCLUDE_DIRS ${UDEV_INCLUDE_DIR})
  set(UDEV_DEFINITIONS -DHAVE_LIBUDEV=1)

  if(NOT TARGET UDEV::UDEV)
    add_library(UDEV::UDEV UNKNOWN IMPORTED)
    set_target_properties(UDEV::UDEV PROPERTIES
                                   IMPORTED_LOCATION "${UDEV_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${UDEV_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAVE_LIBUDEV=1)
  endif()
endif()

mark_as_advanced(UDEV_INCLUDE_DIR UDEV_LIBRARY)
