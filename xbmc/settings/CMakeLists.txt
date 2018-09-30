set(SOURCES AdvancedSettings.cpp
            DisplaySettings.cpp
            GameSettings.cpp
            LibExportSettings.cpp
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
            SettingsComponent.cpp)

set(HEADERS AdvancedSettings.h
            DiscSettings.h
            DisplaySettings.h
            GameSettings.h
            LibExportSettings.h
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
            SettingsComponent.h)

if(BLURAY_FOUND)
  list(APPEND SOURCES DiscSettings.cpp)
endif()

core_add_library(settings)
