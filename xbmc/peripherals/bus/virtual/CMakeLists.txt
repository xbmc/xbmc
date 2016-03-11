set(SOURCES PeripheralBusAddon.cpp)

set(HEADERS PeripheralBusAddon.h)

if(CEC_FOUND)
  list(APPEND SOURCES PeripheralBusCEC.cpp)
  list(APPEND HEADERS PeripheralBusCEC.h)
endif()

core_add_library(peripheral_bus_virtual)
