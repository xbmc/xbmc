/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupAllChannelsSingleClient.h"

#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelsPath.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <unordered_set>

using namespace PVR;

std::vector<std::shared_ptr<CPVRChannelGroup>> CPVRChannelGroupAllChannelsSingleClient::
    CreateMissingGroups(const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup,
                        const std::vector<std::shared_ptr<CPVRChannelGroup>>& allChannelGroups)
{
  const std::vector<std::shared_ptr<CPVRChannelGroupMember>> allGroupMembers{
      allChannelsGroup->GetMembers()};

  // Create a unique list of active client ids from current members of the all channels list.
  std::unordered_set<int> clientIds;
  for (const auto& member : allGroupMembers)
  {
    clientIds.insert(member->ChannelClientID());
  }

  // If only one client is active, global all channels group would be identical to the all
  // channels group for the client. No need to create it.
  if (clientIds.size() <= 1)
    return {};

  std::vector<std::shared_ptr<CPVRChannelGroup>> addedGroups;

  for (int clientId : clientIds)
  {
    const std::shared_ptr<const CPVRClient> client{
        CServiceBroker().GetPVRManager().GetClient(clientId)};
    if (!client)
    {
      CLog::LogFC(LOGERROR, LOGPVR, "Failed to get client instance for client id {}", clientId);
      continue;
    }

    // Create a group containg all channels for this client, if not yet existing.
    const auto it = std::find_if(allChannelGroups.cbegin(), allChannelGroups.cend(),
                                 [&client](const auto& group)
                                 {
                                   return (group->GroupType() ==
                                           PVR_GROUP_TYPE_SYSTEM_ALL_CHANNELS_SINGLE_CLIENT) &&
                                          (group->GetClientID() == client->GetID());
                                 });
    if (it == allChannelGroups.cend())
    {
      const std::string name{
          StringUtils::Format(g_localizeStrings.Get(859), client->GetFullClientName())};
      const CPVRChannelsPath path{allChannelsGroup->IsRadio(), name, client->GetID()};
      addedGroups.emplace_back(
          std::make_shared<CPVRChannelGroupAllChannelsSingleClient>(path, allChannelsGroup));
    }
  }

  return addedGroups;
}

bool CPVRChannelGroupAllChannelsSingleClient::UpdateGroupMembers(
    const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup,
    const std::vector<std::shared_ptr<CPVRChannelGroup>>& allChannelGroups)
{
  std::vector<std::shared_ptr<CPVRChannelGroupMember>> groupMembers;

  // Collect and populate matching members.
  const auto allChannelsGroupMembers{allChannelsGroup->GetMembers()};
  for (const auto& member : allChannelsGroupMembers)
  {
    if (member->ChannelClientID() != GetClientID())
      continue;

    groupMembers.emplace_back(std::make_shared<CPVRChannelGroupMember>(
        GroupID(), GroupName(), GetClientID(), member->Channel()));
  }

  return UpdateGroupEntries(groupMembers);
}
