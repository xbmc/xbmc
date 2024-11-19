#.rst:
# FindVDPAU
# ---------
# Finds the VDPAU library
#
# This will define the following target:
#
#   VDPAU::VDPAU   - The VDPAU library

if(NOT TARGET VDPAU::VDPAU)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_VDPAU vdpau QUIET)
  endif()

  find_path(VDPAU_INCLUDE_DIR NAMES vdpau/vdpau.h vdpau/vdpau_x11.h
                              HINTS ${PC_VDPAU_INCLUDEDIR}
                              NO_CACHE)
  find_library(VDPAU_LIBRARY NAMES vdpau
                             HINTS ${PC_VDPAU_LIBDIR}
                             NO_CACHE)

  set(VDPAU_VERSION ${PC_VDPAU_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(VDPAU
                                    REQUIRED_VARS VDPAU_LIBRARY VDPAU_INCLUDE_DIR
                                    VERSION_VAR VDPAU_VERSION)

  if(VDPAU_FOUND)
    add_library(VDPAU::VDPAU UNKNOWN IMPORTED)
    set_target_properties(VDPAU::VDPAU PROPERTIES
                                       IMPORTED_LOCATION "${VDPAU_LIBRARY}"
                                       INTERFACE_INCLUDE_DIRECTORIES "${VDPAU_INCLUDE_DIR}"
                                       INTERFACE_COMPILE_DEFINITIONS HAVE_LIBVDPAU=1)
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP VDPAU::VDPAU)
  endif()
endif()
