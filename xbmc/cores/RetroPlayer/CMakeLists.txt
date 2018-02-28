set(SOURCES RetroPlayer.cpp
            RetroPlayerAutoSave.cpp
            RetroPlayerInput.cpp
            RetroPlayerUtils.cpp
)

set(HEADERS RetroPlayer.h
            RetroPlayerAutoSave.h
            RetroPlayerInput.h
            RetroPlayerTypes.h
            RetroPlayerUtils.h
)

core_add_library(retroplayer)
