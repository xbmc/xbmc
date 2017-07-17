set(SOURCES GUIDialogAudioSubtitleSettings.cpp
            GUIDialogFullScreenInfo.cpp
            GUIDialogSubtitles.cpp
            GUIDialogTeletext.cpp
            GUIDialogVideoBookmarks.cpp
            GUIDialogVideoInfo.cpp
            GUIDialogVideoOSD.cpp
            GUIDialogVideoSettings.cpp)

set(HEADERS GUIDialogAudioSubtitleSettings.h
            GUIDialogFullScreenInfo.h
            GUIDialogSubtitles.h
            GUIDialogTeletext.h
            GUIDialogVideoBookmarks.h
            GUIDialogVideoInfo.h
            GUIDialogVideoOSD.h
            GUIDialogVideoSettings.h)

if(OPENGL_FOUND OR CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES GUIDialogCMSSettings.cpp)
  list(APPEND HEADERS GUIDialogCMSSettings.h)
endif()

core_add_library(video_dialogs)
