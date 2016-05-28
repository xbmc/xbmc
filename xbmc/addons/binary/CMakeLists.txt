set(SOURCES ExceptionHandling.cpp)

set(HEADERS ExceptionHandling.h)

core_add_library(addonsBinary)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(addonsBinary ffmpeg)
endif()
