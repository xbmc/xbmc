set(SOURCES GameClient.cpp
            GameClientInGameSaves.cpp
            GameClientInput.cpp
            GameClientKeyboard.cpp
            GameClientMouse.cpp
            GameClientProperties.cpp
            GameClientTiming.cpp
            GameClientTranslator.cpp)

set(HEADERS GameClient.h
            GameClientCallbacks.h
            GameClientInGameSaves.h
            GameClientInput.h
            GameClientKeyboard.h
            GameClientMouse.h
            GameClientProperties.h
            GameClientTiming.h
            GameClientTranslator.h)

core_add_library(gameaddons)
