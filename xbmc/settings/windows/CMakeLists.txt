set(SOURCES GUIControlSettings.cpp
            GUIWindowSettings.cpp
            GUIWindowSettingsCategory.cpp
            GUIWindowSettingsScreenCalibration.cpp
            GUIWindowTestPattern.cpp)

set(HEADERS GUIControlSettings.h
            GUIWindowSettings.h
            GUIWindowSettingsCategory.h
            GUIWindowSettingsScreenCalibration.h
            GUIWindowTestPattern.h)

core_add_library(settings_windows)
