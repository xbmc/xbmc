set(SOURCES GUIDialogPVRChannelManager.cpp
            GUIDialogPVRChannelsOSD.cpp
            GUIDialogPVRGroupManager.cpp
            GUIDialogPVRGuideInfo.cpp
            GUIDialogPVRChannelGuide.cpp
            GUIDialogPVRGuideSearch.cpp
            GUIDialogPVRRadioRDSInfo.cpp
            GUIDialogPVRRecordingInfo.cpp
            GUIDialogPVRRecordingSettings.cpp
            GUIDialogPVRTimerSettings.cpp
            GUIDialogPVRClientPriorities.cpp
            GUIDialogPVRItemsViewBase.cpp)

set(HEADERS GUIDialogPVRChannelManager.h
            GUIDialogPVRChannelsOSD.h
            GUIDialogPVRGroupManager.h
            GUIDialogPVRGuideInfo.h
            GUIDialogPVRChannelGuide.h
            GUIDialogPVRGuideSearch.h
            GUIDialogPVRRadioRDSInfo.h
            GUIDialogPVRRecordingInfo.h
            GUIDialogPVRRecordingSettings.h
            GUIDialogPVRTimerSettings.h
            GUIDialogPVRClientPriorities.h
            GUIDialogPVRItemsViewBase.h)

core_add_library(pvr_dialogs)
