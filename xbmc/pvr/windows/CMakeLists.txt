set(SOURCES GUIViewStatePVR.cpp
            GUIWindowPVRBase.cpp
            GUIWindowPVRChannels.cpp
            GUIWindowPVRGuide.cpp
            GUIWindowPVRRecordings.cpp
            GUIWindowPVRSearch.cpp
            GUIWindowPVRTimers.cpp
            GUIWindowPVRTimersBase.cpp
            GUIWindowPVRTimerRules.cpp)

set(HEADERS GUIViewStatePVR.h
            GUIWindowPVRBase.h
            GUIWindowPVRChannels.h
            GUIWindowPVRGuide.h
            GUIWindowPVRRecordings.h
            GUIWindowPVRSearch.h
            GUIWindowPVRTimerRules.h
            GUIWindowPVRTimers.h
            GUIWindowPVRTimersBase.h)

core_add_library(pvr_windows)
add_dependencies(pvr_windows libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(pvr_windows ffmpeg)
endif()
