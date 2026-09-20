# xrandr
if(TARGET ${APP_NAME_LC}::X AND TARGET ${APP_NAME_LC}::XRandR)
  find_package(X ${SEARCH_QUIET})
  find_package(XRandR ${SEARCH_QUIET})
  add_executable(${APP_NAME_LC}-xrandr ${CMAKE_SOURCE_DIR}/xbmc-xrandr.c)
  target_link_libraries(${APP_NAME_LC}-xrandr ${SYSTEM_LDFLAGS} ${APP_NAME_LC}::X m ${APP_NAME_LC}::XRandR)
endif()

# WiiRemote
if(ENABLE_EVENTCLIENTS AND TARGET ${APP_NAME_LC}::Bluetooth)
  find_package(CWiid ${SEARCH_QUIET})
  find_package(GLU ${SEARCH_QUIET})
  if(TARGET ${APP_NAME_LC}::CWiid AND TARGET ${APP_NAME_LC}::GLU)
    add_subdirectory(${CMAKE_SOURCE_DIR}/tools/EventClients/Clients/WiiRemote build/WiiRemote)
  endif()
endif()

if("wayland" IN_LIST CORE_PLATFORM_NAME_LC)
  # Both lists are assembled and checked by the find modules that locate their directories
  set(PROTOCOL_XMLS ${WAYLANDPP_PROTOCOLS_XMLS} ${WAYLAND_PROTOCOLS_XMLS})

  add_custom_command(OUTPUT "${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-extra-protocols.hpp" "${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-extra-protocols.cpp"
                     COMMAND wayland::waylandppscanner
                     ARGS ${PROTOCOL_XMLS} "${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-extra-protocols.hpp" "${WAYLAND_EXTRA_PROTOCOL_GENERATED_DIR}/wayland-extra-protocols.cpp"
                     DEPENDS wayland::waylandppscanner ${PROTOCOL_XMLS}
                     COMMENT "Generating wayland-protocols C++ wrappers")

  if("webos" IN_LIST CORE_PLATFORM_NAME_LC)
    include(${CMAKE_SOURCE_DIR}/cmake/scripts/webos/ExtraTargets.cmake)
  endif()

  # Dummy target for dependencies
  add_custom_target(generate-wayland-extra-protocols DEPENDS wayland-extra-protocols.hpp)

  add_dependencies(lib${APP_NAME_LC} generate-wayland-extra-protocols)
endif()
