# FindWaylandProtocolsWebOS
# -------------------------
# Find wayland-protocol-webOS
#
# This will define the following variables::
#
# WAYLANDPROTOCOLSWEBOS_PROTOCOLSDIR -  directory containing the additional webOS Wayland protocols
#                                       from the webos-wayland-extensions package

find_path(WAYLANDPROTOCOLSWEBOS_PROTOCOLSDIR NAMES webos-shell.xml
                                             PATH_SUFFIXES wayland-webos
                                             HINTS ${DEPENDS_PATH}/share
                                             REQUIRED)

if(NOT VERBOSE_FIND)
   set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
 endif()

include(FindPackageMessage)
find_package_message(WaylandProtocolsWebOS "Found WaylandProtocols-WebOS: ${WAYLANDPROTOCOLSWEBOS_PROTOCOLSDIR}" "[${WAYLANDPROTOCOLSWEBOS_PROTOCOLSDIR}]")

mark_as_advanced(WAYLANDPROTOCOLSWEBOS_PROTOCOLSDIR)
