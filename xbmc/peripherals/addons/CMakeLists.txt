set(SOURCES AddonButtonMap.cpp
            AddonButtonMapping.cpp
            AddonInputHandling.cpp
            PeripheralAddon.cpp
            PeripheralAddonTranslator.cpp)

set(HEADERS AddonButtonMap.h
            AddonButtonMapping.h
            AddonInputHandling.h
            PeripheralAddon.h
            PeripheralAddonTranslator.h)

core_add_library(peripherals_addons)
