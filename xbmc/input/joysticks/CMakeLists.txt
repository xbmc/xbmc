set(SOURCES DeadzoneFilter.cpp
            DriverPrimitive.cpp
            JoystickEasterEgg.cpp
            JoystickMonitor.cpp
            JoystickTranslator.cpp
            RumbleGenerator.cpp)

set(HEADERS DeadzoneFilter.h
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
