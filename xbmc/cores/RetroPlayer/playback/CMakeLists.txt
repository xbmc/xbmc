set(SOURCES ReversiblePlayback.cpp
            GameLoop.cpp)

set(HEADERS RealtimePlayback.h
            ReversiblePlayback.h
            GameLoop.h
            IPlayback.h)

core_add_library(retroplayer_playback)
