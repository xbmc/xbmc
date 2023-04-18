# FindWaylandProtocolsWebOS
# -------------------------
# Find wayland-protocol-webOS
#
# This will define the following variables::
#
# WAYLANDPROTOCOLSWEBOS_FOUND        -  systm has wayland-webos-client
# WAYLANDPROTOCOLSWEBOS_PROTOCOLSDIR -  directory containing the additional webOS Wayland protocols
#                                       from the webos-wayland-extensions package
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_WAYLAND_WEBOS_CLIENT wayland-webos-client>=1.0.0)
endif()

find_path(WAYLAND_PROTOCOLS_WEBOS_PROTOCOLDIR NAMES wayland-webos/webos-shell.xml
                                              PATHS ${DEPENDS_PATH}/share)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WaylandProtocolsWebOS
                                  REQUIRED_VARS WAYLAND_PROTOCOLS_WEBOS_PROTOCOLDIR
                                  VERSION_VAR WAYLAND_WEBOS_SERVER_VERSION)

if(WAYLANDPROTOCOLSWEBOS_FOUND)
  # Promote to cache variables so all code can access it
  set(WAYLANDPROTOCOLSWEBOS_PROTOCOLSDIR "${WAYLAND_PROTOCOLS_WEBOS_PROTOCOLDIR}/wayland-webos" CACHE INTERNAL "")
endif()

mark_as_advanced(WAYLANDPROTOCOLSWEBOS_PROTOCOLSDIR)
