/*
 *  Copyright (C) 2012-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupFromClient.h"

#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClients.h"
#include "utils/log.h"

using namespace PVR;

CPVRChannelGroupFromClient::CPVRChannelGroupFromClient(
    const CPVRChannelsPath& path, const std::shared_ptr<const CPVRChannelGroup>& allChannelsGroup)
  : CPVRChannelGroup(path, allChannelsGroup)
{
}

CPVRChannelGroupFromClient::CPVRChannelGroupFromClient(
    const PVR_CHANNEL_GROUP& group,
    int clientID,
    const std::shared_ptr<const CPVRChannelGroup>& allChannelsGroup)
  : CPVRChannelGroup(
        CPVRChannelsPath(group.bIsRadio, group.strGroupName ? group.strGroupName : "", clientID),
        allChannelsGroup)
{
  if (group.strGroupName)
    SetClientGroupName(group.strGroupName);

  SetClientPosition(group.iPosition);
}

bool CPVRChannelGroupFromClient::UpdateFromClients(
    const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  std::shared_ptr<CPVRClient> client;
  if (clients.empty())
  {
    // Requested to update groups from all clients
    client = CServiceBroker::GetPVRManager().GetClient(GetClientID());
    if (!client)
    {
      CLog::LogF(LOGERROR, "Unable to obtain client for group {}, clientid={}", GroupName(),
                 GetClientID());
      return false;
    }
  }
  else
  {
    const auto it =
        std::find_if(clients.cbegin(), clients.cend(),
                     [this](const auto& client) { return client->GetID() == GetClientID(); });
    if (it == clients.cend())
      return true; // this group is not provided by one of the clients to get the group members for

    client = *it;
  }

  // get the channel group members from the backends.
  std::vector<std::shared_ptr<CPVRChannelGroupMember>> groupMembers;
  CServiceBroker::GetPVRManager().Clients()->GetChannelGroupMembers({client}, this, groupMembers,
                                                                    m_failedClients);
  return UpdateGroupEntries(groupMembers);
}
