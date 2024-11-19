#.rst:
# FindXRandR
# ----------
# Finds the XRandR library
#
# This will define the following target:
#
#   XRandR::XRandR   - The XRANDR library

if(NOT TARGET XRandR::XRandR)

  find_package(PkgConfig)

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_XRANDR xrandr QUIET)
  endif()

  find_path(XRANDR_INCLUDE_DIR NAMES X11/extensions/Xrandr.h
                               HINTS ${PC_XRANDR_INCLUDEDIR}
                               NO_CACHE)
  find_library(XRANDR_LIBRARY NAMES Xrandr
                              HINTS ${PC_XRANDR_LIBDIR}
                              NO_CACHE)

  set(XRANDR_VERSION ${PC_XRANDR_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(XRandR
                                    REQUIRED_VARS XRANDR_LIBRARY XRANDR_INCLUDE_DIR
                                    VERSION_VAR XRANDR_VERSION)

  if(XRANDR_FOUND)
    add_library(XRandR::XRandR UNKNOWN IMPORTED)
    set_target_properties(XRandR::XRandR PROPERTIES
                                         IMPORTED_LOCATION "${XRANDR_LIBRARY}"
                                         INTERFACE_INCLUDE_DIRECTORIES "${XRANDR_INCLUDE_DIR}"
                                         INTERFACE_COMPILE_DEFINITIONS HAVE_LIBXRANDR=1)

    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP XRandR::XRandR)
  endif()
endif()
