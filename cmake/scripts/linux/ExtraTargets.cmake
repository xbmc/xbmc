# xrandr
if(X_FOUND AND XRANDR_FOUND)
  find_package(X QUIET)
  find_package(XRandR QUIET)
  add_executable(${APP_NAME_LC}-xrandr ${CMAKE_SOURCE_DIR}/xbmc-xrandr.c)
  target_link_libraries(${APP_NAME_LC}-xrandr ${SYSTEM_LDFLAGS} ${X_LIBRARIES} m ${XRANDR_LIBRARIES})
endif()

# WiiRemote
if(ENABLE_EVENTCLIENTS AND BLUETOOTH_FOUND)
  find_package(CWiid QUIET)
  if(CWIID_FOUND)
    add_subdirectory(${CMAKE_SOURCE_DIR}/tools/EventClients/Clients/WiiRemote build/WiiRemote)
  endif()
endif()

if(CORE_PLATFORM_NAME_LC STREQUAL "wayland")
  # This cannot go into wayland.cmake since it requires the Wayland dependencies
  # to already be resolved
  set(PROTOCOL_XMLS "${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-shell/xdg-shell-unstable-v6.xml"
                    "${WAYLAND_PROTOCOLS_DIR}/unstable/idle-inhibit/idle-inhibit-unstable-v1.xml")
  add_custom_command(OUTPUT "${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-extra-protocols.hpp" "${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-extra-protocols.cpp"
                     COMMAND "${WAYLANDPP_SCANNER}" ${PROTOCOL_XMLS} "${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-extra-protocols.hpp" "${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-extra-protocols.cpp"
                     DEPENDS "${WAYLANDPP_SCANNER}" ${PROTOCOL_XMLS}
                     COMMENT "Generating wayland-protocols C++ wrappers")

  # Dummy target for dependencies
  add_custom_target(generate-wayland-extra-protocols DEPENDS wayland-extra-protocols.hpp)
endif()