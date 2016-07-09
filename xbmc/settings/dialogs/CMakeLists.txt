set(SOURCES GUIDialogAudioDSPManager.cpp
            GUIDialogAudioDSPSettings.cpp
            GUIDialogContentSettings.cpp
            GUIDialogSettingsBase.cpp
            GUIDialogSettingsManagerBase.cpp
            GUIDialogSettingsManualBase.cpp)

set(HEADERS GUIDialogAudioDSPManager.h
            GUIDialogAudioDSPSettings.h
            GUIDialogContentSettings.h
            GUIDialogSettingsBase.h
            GUIDialogSettingsManagerBase.h
            GUIDialogSettingsManualBase.h)

core_add_library(settings_dialogs)
