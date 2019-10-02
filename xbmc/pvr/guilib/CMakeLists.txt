set(SOURCES GUIEPGGridContainer.cpp
            GUIEPGGridContainerModel.cpp
            PVRGUIActionListener.cpp
            PVRGUIActions.cpp
            PVRGUIChannelIconUpdater.cpp
            PVRGUIChannelNavigator.cpp
            PVRGUIProgressHandler.cpp)

set(HEADERS GUIEPGGridContainer.h
            GUIEPGGridContainerModel.h
            PVRGUIActionListener.h
            PVRGUIActions.h
            PVRGUIChannelIconUpdater.h
            PVRGUIChannelNavigator.h
            PVRGUIProgressHandler.h)

core_add_library(pvr_guilib)
