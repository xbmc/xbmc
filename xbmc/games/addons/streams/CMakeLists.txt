set(SOURCES GameClientStreamAudio.cpp
            GameClientStreams.cpp
            GameClientStreamVideo.cpp
)

set(HEADERS GameClientStreamAudio.h
            GameClientStreams.h
            GameClientStreamVideo.h
            IGameClientStream.h
)

core_add_library(game_addon_streams)
