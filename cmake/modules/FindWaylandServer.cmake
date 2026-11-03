#.rst:
# FindWaylandServer
# -----------------
# Finds the wayland-server library, the wayland-scanner code generator and
# the wayland-protocols XML files needed to host Wayland clients
#
# This will define the following target:
#
#   ${APP_NAME_LC}::WaylandServer   - The wayland-server library
#
# and the following variables:
#
#   WAYLAND_SCANNER_EXECUTABLE   - wayland-scanner program
#   WAYLAND_SERVER_PROTOCOLS     - name:xml pairs of the protocols the host serves

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC wayland-server)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})

    pkg_check_modules(PC_WAYLAND_SCANNER wayland-scanner ${SEARCH_QUIET})
    if(PC_WAYLAND_SCANNER_FOUND)
      pkg_get_variable(PC_WAYLAND_SCANNER_EXECUTABLE wayland-scanner wayland_scanner)
    endif()
    find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner
                                            HINTS ${PC_WAYLAND_SCANNER_EXECUTABLE})

    # the host serves wl_surface 5 (wayland 1.20) and linux-dmabuf 4 (wayland-protocols 1.24)
    pkg_check_modules(PC_WAYLAND_SERVER_VERSION wayland-server>=1.20 ${SEARCH_QUIET})
    pkg_check_modules(PC_WAYLAND_PROTOCOLS wayland-protocols>=1.24 ${SEARCH_QUIET})
    if(PC_WAYLAND_PROTOCOLS_FOUND)
      pkg_get_variable(_protocols_dir wayland-protocols pkgdatadir)
    endif()

    set(_dmabuf_xml ${_protocols_dir}/stable/linux-dmabuf/linux-dmabuf-v1.xml)
    if(NOT EXISTS ${_dmabuf_xml})
      # older wayland-protocols carry linux-dmabuf in unstable/ only
      set(_dmabuf_xml ${_protocols_dir}/unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml)
    endif()
    set(_protocols linux-dmabuf-v1:${_dmabuf_xml}
                   presentation-time:${_protocols_dir}/stable/presentation-time/presentation-time.xml
                   text-input-unstable-v3:${_protocols_dir}/unstable/text-input/text-input-unstable-v3.xml
                   xdg-shell:${_protocols_dir}/stable/xdg-shell/xdg-shell.xml)

    set(_protocols_found ${PC_WAYLAND_PROTOCOLS_FOUND})
    foreach(_protocol IN LISTS _protocols)
      string(REGEX REPLACE "^[^:]*:" "" _xml ${_protocol})
      if(NOT EXISTS ${_xml})
        set(_protocols_found FALSE)
      endif()
    endforeach()
    if(_protocols_found)
      set(WAYLAND_SERVER_PROTOCOLS ${_protocols} CACHE INTERNAL "")
    endif()

    if(WAYLAND_SCANNER_EXECUTABLE AND PC_WAYLAND_SERVER_VERSION_FOUND AND _protocols_found)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})

      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAVE_WAYLAND_SERVER)

      ADD_TARGET_COMPILE_DEFINITION()
    elseif(WaylandServer_FIND_REQUIRED)
      message(FATAL_ERROR "wayland-scanner, wayland-server 1.20 or the wayland-protocols 1.24 descriptions were not found.")
    endif()
  else()
    if(WaylandServer_FIND_REQUIRED)
      message(FATAL_ERROR "wayland-server library was not found.")
    endif()
  endif()
endif()
