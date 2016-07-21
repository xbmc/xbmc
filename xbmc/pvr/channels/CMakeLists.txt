set(SOURCES PVRChannel.cpp
            PVRChannelGroup.cpp
            PVRChannelGroupInternal.cpp
            PVRChannelGroups.cpp
            PVRChannelGroupsContainer.cpp
            PVRRadioRDSInfoTag.cpp)

set(HEADERS PVRChannel.h
            PVRChannelGroup.h
            PVRChannelGroupInternal.h
            PVRChannelGroups.h
            PVRChannelGroupsContainer.h
            PVRRadioRDSInfoTag.h)

core_add_library(pvr_channels)
