set(SOURCES GUIViewStatePVR.cpp
            GUIWindowPVRBase.cpp
            GUIWindowPVRChannels.cpp
            GUIWindowPVRGuide.cpp
            GUIWindowPVRRecordings.cpp
            GUIWindowPVRSearch.cpp
            GUIWindowPVRTimers.cpp
            GUIWindowPVRTimersBase.cpp
            GUIWindowPVRTimerRules.cpp
            GUIEPGGridContainer.cpp
            GUIEPGGridContainerModel.cpp)

set(HEADERS GUIViewStatePVR.h
            GUIWindowPVRBase.h
            GUIWindowPVRChannels.h
            GUIWindowPVRGuide.h
            GUIWindowPVRRecordings.h
            GUIWindowPVRSearch.h
            GUIWindowPVRTimerRules.h
            GUIWindowPVRTimers.h
            GUIWindowPVRTimersBase.h
            GUIEPGGridContainer.h
            GUIEPGGridContainerModel.h)

core_add_library(pvr_windows)
