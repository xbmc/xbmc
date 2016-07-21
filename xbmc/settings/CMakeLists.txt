set(SOURCES AdvancedSettings.cpp
            AudioDSPSettings.cpp
            DisplaySettings.cpp
            MediaSettings.cpp
            MediaSourceSettings.cpp
            SettingAddon.cpp
            SettingConditions.cpp
            SettingControl.cpp
            SettingCreator.cpp
            SettingPath.cpp
            Settings.cpp
            SettingUtils.cpp
            SkinSettings.cpp
            VideoSettings.cpp)

set(HEADERS AdvancedSettings.h
            AudioDSPSettings.h
            DiscSettings.h
            DisplaySettings.h
            MediaSettings.h
            MediaSourceSettings.h
            SettingAddon.h
            SettingConditions.h
            SettingControl.h
            SettingCreator.h
            SettingPath.h
            Settings.h
            SettingUtils.h
            SkinSettings.h
            VideoSettings.h)

core_add_library(settings)
