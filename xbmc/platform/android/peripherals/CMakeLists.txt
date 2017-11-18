set(SOURCES AndroidJoystickState.cpp
            AndroidJoystickTranslator.cpp
            PeripheralBusAndroid.cpp)

set(HEADERS AndroidJoystickState.h
            AndroidJoystickTranslator.h
            PeripheralBusAndroid.h)

core_add_library(platform_android_peripherals)
