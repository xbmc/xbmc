set(SOURCES GUIDialogAudioDSPManager.cpp
            GUIDialogContentSettings.cpp
            GUIDialogLibExportSettings.cpp
            GUIDialogSettingsBase.cpp
            GUIDialogSettingsManagerBase.cpp
            GUIDialogSettingsManualBase.cpp)

set(HEADERS GUIDialogAudioDSPManager.h
            GUIDialogContentSettings.h
            GUIDialogLibExportSettings.h
            GUIDialogSettingsBase.h
            GUIDialogSettingsManagerBase.h
            GUIDialogSettingsManualBase.h)

core_add_library(settings_dialogs)
