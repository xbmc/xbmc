set(SOURCES AddonCallbacksCodec.cpp)

set(HEADERS AddonCallbacksCodec.h)

core_add_library(api1AddonCallbacks_Codec)
add_dependencies(api1AddonCallbacks_Codec libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(api1AddonCallbacks_Codec ffmpeg)
endif()
