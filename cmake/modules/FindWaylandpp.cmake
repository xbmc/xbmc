# FindWaylandpp
# -------------
# Finds the waylandpp library
#
# This will define the following variables::
#
# WAYLANDPP_FOUND        - the system has waylandpp
# WAYLANDPP_INCLUDE_DIRS - the waylandpp include directory
# WAYLANDPP_LIBRARIES    - the waylandpp libraries
# WAYLANDPP_DEFINITIONS  - the waylandpp definitions
# WAYLANDPP_SCANNER      - path to wayland-scanner++

pkg_check_modules(PC_WAYLANDPP wayland-client++ wayland-egl++ wayland-cursor++ QUIET)
pkg_check_modules(PC_WAYLANDPP_SCANNER wayland-scanner++ QUIET)
if(PC_WAYLANDPP_FOUND)
  pkg_get_variable(PC_WAYLANDPP_PKGDATADIR wayland-client++ pkgdatadir)
endif()
if(PC_WAYLANDPP_SCANNER_FOUND)
  pkg_get_variable(PC_WAYLANDPP_SCANNER wayland-scanner++ wayland_scannerpp)
endif()

find_path(WAYLANDPP_INCLUDE_DIR wayland-client.hpp PATHS ${PC_WAYLANDPP_INCLUDEDIR})

find_library(WAYLANDPP_CLIENT_LIBRARY NAMES wayland-client++
                                      PATHS ${PC_WAYLANDPP_LIBRARY_DIRS})

find_library(WAYLANDPP_CURSOR_LIBRARY NAMES wayland-cursor++
                                      PATHS ${PC_WAYLANDPP_LIBRARY_DIRS})

find_library(WAYLANDPP_EGL NAMES wayland-egl++
                           PATHS ${PC_WAYLANDPP_LIBRARY_DIRS})


# Promote to cache variables so all code can access it
set(WAYLANDPP_PROTOCOLS_DIR "${PC_WAYLANDPP_PKGDATADIR}/protocols" CACHE INTERNAL "")

# wayland-scanner++ is from native/host system in case of cross-compilation, so
# it's ok if we don't find it with pkgconfig
find_program(WAYLANDPP_SCANNER wayland-scanner++ PATHS ${PC_WAYLANDPP_SCANNER})

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args(Waylandpp
                                  REQUIRED_VARS WAYLANDPP_INCLUDE_DIR
                                                WAYLANDPP_CLIENT_LIBRARY
                                                WAYLANDPP_CURSOR_LIBRARY
                                                WAYLANDPP_EGL
                                                WAYLANDPP_SCANNER
                                  VERSION_VAR WAYLANDPP_wayland-client++_VERSION)

if(WAYLANDPP_FOUND)
  set(WAYLANDPP_INCLUDE_DIRS ${WAYLANDPP_INCLUDE_DIR})
  set(WAYLANDPP_LIBRARIES ${WAYLANDPP_CLIENT_LIBRARY}
                          ${WAYLANDPP_CURSOR_LIBRARY}
                          ${WAYLANDPP_EGL})
  set(WAYLANDPP_DEFINITIONS -DHAVE_WAYLAND=1)
endif()

mark_as_advanced(WAYLANDPP_INCLUDE_DIR
                 WAYLANDPP_CLIENT_LIBRARY
                 WAYLANDPP_CURSOR_LIBRARY
                 WAYLANDPP_EGL WAYLANDPP_SCANNER)
