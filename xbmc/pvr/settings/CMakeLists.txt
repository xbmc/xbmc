set(SOURCES PVRCustomTimerSettings.cpp
            PVRIntSettingDefinition.cpp
            PVRIntSettingValues.cpp
            PVRSettings.cpp
            PVRStringSettingDefinition.cpp
            PVRStringSettingValues.cpp
            PVRTimerSettingDefinition.cpp)

set(HEADERS IPVRSettingsContainer.h
            PVRCustomTimerSettings.h
            PVRIntSettingDefinition.h
            PVRIntSettingValues.h
            PVRSettings.h
            PVRStringSettingDefinition.h
            PVRStringSettingValues.h
            PVRTimerSettingDefinition.h)

core_add_library(pvr_settings)
