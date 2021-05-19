set(SOURCES Input_Gamecontroller.mm
            PeripheralBusGCController.mm
            PeripheralBusGCControllerManager.mm)

set(HEADERS Input_Gamecontroller.h
            InputKey.h
            PeripheralBusGCController.h
            PeripheralBusGCControllerManager.h)

core_add_library(platform_darwin_peripherals)
