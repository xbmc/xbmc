set(SOURCES GameClient.cpp
            GameClientInGameSaves.cpp
            GameClientProperties.cpp
            GameClientSubsystem.cpp
            GameClientTiming.cpp
            GameClientTranslator.cpp)

set(HEADERS GameClient.h
            GameClientCallbacks.h
            GameClientInGameSaves.h
            GameClientProperties.h
            GameClientSubsystem.h
            GameClientTiming.h
            GameClientTranslator.h)

core_add_library(gameaddons)
