set(SOURCES AddonCallbacksInputStream.cpp)

set(HEADERS AddonCallbacksInputStream.h)

core_add_library(api1AddonCallbacks_InputStream)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(api1AddonCallbacks_InputStream ffmpeg)
endif()
