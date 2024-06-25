# FindWaylandPPScanner
# --------
# Find the WaylandPPScanner Tool
#
# This will define the following target:
#
#   wayland::waylandppscanner - The FXC compiler

if(NOT wayland::waylandppscanner)

  find_package(PkgConfig)
  pkg_check_modules(PC_WAYLANDPP_SCANNER wayland-scanner++ QUIET)

  if(PC_WAYLANDPP_SCANNER_FOUND)
    pkg_get_variable(PC_WAYLANDPP_SCANNER wayland-scanner++ wayland_scannerpp)
  endif()

  find_program(WAYLANDPP_SCANNER wayland-scanner++ HINTS ${PC_WAYLANDPP_SCANNER})

  if(WAYLANDPP_SCANNER)

    include(FindPackageMessage)
    find_package_message(WaylandPPScanner "Found WaylandPP Scanner: ${WAYLANDPP_SCANNER}" "[${WAYLANDPP_SCANNER}]")

    add_executable(wayland::waylandppscanner IMPORTED)
    set_target_properties(wayland::waylandppscanner PROPERTIES
                                                    IMPORTED_LOCATION "${WAYLANDPP_SCANNER}"
                                                    FOLDER "External Projects")
  else()
    if(WaylandPPScanner_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find WaylandPP Scanner")
    endif()
  endif()
endif()
