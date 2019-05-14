set(SOURCES PVRActionListener.cpp
            PVRGUIInfo.cpp
            PVRManager.cpp
            PVRDatabase.cpp
            PVRSettings.cpp
            PVRContextMenus.cpp
            PVRGUIActions.cpp
            PVRItem.cpp
            PVRChannelNumberInputHandler.cpp
            PVRJobs.cpp
            PVRGUIChannelNavigator.cpp
            PVRGUIDirectory.cpp
            PVRGUIProgressHandler.cpp
            PVRGUITimerInfo.cpp
            PVRGUITimesInfo.cpp)

set(HEADERS PVRActionListener.h
            PVRDatabase.h
            PVRGUIInfo.h
            PVRManager.h
            PVRSettings.h
            PVRContextMenus.h
            PVRGUIActions.h
            PVRItem.h
            PVRTypes.h
            PVRChannelNumberInputHandler.h
            PVRJobs.h
            PVRGUIChannelNavigator.h
            PVRGUIDirectory.h
            PVRGUIProgressHandler.h
            PVRGUITimerInfo.h
            PVRGUITimesInfo.h)

core_add_library(pvr)
