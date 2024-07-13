set(SOURCES PVRChannel.cpp
            PVRChannelGroup.cpp
            PVRChannelGroupAllChannels.cpp
            PVRChannelGroupFactory.cpp
            PVRChannelGroupAllChannelsSingleClient.cpp
            PVRChannelGroupFromClient.cpp
            PVRChannelGroupMember.cpp
            PVRChannelGroupMergedByName.cpp
            PVRChannelGroupSettings.cpp
            PVRChannelGroups.cpp
            PVRChannelGroupsContainer.cpp
            PVRChannelNumber.cpp
            PVRRadioRDSInfoTag.cpp
            PVRChannelsPath.cpp)

set(HEADERS PVRChannel.h
            PVRChannelGroup.h
            PVRChannelGroupAllChannels.h
            PVRChannelGroupFactory.h
            PVRChannelGroupAllChannelsSingleClient.h
            PVRChannelGroupFromClient.h
            PVRChannelGroupFromUser.h
            PVRChannelGroupMember.h
            PVRChannelGroupMergedByName.h
            PVRChannelGroupSettings.h
            PVRChannelGroups.h
            PVRChannelGroupsContainer.h
            PVRChannelNumber.h
            PVRRadioRDSInfoTag.h
            PVRChannelsPath.h)

core_add_library(pvr_channels)
