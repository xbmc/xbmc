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
add_dependencies(settings_dialogs libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(settings_dialogs ffmpeg)
endif()
