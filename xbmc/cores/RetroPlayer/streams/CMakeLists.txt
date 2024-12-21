set(SOURCES RetroPlayerAudio.cpp
            RetroPlayerRendering.cpp
            RetroPlayerStreamTypes.cpp
            RetroPlayerVideo.cpp
            RPStreamManager.cpp
)

set(HEADERS IRetroPlayerStream.h
            IStreamManager.h
            RetroPlayerAudio.h
            RetroPlayerRendering.h
            RetroPlayerStreamTypes.h
            RetroPlayerVideo.h
            RPStreamManager.h
)

core_add_library(retroplayer_streams)
