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

find_package(PkgConfig)
pkg_check_modules(PC_WAYLANDPP wayland-client++ wayland-egl++ wayland-cursor++ QUIET)

if(PC_WAYLANDPP_FOUND)
  pkg_get_variable(PC_WAYLANDPP_PKGDATADIR wayland-client++ pkgdatadir)
else()
  message(SEND_ERROR "wayland-client++ not found via pkg-config")
endif()

pkg_check_modules(PC_WAYLANDPP_SCANNER wayland-scanner++ QUIET)

if(PC_WAYLANDPP_SCANNER_FOUND)
  pkg_get_variable(PC_WAYLANDPP_SCANNER wayland-scanner++ wayland_scannerpp)
else()
  message(SEND_ERROR "wayland-scanner++ not found via pkg-config")
endif()

find_path(WAYLANDPP_INCLUDE_DIR wayland-client.hpp HINTS ${PC_WAYLANDPP_INCLUDEDIR})

find_library(WAYLANDPP_CLIENT_LIBRARY NAMES wayland-client++
                                      HINTS ${PC_WAYLANDPP_LIBRARY_DIRS})

find_library(WAYLANDPP_CURSOR_LIBRARY NAMES wayland-cursor++
                                      HINTS ${PC_WAYLANDPP_LIBRARY_DIRS})

find_library(WAYLANDPP_EGL_LIBRARY NAMES wayland-egl++
                                   HINTS ${PC_WAYLANDPP_LIBRARY_DIRS})

if(KODI_DEPENDSBUILD)
  pkg_check_modules(PC_WAYLANDC wayland-client wayland-egl wayland-cursor QUIET)

  if(PREFER_TOOLCHAIN_PATH)
    set(WAYLAND_SEARCH_PATH ${PREFER_TOOLCHAIN_PATH}
                            NO_DEFAULT_PATH
                            PATH_SUFFIXES usr/lib)
  else()
    set(WAYLAND_SEARCH_PATH ${PC_WAYLANDC_LIBRARY_DIRS})
  endif()

  find_library(WAYLANDC_CLIENT_LIBRARY NAMES wayland-client
                                       HINTS ${WAYLAND_SEARCH_PATH}
                                       REQUIRED)
  find_library(WAYLANDC_CURSOR_LIBRARY NAMES wayland-cursor
                                       HINTS ${WAYLAND_SEARCH_PATH}
                                       REQUIRED)
  find_library(WAYLANDC_EGL_LIBRARY NAMES wayland-egl
                                    HINTS ${WAYLAND_SEARCH_PATH}
                                    REQUIRED)

  set(WAYLANDPP_STATIC_DEPS ${WAYLANDC_CLIENT_LIBRARY}
                            ${WAYLANDC_CURSOR_LIBRARY}
                            ${WAYLANDC_EGL_LIBRARY})
endif()

# Promote to cache variables so all code can access it
set(WAYLANDPP_PROTOCOLS_DIR "${PC_WAYLANDPP_PKGDATADIR}/protocols" CACHE INTERNAL "")

# wayland-scanner++ is from native/host system in case of cross-compilation, so
# it's ok if we don't find it with pkgconfig
find_program(WAYLANDPP_SCANNER wayland-scanner++ HINTS ${PC_WAYLANDPP_SCANNER})

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args(Waylandpp
                                  REQUIRED_VARS WAYLANDPP_INCLUDE_DIR
                                                WAYLANDPP_CLIENT_LIBRARY
                                                WAYLANDPP_CURSOR_LIBRARY
                                                WAYLANDPP_EGL_LIBRARY
                                                WAYLANDPP_SCANNER
                                  VERSION_VAR WAYLANDPP_wayland-client++_VERSION)

if(WAYLANDPP_FOUND)
  set(WAYLANDPP_INCLUDE_DIRS ${WAYLANDPP_INCLUDE_DIR})
  set(WAYLANDPP_LIBRARIES ${WAYLANDPP_CLIENT_LIBRARY}
                          ${WAYLANDPP_CURSOR_LIBRARY}
                          ${WAYLANDPP_EGL_LIBRARY}
                          ${WAYLANDPP_STATIC_DEPS})
  set(WAYLANDPP_DEFINITIONS -DHAVE_WAYLAND=1)
endif()

mark_as_advanced(WAYLANDPP_INCLUDE_DIR
                 WAYLANDPP_CLIENT_LIBRARY
                 WAYLANDC_CLIENT_LIBRARY
                 WAYLANDPP_CURSOR_LIBRARY
                 WAYLANDC_CURSOR_LIBRARY
                 WAYLANDPP_EGL_LIBRARY
                 WAYLANDC_EGL_LIBRARY
                 WAYLANDPP_SCANNER)
