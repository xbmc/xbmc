set(SOURCES GUIEPGGridContainer.cpp
            GUIEPGGridContainerModel.cpp
            PVRGUIActionListener.cpp
            PVRGUIActionsChannels.cpp
            PVRGUIActionsClients.cpp
            PVRGUIActionsDatabase.cpp
            PVRGUIActionsEPG.cpp
            PVRGUIActionsUtils.cpp
            PVRGUIActionsParentalControl.cpp
            PVRGUIActionsPlayback.cpp
            PVRGUIActionsPowerManagement.cpp
            PVRGUIActionsRecordings.cpp
            PVRGUIActionsTimers.cpp
            PVRGUIChannelIconUpdater.cpp
            PVRGUIChannelNavigator.cpp
            PVRGUIProgressHandler.cpp)

set(HEADERS GUIEPGGridContainer.h
            GUIEPGGridContainerModel.h
            PVRGUIActionListener.h
            PVRGUIActionsChannels.h
            PVRGUIActionsClients.h
            PVRGUIActionsDatabase.h
            PVRGUIActionsEPG.h
            PVRGUIActionsUtils.h
            PVRGUIActionsParentalControl.h
            PVRGUIActionsPlayback.h
            PVRGUIActionsPowerManagement.h
            PVRGUIActionsRecordings.h
            PVRGUIActionsTimers.h
            PVRGUIChannelIconUpdater.h
            PVRGUIChannelNavigator.h
            PVRGUIProgressHandler.h
            PVRGUIRecordingsPlayActionProcessor.h)

core_add_library(pvr_guilib)
