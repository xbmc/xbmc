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
#
# and the following imported targets::
#
#   VDPAU::VDPAU   - The VDPAU library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_VDPAU vdpau QUIET)
endif()

find_path(VDPAU_INCLUDE_DIR NAMES vdpau/vdpau.h vdpau/vdpau_x11.h
                            PATHS ${PC_VDPAU_INCLUDEDIR})
find_library(VDPAU_LIBRARY NAMES vdpau
                           PATHS ${PC_VDPAU_LIBDIR})

set(VDPAU_VERSION ${PC_VDPAU_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VDPAU
                                  REQUIRED_VARS VDPAU_LIBRARY VDPAU_INCLUDE_DIR
                                  VERSION_VAR VDPAU_VERSION)

if(VDPAU_FOUND)
  set(VDPAU_INCLUDE_DIRS ${VDPAU_INCLUDE_DIR})
  set(VDPAU_LIBRARIES ${VDPAU_LIBRARY})
  set(VDPAU_DEFINITIONS -DHAVE_LIBVDPAU=1)

  if(NOT TARGET VDPAU::VDPAU)
    add_library(VDPAU::VDPAU UNKNOWN IMPORTED)
    set_target_properties(VDPAU::VDPAU PROPERTIES
                                       IMPORTED_LOCATION "${VDPAU_LIBRARY}"
                                       INTERFACE_INCLUDE_DIRECTORIES "${VDPAU_INCLUDE_DIR}"
                                       INTERFACE_COMPILE_DEFINITIONS HAVE_LIBVDPAU=1)
  endif()
endif()

mark_as_advanced(VDPAU_INCLUDE_DIR VDPAU_LIBRARY)
