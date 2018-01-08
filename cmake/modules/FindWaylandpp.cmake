# FindWaylandpp
# -------------
# Finds the waylandpp library
#
# This will will define the following variables::
#
# WAYLANDPP_FOUND        - the system has waylandpp
# WAYLANDPP_INCLUDE_DIRS - the waylandpp include directory
# WAYLANDPP_LIBRARIES    - the waylandpp libraries
# WAYLANDPP_DEFINITIONS  - the waylandpp definitions
# WAYLANDPP_SCANNER      - path to wayland-scanner++

pkg_check_modules(WAYLANDPP wayland-client++ wayland-egl++ wayland-cursor++)
pkg_check_modules(PC_WAYLANDPP_SCANNER wayland-scanner++)
if(WAYLANDPP_FOUND)
  pkg_get_variable(PC_WAYLANDPP_PKGDATADIR wayland-client++ pkgdatadir)
endif()
if(PC_WAYLANDPP_SCANNER_FOUND)
  pkg_get_variable(PC_WAYLANDPP_SCANNER wayland-scanner++ wayland_scannerpp)
endif()

# Promote to cache variables so all code can access it
set(WAYLANDPP_PROTOCOLS_DIR "${PC_WAYLANDPP_PKGDATADIR}/protocols" CACHE INTERNAL "")

# wayland-scanner++ is from native/host system in case of cross-compilation, so
# it's ok if we don't find it with pkgconfig
find_program(WAYLANDPP_SCANNER wayland-scanner++ PATHS ${PC_WAYLANDPP_SCANNER})

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (Waylandpp
  REQUIRED_VARS
    WAYLANDPP_FOUND
    WAYLANDPP_SCANNER
  VERSION_VAR
    WAYLANDPP_wayland-client++_VERSION)

set(WAYLANDPP_DEFINITIONS -DHAVE_WAYLAND=1)
# Also pass on library directories
set(WAYLANDPP_LIBRARIES ${WAYLANDPP_LDFLAGS})
