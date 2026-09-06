set(SOURCES GameLoop.cpp
            ReversiblePlayback.cpp)

set(HEADERS AutosaveCapture.h
            GameLoop.h
            IPlayback.h
            IPlaybackControl.h
            RealtimePlayback.h
            ReversiblePlayback.h
            SavestateCapture.h
            SavestateWorker.h)

core_add_library(retroplayer_playback)
