set(SOURCES ExternalPlayer.cpp)

set(HEADERS ExternalPlayer.h)

core_add_library(externalplayer)
add_dependencies(externalplayer libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(externalplayer ffmpeg)
endif()
