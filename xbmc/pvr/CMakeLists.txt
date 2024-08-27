set(SOURCES PVRCachedImage.cpp
	    PVRCachedImages.cpp
            PVRChannelGroupImageFileLoader.cpp
            PVRChannelNumberInputHandler.cpp
            PVRComponentRegistration.cpp
            PVRContextMenus.cpp
            PVRDatabase.cpp
            PVREdl.cpp
            PVREventLogJob.cpp
            PVRItem.cpp
            PVRManager.cpp
            PVRPlaybackState.cpp
            PVRStreamProperties.cpp
            PVRThumbLoader.cpp)

set(HEADERS IPVRComponent.h
            PVRCachedImage.h
            PVRCachedImages.h
            PVRChannelGroupImageFileLoader.h
            PVRChannelNumberInputHandler.h
            PVRComponentRegistration.h
            PVRConstants.h
            PVRContextMenus.h
            PVRDatabase.h
            PVRDescrambleInfo.h
            PVREdl.h
            PVREventLogJob.h
            PVRItem.h
            PVRManager.h
            PVRPlaybackState.h
            PVRSignalStatus.h
            PVRStreamProperties.h
            PVRThumbLoader.h)

core_add_library(pvr)
