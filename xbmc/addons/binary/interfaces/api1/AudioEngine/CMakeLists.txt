set(SOURCES AddonCallbacksAudioEngine.cpp)

set(HEADERS AddonCallbacksAudioEngine.h)

core_add_library(api1AddonCallbacks_AudioEngine)
add_dependencies(api1AddonCallbacks_AudioEngine libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(api1AddonCallbacks_AudioEngine ffmpeg)
endif()
