if(TARGET ${APP_NAME_LC}::WaylandServer)
  set(PROTOCOLS ${WAYLAND_SERVER_PROTOCOLS}
                kodi-web-v1:${CMAKE_CURRENT_SOURCE_DIR}/protocol/kodi-web-v1.xml)

  set(SOURCES WaylandBuffer.cpp
              WaylandHost.cpp
              WaylandSurface.cpp)

  set(HEADERS WaylandBuffer.h
              WaylandHost.h
              WaylandSurface.h)

  foreach(protocol ${PROTOCOLS})
    string(REPLACE ":" ";" protocol ${protocol})
    list(GET protocol 0 name)
    list(GET protocol 1 xml)
    set(header ${CMAKE_CURRENT_BINARY_DIR}/${name}-server-protocol.h)
    set(code ${CMAKE_CURRENT_BINARY_DIR}/${name}-protocol.c)
    add_custom_command(OUTPUT ${header}
                       COMMAND ${WAYLAND_SCANNER_EXECUTABLE} server-header ${xml} ${header}
                       DEPENDS ${xml}
                       COMMENT "Generating ${name} server header")
    add_custom_command(OUTPUT ${code}
                       COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code ${xml} ${code}
                       DEPENDS ${xml} ${header}
                       COMMENT "Generating ${name} protocol code")
    list(APPEND SOURCES ${code})
    list(APPEND HEADERS ${header})
  endforeach()

  include_directories(${CMAKE_CURRENT_BINARY_DIR})

  core_add_library(platform_linux_waylandhost)
endif()
