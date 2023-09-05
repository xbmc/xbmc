set(SOURCES Peripheral.cpp
            PeripheralBluetooth.cpp
            PeripheralDisk.cpp
            PeripheralHID.cpp
            PeripheralImon.cpp
            PeripheralJoystick.cpp
            PeripheralKeyboard.cpp
            PeripheralMouse.cpp
            PeripheralNIC.cpp
            PeripheralNyxboard.cpp
            PeripheralTuner.cpp)

set(HEADERS Peripheral.h
            PeripheralBluetooth.h
            PeripheralDisk.h
            PeripheralHID.h
            PeripheralImon.h
            PeripheralJoystick.h
            PeripheralKeyboard.h
            PeripheralMouse.h
            PeripheralNIC.h
            PeripheralNyxboard.h
            PeripheralTuner.h)

if(TARGET CEC::CEC)
  list(APPEND SOURCES PeripheralCecAdapter.cpp)
  list(APPEND HEADERS PeripheralCecAdapter.h)
endif()

core_add_library(peripherals_devices)
