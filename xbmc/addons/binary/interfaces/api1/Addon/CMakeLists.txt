set(SOURCES AddonCallbacksAddon.cpp)

set(HEADERS AddonCallbacksAddon.h)

core_add_library(api1AddonCallbacks_Addon)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(api1AddonCallbacks_Addon ffmpeg)
endif()
