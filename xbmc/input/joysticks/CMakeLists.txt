set(SOURCES DeadzoneFilter.cpp
            DriverPrimitive.cpp
            JoystickEasterEgg.cpp
            JoystickMonitor.cpp
            JoystickTranslator.cpp
            JoystickUtils.cpp
            RumbleGenerator.cpp)

set(HEADERS interfaces/IKeyHandler.h
            interfaces/IKeymapHandler.h
            DeadzoneFilter.h
            DriverPrimitive.h
            IButtonMap.h
            IButtonMapCallback.h
            IButtonMapper.h
            IButtonSequence.h
            IDriverHandler.h
            IDriverReceiver.h
            IInputHandler.h
            IInputProvider.h
            IInputReceiver.h
            JoystickEasterEgg.h
            JoystickIDs.h
            JoystickMonitor.h
            JoystickTranslator.h
            JoystickTypes.h
            JoystickUtils.h
            RumbleGenerator.h)

core_add_library(input_joystick)
