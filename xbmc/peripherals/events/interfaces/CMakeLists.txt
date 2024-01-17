set(HEADERS IEventScannerCallback.h
)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(peripherals_events_interfaces)
endif()
