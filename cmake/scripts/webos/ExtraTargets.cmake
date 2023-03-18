# generate webos wayland protocols
set(WEBOS_PROTOCOL_XMLS "${WAYLANDPROTOCOLSWEBOS_PROTOCOLSDIR}/webos-shell.xml")
add_custom_command(OUTPUT "${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-webos-protocols.hpp" "${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-webos-protocols.cpp"
                  COMMAND "${WAYLANDPP_SCANNER}" ${WEBOS_PROTOCOL_XMLS} "${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-webos-protocols.hpp" "${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-webos-protocols.cpp"
                  DEPENDS "${WAYLANDPP_SCANNER}" ${WEBOS_PROTOCOL_XMLS}
                  COMMENT "Generating wayland-webos C++ wrappers")
add_custom_target(generate-wayland-webos-protocols DEPENDS wayland-webos-protocols.hpp)
