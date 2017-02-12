set(SOURCES PVRActionListener.cpp
            PVRGUIInfo.cpp
            PVRManager.cpp
            PVRDatabase.cpp
            PVRSettings.cpp
            PVRContextMenus.cpp
            PVRGUIActions.cpp
            PVRItem.cpp
            PVRChannelNumberInputHandler.cpp)

set(HEADERS PVRActionListener.h
            PVRDatabase.h
            PVREvent.h
            PVRGUIInfo.h
            PVRManager.h
            PVRSettings.h
            PVRContextMenus.h
            PVRGUIActions.h
            PVRItem.h
            PVRTypes.h
            PVRChannelNumberInputHandler.h)

core_add_library(pvr)
