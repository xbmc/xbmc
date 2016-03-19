# - Try to find xrandr
# Once done this will define
#
# XRANDR_FOUND - system has lixrandr
# XRANDR_INCLUDE_DIRS - the libxrandr include directory
# XRANDR_LIBRARIES - The libxrandr libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (XRANDR xrandr)
  list(APPEND XRANDR_INCLUDE_DIRS ${XRANDR_INCLUDEDIR})
else()
  find_library(XRANDR_LIBRARIES Xrandr)
endif()

if(XRANDR_FOUND)
include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(XRandR DEFAULT_MSG XRANDR_INCLUDE_DIRS XRANDR_LIBRARIES)

  list(APPEND XRANDR_DEFINITIONS -DHAVE_LIBXRANDR=1)

  mark_as_advanced(XRANDR_INCLUDE_DIRS XRANDR_LIBRARIES XRANDR_DEFINITIONS)
endif()
