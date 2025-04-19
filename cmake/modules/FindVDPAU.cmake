#.rst:
# FindVDPAU
# ---------
# Finds the VDPAU library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::VDPAU   - The VDPAU library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig ${SEARCH_QUIET})
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_VDPAU vdpau ${SEARCH_QUIET})
  endif()

  find_path(VDPAU_INCLUDE_DIR NAMES vdpau/vdpau.h vdpau/vdpau_x11.h
                              HINTS ${PC_VDPAU_INCLUDEDIR})
  find_library(VDPAU_LIBRARY NAMES vdpau
                             HINTS ${PC_VDPAU_LIBDIR})

  set(VDPAU_VERSION ${PC_VDPAU_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(VDPAU
                                    REQUIRED_VARS VDPAU_LIBRARY VDPAU_INCLUDE_DIR
                                    VERSION_VAR VDPAU_VERSION)

  if(VDPAU_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${VDPAU_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${VDPAU_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAVE_LIBVDPAU)
  endif()
endif()
