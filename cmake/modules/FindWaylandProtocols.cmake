# FindWaylandProtocols
# --------------------
# Find wayland-protocols
#
# This will define the following variables::
#
# WAYLAND_PROTOCOLS_DIR - directory containing the additional Wayland protocols
#                         from the wayland-protocols package

find_package(PkgConfig)
pkg_check_modules(PC_WAYLAND_PROTOCOLS wayland-protocols)
if(PC_WAYLAND_PROTOCOLS_FOUND)
  pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
endif()

# Promote to cache variables so all code can access it
set(WAYLAND_PROTOCOLS_DIR ${WAYLAND_PROTOCOLS_DIR} CACHE INTERNAL "")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WaylandProtocols
  REQUIRED_VARS
    PC_WAYLAND_PROTOCOLS_FOUND
    WAYLAND_PROTOCOLS_DIR
  VERSION_VAR
    PC_WAYLAND_PROTOCOLS_VERSION)
