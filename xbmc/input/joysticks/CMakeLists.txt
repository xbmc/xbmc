set(SOURCES DefaultJoystick.cpp
            DriverPrimitive.cpp
            JoystickMonitor.cpp
            JoystickTranslator.cpp
            KeymapHandler.cpp
            RumbleGenerator.cpp)

set(HEADERS DefaultJoystick.h
            DriverPrimitive.h
            IButtonMap.h
            IButtonMapper.h
            IDriverHandler.h
            IDriverReceiver.h
            IInputHandler.h
            IInputReceiver.h
            IKeymapHandler.h
            JoystickMonitor.h
            JoystickTranslator.h
            JoystickTypes.h
            JoystickUtils.h
            KeymapHandler.h
            RumbleGenerator.h)

core_add_library(input_joystick)
