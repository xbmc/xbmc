set(SOURCES SmartHomeGuiInfo.cpp
            SmartHomeProperty.cpp
)

set(HEADERS IPowerMeterHUD.h
            ISystemHealthHUD.h
            IVehicleHUD.h
            SmartHomeGuiInfo.h
            SmartHomeProperty.h
)

core_add_library(smarthome_guiinfo)
