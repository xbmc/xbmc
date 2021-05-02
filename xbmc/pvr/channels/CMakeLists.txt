set(SOURCES PVRChannel.cpp
            PVRChannelGroup.cpp
            PVRChannelGroupInternal.cpp
            PVRChannelGroupMember.cpp
            PVRChannelGroups.cpp
            PVRChannelGroupsContainer.cpp
            PVRChannelNumber.cpp
            PVRRadioRDSInfoTag.cpp
            PVRChannelsPath.cpp)

set(HEADERS PVRChannel.h
            PVRChannelGroup.h
            PVRChannelGroupInternal.h
            PVRChannelGroupMember.h
            PVRChannelGroups.h
            PVRChannelGroupsContainer.h
            PVRChannelNumber.h
            PVRRadioRDSInfoTag.h
            PVRChannelsPath.h)

core_add_library(pvr_channels)
