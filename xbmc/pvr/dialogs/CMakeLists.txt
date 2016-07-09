set(SOURCES GUIDialogPVRChannelManager.cpp
            GUIDialogPVRChannelsOSD.cpp
            GUIDialogPVRGroupManager.cpp
            GUIDialogPVRGuideInfo.cpp
            GUIDialogPVRGuideOSD.cpp
            GUIDialogPVRGuideSearch.cpp
            GUIDialogPVRRadioRDSInfo.cpp
            GUIDialogPVRRecordingInfo.cpp
            GUIDialogPVRTimerSettings.cpp)

set(HEADERS GUIDialogPVRChannelManager.h
            GUIDialogPVRChannelsOSD.h
            GUIDialogPVRGroupManager.h
            GUIDialogPVRGuideInfo.h
            GUIDialogPVRGuideOSD.h
            GUIDialogPVRGuideSearch.h
            GUIDialogPVRRadioRDSInfo.h
            GUIDialogPVRRecordingInfo.h
            GUIDialogPVRTimerSettings.h)

core_add_library(pvr_dialogs)
