set(SOURCES GameClient.cpp
            GameClientInGameSaves.cpp
            GameClientProperties.cpp
            GameClientSubsystem.cpp
            GameClientTranslator.cpp)

set(HEADERS GameClient.h
            GameClientCallbacks.h
            GameClientInGameSaves.h
            GameClientProperties.h
            GameClientSubsystem.h
            GameClientTranslator.h)

core_add_library(gameaddons)
