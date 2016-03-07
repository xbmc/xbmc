#.rst:
# FindVDPAU
# ---------
# Finds the VDPAU library
#
# This will will define the following variables::
#
# VDPAU_FOUND - system has VDPAU
# VDPAU_INCLUDE_DIRS - the VDPAU include directory
# VDPAU_LIBRARIES - the VDPAU libraries
# VDPAU_DEFINITIONS - the VDPAU definitions

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_VDPAU vdpau QUIET)
endif()

find_path(VDPAU_INCLUDE_DIR NAMES vdpau/vdpau.h vdpau/vdpau_x11.h
                            PATHS ${PC_VDPAU_INCLUDEDIR})
find_library(VDPAU_LIBRARY NAMES vdpau
                           PATHS ${PC_VDPAU_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VDPAU
                                  REQUIRED_VARS VDPAU_LIBRARY VDPAU_INCLUDE_DIR)

if(VDPAU_FOUND)
  set(VDPAU_INCLUDE_DIRS ${VDPAU_INCLUDE_DIR})
  set(VDPAU_LIBRARIES ${VDPAU_LIBRARY})
  set(VDPAU_DEFINITIONS -DHAVE_LIBVDPAU=1)
endif()

mark_as_advanced(VDPAU_INCLUDE_DIR VDPAU_LIBRARY)
