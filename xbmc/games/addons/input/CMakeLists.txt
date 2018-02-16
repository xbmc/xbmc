set(SOURCES GameClientDevice.cpp
            GameClientHardware.cpp
            GameClientInput.cpp
            GameClientJoystick.cpp
            GameClientKeyboard.cpp
            GameClientMouse.cpp
            GameClientPort.cpp
            GameClientTopology.cpp
)

set(HEADERS GameClientDevice.h
            GameClientHardware.h
            GameClientInput.h
            GameClientJoystick.h
            GameClientKeyboard.h
            GameClientMouse.h
            GameClientPort.h
            GameClientTopology.h
)

core_add_library(gameinput)
