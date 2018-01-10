set(SOURCES GameClientHardware.cpp
            GameClientInput.cpp
            GameClientJoystick.cpp
            GameClientKeyboard.cpp
            GameClientMouse.cpp
)

set(HEADERS GameClientHardware.h
            GameClientInput.h
            GameClientJoystick.h
            GameClientKeyboard.h
            GameClientMouse.h
)

core_add_library(gameinput)
