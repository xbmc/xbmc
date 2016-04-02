set(SOURCES AddonCallbacksPeripheral.cpp)

set(HEADERS AddonCallbacksPeripheral.h)

core_add_library(api1AddonCallbacks_Peripheral)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(api1AddonCallbacks_Peripheral ffmpeg)
endif()
