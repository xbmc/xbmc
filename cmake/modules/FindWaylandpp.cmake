# FindWaylandpp
# -------------
# Finds the waylandpp library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Waylandpp   - The waylandpp library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig)
  pkg_check_modules(PC_WAYLANDPP wayland-client++ wayland-egl++ wayland-cursor++ QUIET)

  if(PC_WAYLANDPP_FOUND)
    pkg_get_variable(PC_WAYLANDPP_PKGDATADIR wayland-client++ pkgdatadir)
  else()
    message(SEND_ERROR "wayland-client++ not found via pkg-config")
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

  include (FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Waylandpp
                                    REQUIRED_VARS WAYLANDPP_INCLUDE_DIR
                                                  WAYLANDPP_CLIENT_LIBRARY
                                                  WAYLANDPP_CURSOR_LIBRARY
                                                  WAYLANDPP_EGL_LIBRARY
                                    VERSION_VAR WAYLANDPP_wayland-client++_VERSION)

  if(WAYLANDPP_FOUND)

    find_package(WaylandPPScanner REQUIRED)

    set(WAYLANDPP_INCLUDE_DIRS ${WAYLANDPP_INCLUDE_DIR})
    set(WAYLANDPP_LIBRARIES 
                            ${WAYLANDPP_STATIC_DEPS})

    if(KODI_DEPENDSBUILD)
      add_library(${APP_NAME_LC}::waylandc-egl UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::waylandc-egl PROPERTIES
                                                          IMPORTED_LOCATION "${WAYLANDC_EGL_LIBRARY}")

      add_library(${APP_NAME_LC}::waylandc-cursor UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::waylandc-cursor PROPERTIES
                                                             IMPORTED_LOCATION "${WAYLANDC_CURSOR_LIBRARY}")

      add_library(${APP_NAME_LC}::waylandc-client UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::waylandc-client PROPERTIES
                                                             IMPORTED_LOCATION "${WAYLANDC_CLIENT_LIBRARY}")
    endif()

    add_library(${APP_NAME_LC}::waylandpp-egl UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::waylandpp-egl PROPERTIES
                                                        IMPORTED_LOCATION "${WAYLANDPP_EGL_LIBRARY}"
                                                        INTERFACE_INCLUDE_DIRECTORIES "${WAYLANDPP_INCLUDE_DIR}")

    add_library(${APP_NAME_LC}::waylandpp-cursor UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::waylandpp-cursor PROPERTIES
                                                           IMPORTED_LOCATION "${WAYLANDPP_CURSOR_LIBRARY}"
                                                           INTERFACE_INCLUDE_DIRECTORIES "${WAYLANDPP_INCLUDE_DIR}")

    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${WAYLANDPP_CLIENT_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${WAYLANDPP_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAVE_WAYLAND
                                                                     INTERFACE_LINK_LIBRARIES "${APP_NAME_LC}::waylandpp-cursor;${APP_NAME_LC}::waylandpp-egl;$<TARGET_NAME_IF_EXISTS:${APP_NAME_LC}::waylandc-egl>;$<TARGET_NAME_IF_EXISTS:${APP_NAME_LC}::waylandc-cursor>;$<TARGET_NAME_IF_EXISTS:${APP_NAME_LC}::waylandc-client>")

  endif()
endif()
