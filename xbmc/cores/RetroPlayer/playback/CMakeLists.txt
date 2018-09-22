set(SOURCES GameLoop.cpp
            ReversiblePlayback.cpp)

set(HEADERS GameLoop.h
            IPlayback.h
            IPlaybackControl.h
            RealtimePlayback.h
            ReversiblePlayback.h)

core_add_library(retroplayer_playback)
