set(SOURCES DiscStateHistory.cpp
            GameLoop.cpp
            ReversiblePlayback.cpp)

set(HEADERS DiscStateHistory.h
            GameLoop.h
            IPlayback.h
            IPlaybackControl.h
            RealtimePlayback.h
            ReversiblePlayback.h)

core_add_library(retroplayer_playback)
