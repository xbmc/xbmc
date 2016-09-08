#.rst:
# FindXRandR
# ----------
# Finds the XRandR library
#
# This will will define the following variables::
#
# XRANDR_FOUND - system has XRANDR
# XRANDR_INCLUDE_DIRS - the XRANDR include directory
# XRANDR_LIBRARIES - the XRANDR libraries
# XRANDR_DEFINITIONS - the XRANDR definitions
#
# and the following imported targets::
#
#   XRandR::XRandR   - The XRANDR library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_XRANDR xrandr QUIET)
endif()

find_path(XRANDR_INCLUDE_DIR NAMES X11/extensions/Xrandr.h
                             PATHS ${PC_XRANDR_INCLUDEDIR})
find_library(XRANDR_LIBRARY NAMES Xrandr
                            PATHS ${PC_XRANDR_LIBDIR})

set(XRANDR_VERSION ${PC_XRANDR_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XRandR
                                  REQUIRED_VARS XRANDR_LIBRARY XRANDR_INCLUDE_DIR
                                  VERSION_VAR XRANDR_VERSION)

if(XRANDR_FOUND)
  set(XRANDR_LIBRARIES ${XRANDR_LIBRARY})
  set(XRANDR_INCLUDE_DIRS ${XRANDR_INCLUDE_DIR})
  set(XRANDR_DEFINITIONS -DHAVE_LIBXRANDR=1)

  if(NOT TARGET XRandR::XRandR)
    add_library(XRandR::XRandR UNKNOWN IMPORTED)
    set_target_properties(XRandR::XRandR PROPERTIES
                                         IMPORTED_LOCATION "${XRANDR_LIBRARY}"
                                         INTERFACE_INCLUDE_DIRECTORIES "${XRANDR_INCLUDE_DIR}"
                                         INTERFACE_COMPILE_DEFINITIONS HAVE_LIBXRANDR=1)
  endif()
endif()

mark_as_advanced(XRANDR_INCLUDE_DIR XRANDR_LIBRARY)
