set(SOURCES DeadzoneFilter.cpp
            DriverPrimitive.cpp
            JoystickEasterEgg.cpp
            JoystickMonitor.cpp
            JoystickTranslator.cpp
            JoystickUtils.cpp
            RumbleGenerator.cpp)

set(HEADERS DeadzoneFilter.h
            DriverPrimitive.h
            JoystickEasterEgg.h
            JoystickIDs.h
            JoystickMonitor.h
            JoystickTranslator.h
            JoystickTypes.h
            JoystickUtils.h
            RumbleGenerator.h)

core_add_library(input_joystick)
