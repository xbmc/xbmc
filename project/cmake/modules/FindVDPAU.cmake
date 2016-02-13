# -*- cmake -*-

# - Find VDPAU
# Find the VDPAU includes and library
# This module defines
#  VDPAU_INCLUDE_DIRS, where to find .h
#  VDPAU_LIBRARIES, the libraries needed to use VDPAU.
#  VDPAU_FOUND, If false, do not try to use VDPAU.

if(PKG_CONFIG_FOUND)
  pkg_check_modules(VDPAU vdpau)
else()
  find_path(VDPAU_INCLUDE_DIRS vdpau/vdpau.h vdpau/vdpau_x11.h)
  find_library(VDPAU_LIBRARIES NAMES vdpau)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VDPAU DEFAULT_MSG VDPAU_LIBRARIES)

list(APPEND VDPAU_DEFINITIONS -DHAVE_LIBVDPAU=1)

mark_as_advanced(VDPAU_INCLUDE_DIRS VDPAU_LIBRARIES VDPAU_DEFINITIONS)
