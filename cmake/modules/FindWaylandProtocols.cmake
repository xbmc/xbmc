# FindWaylandProtocols
# --------------------
# Find wayland-protocols
#
# This will define the following variables::
#
# WAYLAND_PROTOCOLS_DIR - directory containing the additional Wayland protocols
#                         from the wayland-protocols package
# WAYLAND_PROTOCOLS_XMLS - the protocol descriptions C++ wrappers are generated from

find_package(PkgConfig ${SEARCH_QUIET})
pkg_check_modules(PC_WAYLAND_PROTOCOLS wayland-protocols ${SEARCH_QUIET})
if(PC_WAYLAND_PROTOCOLS_FOUND)
  pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

  # pkgdatadir is defined in terms of ${pc_sysrootdir}, which pkg-config expands to "/" when
  # no sysroot is set, leaving a doubled leading separator
  string(REGEX REPLACE "^/+" "/" WAYLAND_PROTOCOLS_DIR "${WAYLAND_PROTOCOLS_DIR}")

  set(WAYLAND_PROTOCOLS_XMLS "${WAYLAND_PROTOCOLS_DIR}/stable/viewporter/viewporter.xml"
                             "${WAYLAND_PROTOCOLS_DIR}/staging/fractional-scale/fractional-scale-v1.xml"
                             "${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-shell/xdg-shell-unstable-v6.xml"
                             "${WAYLAND_PROTOCOLS_DIR}/unstable/idle-inhibit/idle-inhibit-unstable-v1.xml")

  if(PC_WAYLAND_PROTOCOLS_VERSION VERSION_GREATER_EQUAL 1.41)
    list(APPEND WAYLAND_PROTOCOLS_XMLS "${WAYLAND_PROTOCOLS_DIR}/staging/color-management/color-management-v1.xml")
  endif()

  # pkg-config finding the package is not evidence the descriptions were installed with it
  foreach(xml IN LISTS WAYLAND_PROTOCOLS_XMLS)
    if(NOT EXISTS "${xml}")
      list(APPEND _missing_xmls "${xml}")
    endif()
  endforeach()

  if(_missing_xmls)
    list(JOIN _missing_xmls "\n    " _missing_xmls)
    set(WAYLAND_PROTOCOLS_REASON "wayland-protocols ${PC_WAYLAND_PROTOCOLS_VERSION} was found via pkg-config, but these protocol descriptions are not installed:\n    ${_missing_xmls}")
    unset(WAYLAND_PROTOCOLS_XMLS)
    unset(_missing_xmls)
  endif()
endif()

# Promote to cache variables so all code can access it
set(WAYLAND_PROTOCOLS_DIR ${WAYLAND_PROTOCOLS_DIR} CACHE INTERNAL "")
set(WAYLAND_PROTOCOLS_XMLS ${WAYLAND_PROTOCOLS_XMLS} CACHE INTERNAL "")

if(NOT VERBOSE_FIND)
   set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
 endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WaylandProtocols
  REQUIRED_VARS
    PC_WAYLAND_PROTOCOLS_FOUND
    WAYLAND_PROTOCOLS_DIR
    WAYLAND_PROTOCOLS_XMLS
  VERSION_VAR
    PC_WAYLAND_PROTOCOLS_VERSION
  REASON_FAILURE_MESSAGE
    "${WAYLAND_PROTOCOLS_REASON}")
