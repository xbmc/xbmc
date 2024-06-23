/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupFactory.h"

#include "pvr/channels/PVRChannelGroupAllChannels.h"
#include "pvr/channels/PVRChannelGroupFromClient.h"
#include "pvr/channels/PVRChannelGroupFromUser.h"
#include "pvr/channels/PVRChannelsPath.h"
#include "utils/log.h"

using namespace PVR;

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroupFactory::CreateAllChannelsGroup(bool isRadio)
{
  return std::make_shared<CPVRChannelGroupAllChannels>(isRadio);
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroupFactory::CreateClientGroup(
    const PVR_CHANNEL_GROUP& groupData,
    int clientID,
    const std::shared_ptr<const CPVRChannelGroup>& allChannels)
{
  return std::make_shared<CPVRChannelGroupFromClient>(groupData, clientID, allChannels);
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroupFactory::CreateUserGroup(
    bool isRadio,
    const std::string& name,
    const std::shared_ptr<const CPVRChannelGroup>& allChannels)
{
  return std::make_shared<CPVRChannelGroupFromUser>(
      CPVRChannelsPath{isRadio, name, PVR_GROUP_CLIENT_ID_LOCAL}, allChannels);
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroupFactory::CreateGroup(
    int groupType,
    const CPVRChannelsPath& groupPath,
    const std::shared_ptr<const CPVRChannelGroup>& allChannels)
{
  switch (groupType)
  {
    case PVR_GROUP_TYPE_SYSTEM_ALL_CHANNELS:
      return std::make_shared<CPVRChannelGroupAllChannels>(groupPath);
    case PVR_GROUP_TYPE_USER:
      return std::make_shared<CPVRChannelGroupFromUser>(groupPath, allChannels);
    case PVR_GROUP_TYPE_CLIENT:
      return std::make_shared<CPVRChannelGroupFromClient>(groupPath, allChannels);
    default:
      CLog::LogFC(LOGERROR, LOGPVR, "Cannot create channel group '{}'. Unknown type {}.",
                  groupPath.GetGroupName(), groupType);
      return {};
  }
}

int CPVRChannelGroupFactory::GetGroupTypePriority(
    const std::shared_ptr<const CPVRChannelGroup>& group) const
{
  switch (group->GroupType())
  {
    case PVR_GROUP_TYPE_SYSTEM_ALL_CHANNELS:
      return 0; // highest
    case PVR_GROUP_TYPE_USER:
      return 1;
    case PVR_GROUP_TYPE_CLIENT:
      return 2;
    default:
      return 3;
  }
}
