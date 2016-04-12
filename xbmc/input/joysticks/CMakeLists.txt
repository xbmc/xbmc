set(SOURCES DefaultJoystick.cpp
            DriverPrimitive.cpp
            JoystickMonitor.cpp
            JoystickTranslator.cpp
            KeymapHandler.cpp)

set(HEADERS DefaultJoystick.h
            DriverPrimitive.h
            IButtonMap.h
            IButtonMapper.h
            IDriverHandler.h
            IInputHandler.h
            IKeymapHandler.h
            JoystickMonitor.h
            JoystickTranslator.h
            JoystickTypes.h
            JoystickUtils.h
            KeymapHandler.h)

core_add_library(input_joystick)
add_dependencies(input_joystick libcpluff)
