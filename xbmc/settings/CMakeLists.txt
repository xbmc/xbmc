set(SOURCES AdvancedSettings.cpp
            AudioDSPSettings.cpp
            DisplaySettings.cpp
            MediaSettings.cpp
            MediaSourceSettings.cpp
            SettingAddon.cpp
            SettingConditions.cpp
            SettingControl.cpp
            SettingCreator.cpp
            SettingDateTime.cpp
            SettingPath.cpp
            Settings.cpp
            SettingsBase.cpp
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
            SettingDateTime.h
            SettingPath.h
            Settings.h
            SettingsBase.h
            SettingUtils.h
            SkinSettings.h
            VideoSettings.h)

if(BLURAY_FOUND)
  list(APPEND SOURCES DiscSettings.cpp)
endif()

core_add_library(settings)
