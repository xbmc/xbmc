set(SOURCES SmartHomeInputManager.cpp
            SmartHomeJoystick.cpp
)

set(HEADERS ISmartHomeJoystickHandler.h
            SmartHomeInputManager.h
            SmartHomeJoystick.h
)

core_add_library(smarthome_input)
