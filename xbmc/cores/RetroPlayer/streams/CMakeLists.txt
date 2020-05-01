set(SOURCES RetroPlayerAudio.cpp
            RetroPlayerStreamTypes.cpp
            RetroPlayerVideo.cpp
            RPStreamManager.cpp
)

set(HEADERS IRetroPlayerStream.h
            IStreamManager.h
            RetroPlayerAudio.h
            RetroPlayerStreamTypes.h
            RetroPlayerVideo.h
            RPStreamManager.h
)

if(CORE_PLATFORM_NAME_LC STREQUAL tvos OR CORE_PLATFORM_NAME_LC STREQUAL ios)
  set(UNITY_EXCLUDES RetroPlayerVideo.cpp)
endif()

core_add_library(retroplayer_streams)
