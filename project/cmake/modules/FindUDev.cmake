# - Try to find udev
# Once done this will define
#
# UDEV_FOUND - system has libudev
# UDEV_INCLUDE_DIRS - the libudev include directory
# UDEV_LIBRARIES - The libudev libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (UDEV libudev)
  list(APPEND UDEV_INCLUDE_DIRS ${UDEV_INCLUDEDIR})
endif()

if(NOT UDEV_FOUND)
  find_path(UDEV_INCLUDE_DIRS libudev.h)
  find_library(UDEV_LIBRARIES udev)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UDev DEFAULT_MSG UDEV_INCLUDE_DIRS UDEV_LIBRARIES)

mark_as_advanced(UDEV_INCLUDE_DIRS UDEV_LIBRARIES)
list(APPEND UDEV_DEFINITIONS -DHAVE_LIBUDEV=1)
