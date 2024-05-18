set(SOURCES PeripheralBusAddon.cpp
            PeripheralBusApplication.cpp)

set(HEADERS PeripheralBusAddon.h
            PeripheralBusApplication.h)

if(TARGET ${APP_NAME_LC}::CEC)
  list(APPEND SOURCES PeripheralBusCEC.cpp)
  list(APPEND HEADERS PeripheralBusCEC.h)
endif()

core_add_library(peripheral_bus_virtual)
