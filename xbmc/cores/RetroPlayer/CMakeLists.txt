set(SOURCES RetroPlayer.cpp
            RetroPlayerAudio.cpp
            RetroPlayerAutoSave.cpp
            RetroPlayerInput.cpp
            RetroPlayerUtils.cpp
            RetroPlayerVideo.cpp
)

set(HEADERS RetroPlayer.h
            RetroPlayerAudio.h
            RetroPlayerAutoSave.h
            RetroPlayerInput.h
            RetroPlayerTypes.h
            RetroPlayerUtils.h
            RetroPlayerVideo.h
)

core_add_library(retroplayer)
