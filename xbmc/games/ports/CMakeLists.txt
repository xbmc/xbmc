set(HEADERS PortDefinitions.h
)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(games_ports)
endif()
