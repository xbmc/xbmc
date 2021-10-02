set(SOURCES PVRCachedImage.cpp
	    PVRCachedImages.cpp
            PVRChannelNumberInputHandler.cpp
            PVRContextMenus.cpp
            PVRDatabase.cpp
            PVREdl.cpp
            PVREventLogJob.cpp
            PVRItem.cpp
            PVRManager.cpp
            PVRPlaybackState.cpp
            PVRStreamProperties.cpp
            PVRThumbLoader.cpp)

set(HEADERS PVRCachedImage.h
            PVRCachedImages.h
            PVRChannelNumberInputHandler.h
            PVRContextMenus.h
            PVRDatabase.h
            PVREdl.h
            PVREventLogJob.h
            PVRItem.h
            PVRManager.h
            PVRPlaybackState.h
            PVRStreamProperties.h
            PVRThumbLoader.h)

core_add_library(pvr)
