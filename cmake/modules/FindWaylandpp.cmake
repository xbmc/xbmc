# FindWaylandpp
# -----------
# Finds the waylandpp library
#
# This will will define the following variables::
#
# WAYLANDPP_FOUND        - the system has Wayland
# WAYLANDPP_INCLUDE_DIRS - the Wayland include directory
# WAYLANDPP_LIBRARIES    - the Wayland libraries
# WAYLANDPP_DEFINITIONS  - the Wayland definitions


if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_WAYLANDPP wayland-client++>=0.1 wayland-egl++ wayland-cursor++ wayland-scanner++ QUIET)
  pkg_check_modules(PC_WAYLAND_PROTOCOLS wayland-protocols>=1.7 QUIET)
  # TODO: Remove check when CMake minimum version is bumped globally
  if(CMAKE_VERSION VERSION_EQUAL 3.4.0 OR CMAKE_VERSION VERSION_GREATER 3.4.0)
    if(PC_WAYLANDPP_FOUND)
      pkg_get_variable(PC_WAYLANDPP_SCANNER wayland-scanner++ wayland_scannerpp)
    endif()
    if(PC_WAYLAND_PROTOCOLS_FOUND)
      pkg_get_variable(PC_WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
    endif()
  endif()
endif()

find_path(WAYLANDPP_INCLUDE_DIR NAMES wayland-client.hpp
                                PATHS ${PC_WAYLANDPP_INCLUDE_DIRS})

find_library(WAYLANDPP_CLIENT_LIBRARY NAMES wayland-client++
                                      PATHS ${PC_WAYLANDPP_LIBRARIES} ${PC_WAYLANDPP_LIBRARY_DIRS})

find_library(WAYLANDPP_CURSOR_LIBRARY NAMES wayland-cursor++
                                      PATHS ${PC_WAYLANDPP_LIBRARIES} ${PC_WAYLANDPP_LIBRARY_DIRS})

find_library(WAYLANDPP_EGL_LIBRARY NAMES wayland-egl++
                                   PATHS ${PC_WAYLANDPP_LIBRARIES} ${PC_WAYLANDPP_LIBRARY_DIRS})

find_program(WAYLANDPP_SCANNER NAMES wayland-scanner++
                               PATHS ${PC_WAYLANDPP_SCANNER})

find_path(WAYLAND_PROTOCOLS_DIR NAMES unstable/xdg-shell/xdg-shell-unstable-v6.xml
                                PATHS ${PC_WAYLAND_PROTOCOLS_DIR}
                                DOC "Directory containing additional Wayland protocols")

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (Waylandpp
  REQUIRED_VARS
    WAYLANDPP_INCLUDE_DIR
    WAYLANDPP_CLIENT_LIBRARY
    WAYLANDPP_CURSOR_LIBRARY
    WAYLANDPP_EGL_LIBRARY
    WAYLANDPP_SCANNER
    WAYLAND_PROTOCOLS_DIR
  VERSION_VAR
    PC_WAYLANDPP_wayland-client++_VERSION)

if (WAYLANDPP_FOUND)
  set(WAYLANDPP_LIBRARIES ${WAYLANDPP_CLIENT_LIBRARY} ${WAYLANDPP_CURSOR_LIBRARY} ${WAYLANDPP_EGL_LIBRARY})
  set(WAYLANDPP_INCLUDE_DIRS ${PC_WAYLANDPP_INCLUDE_DIRS})
  set(WAYLANDPP_DEFINITIONS -DHAVE_WAYLAND=1)
endif()

mark_as_advanced (WAYLANDPP_CLIENT_LIBRARY WAYLANDPP_CURSOR_LIBRARY WAYLANDPP_EGL_LIBRARY WAYLANDPP_INCLUDE_DIR)
