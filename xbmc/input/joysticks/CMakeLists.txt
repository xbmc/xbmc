set(SOURCES DeadzoneFilter.cpp
            DriverPrimitive.cpp
            JoystickEasterEgg.cpp
            JoystickMonitor.cpp
            JoystickTranslator.cpp
            JoystickUtils.cpp
            RumbleGenerator.cpp)

set(HEADERS interfaces/IButtonMap.h
            interfaces/IButtonMapCallback.h
            interfaces/IButtonMapper.h
            interfaces/IButtonSequence.h
            interfaces/IDriverHandler.h
            interfaces/IDriverReceiver.h
            interfaces/IInputHandler.h
            interfaces/IInputProvider.h
            interfaces/IInputReceiver.h
            interfaces/IKeyHandler.h
            interfaces/IKeymapHandler.h
            DeadzoneFilter.h
            DriverPrimitive.h
            JoystickEasterEgg.h
            JoystickIDs.h
            JoystickMonitor.h
            JoystickTranslator.h
            JoystickTypes.h
            JoystickUtils.h
            RumbleGenerator.h)

core_add_library(input_joystick)
