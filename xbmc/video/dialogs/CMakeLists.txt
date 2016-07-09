set(SOURCES GUIDialogAudioSubtitleSettings.cpp
            GUIDialogCMSSettings.cpp
            GUIDialogFullScreenInfo.cpp
            GUIDialogSubtitles.cpp
            GUIDialogTeletext.cpp
            GUIDialogVideoBookmarks.cpp
            GUIDialogVideoInfo.cpp
            GUIDialogVideoOSD.cpp
            GUIDialogVideoSettings.cpp)

set(HEADERS GUIDialogAudioSubtitleSettings.h
            GUIDialogCMSSettings.h
            GUIDialogFullScreenInfo.h
            GUIDialogSubtitles.h
            GUIDialogTeletext.h
            GUIDialogVideoBookmarks.h
            GUIDialogVideoInfo.h
            GUIDialogVideoOSD.h
            GUIDialogVideoSettings.h)

core_add_library(video_dialogs)
