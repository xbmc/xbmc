set(SOURCES GameClientReversiblePlayback.cpp
            GameLoop.cpp)

set(HEADERS GameClientRealtimePlayback.h
            GameClientReversiblePlayback.h
            GameLoop.h
            IGameClientPlayback.h)

core_add_library(gameplayback)
