set(SOURCES GameClient.cpp
            GameClientExtractor.cpp
            GameClientFastForward.cpp
            GameClientInGameSaves.cpp
            GameClientProperties.cpp
            GameClientSubsystem.cpp
            GameClientTranslator.cpp)

set(HEADERS GameClient.h
            GameClientCallbacks.h
            GameClientExtractor.h
            GameClientFastForward.h
            GameClientInGameSaves.h
            GameClientProperties.h
            GameClientRestoreResult.h
            GameClientSubsystem.h
            GameClientTranslator.h)

core_add_library(gameaddons)
