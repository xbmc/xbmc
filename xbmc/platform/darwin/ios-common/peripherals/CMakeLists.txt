set(SOURCES Input_Gamecontroller.mm
            PeripheralBusDarwinEmbedded.mm
            PeripheralBusDarwinEmbeddedManager.mm)

set(HEADERS Input_Gamecontroller.h
            InputKey.h
            PeripheralBusDarwinEmbedded.h
            PeripheralBusDarwinEmbeddedManager.h)

core_add_library(platform_darwinembedded_peripherals)
