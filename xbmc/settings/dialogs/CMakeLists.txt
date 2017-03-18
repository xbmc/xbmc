set(SOURCES GUIDialogContentSettings.cpp
            GUIDialogSettingsBase.cpp
            GUIDialogSettingsManagerBase.cpp
            GUIDialogSettingsManualBase.cpp)

set(HEADERS GUIDialogContentSettings.h
            GUIDialogSettingsBase.h
            GUIDialogSettingsManagerBase.h
            GUIDialogSettingsManualBase.h)

core_add_library(settings_dialogs)
