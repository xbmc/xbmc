/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupAllChannels.h"

#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/epg/EpgContainer.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <algorithm>
#include <iterator>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

using namespace PVR;

CPVRChannelGroupAllChannels::CPVRChannelGroupAllChannels(bool bRadio)
  : CPVRChannelGroup(
        CPVRChannelsPath(bRadio, g_localizeStrings.Get(19287), PVR_GROUP_CLIENT_ID_LOCAL), nullptr)
{
}

CPVRChannelGroupAllChannels::CPVRChannelGroupAllChannels(const CPVRChannelsPath& path)
  : CPVRChannelGroup(path, nullptr)
{
}

CPVRChannelGroupAllChannels::~CPVRChannelGroupAllChannels()
{
}

void CPVRChannelGroupAllChannels::CheckGroupName()
{
  //! @todo major design flaw to fix: channel and group URLs must not contain the group name!

  // Ensure the group name is still correct, or channels may fail to load after a locale change
  if (!IsUserSetName())
  {
    if (SetGroupName(g_localizeStrings.Get(19287)))
      Persist();
  }
}

bool CPVRChannelGroupAllChannels::UpdateFromClients(
    const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  // get the channels from the given clients
  std::vector<std::shared_ptr<CPVRChannel>> channels;
  CServiceBroker::GetPVRManager().Clients()->GetChannels(clients, IsRadio(), channels,
                                                         m_failedClients);

  // create group members for the channels
  std::vector<std::shared_ptr<CPVRChannelGroupMember>> groupMembers;
  std::transform(channels.cbegin(), channels.cend(), std::back_inserter(groupMembers),
                 [this](const auto& channel) {
                   return std::make_shared<CPVRChannelGroupMember>(GroupID(), GroupName(),
                                                                   GetClientID(), channel);
                 });

  return UpdateGroupEntries(groupMembers);
}

std::vector<std::shared_ptr<CPVRChannelGroupMember>> CPVRChannelGroupAllChannels::
    RemoveDeletedGroupMembers(
        const std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers)
{
  std::vector<std::shared_ptr<CPVRChannelGroupMember>> removedMembers =
      CPVRChannelGroup::RemoveDeletedGroupMembers(groupMembers);
  if (!removedMembers.empty())
  {
    const std::shared_ptr<CPVRDatabase> database = CServiceBroker::GetPVRManager().GetTVDatabase();
    if (!database)
    {
      CLog::LogF(LOGERROR, "No TV database");
    }
    else
    {
      std::vector<std::shared_ptr<CPVREpg>> epgsToRemove;
      for (const auto& member : removedMembers)
      {
        const auto channel = member->Channel();
        const auto epg = channel->GetEPG();
        if (epg)
          epgsToRemove.emplace_back(epg);

        // Note: We need to obtain a lock for every channel instance before we can lock
        //       the TV db. This order is important. Otherwise deadlocks may occur.
        channel->Lock();
      }

      // Note: We must lock the db the whole time, otherwise races may occur.
      database->Lock();

      bool commitPending = false;

      for (const auto& member : removedMembers)
      {
        // since channel was not found in the all channels group, it was deleted from the backend

        const auto channel = member->Channel();
        commitPending |= channel->QueueDelete();
        channel->Unlock();

        size_t queryCount = database->GetDeleteQueriesCount();
        if (queryCount > CHANNEL_COMMIT_QUERY_COUNT_LIMIT)
          database->CommitDeleteQueries();
      }

      if (commitPending)
        database->CommitDeleteQueries();

      database->Unlock();

      // delete the EPG data for the removed channels
      CServiceBroker::GetPVRManager().EpgContainer().QueueDeleteEpgs(epgsToRemove);
    }
  }
  return removedMembers;
}

bool CPVRChannelGroupAllChannels::AppendToGroup(
    const std::shared_ptr<const CPVRChannelGroupMember>& groupMember)
{
  if (IsGroupMember(groupMember))
    return false;

  groupMember->Channel()->SetHidden(false, true);

  SortAndRenumber();
  return true;
}

bool CPVRChannelGroupAllChannels::RemoveFromGroup(
    const std::shared_ptr<const CPVRChannelGroupMember>& groupMember)
{
  if (!IsGroupMember(groupMember))
    return false;

  groupMember->Channel()->SetHidden(true, true);

  SortAndRenumber();
  return true;
}

bool CPVRChannelGroupAllChannels::IsGroupMember(
    const std::shared_ptr<const CPVRChannelGroupMember>& groupMember) const
{
  return !groupMember->Channel()->IsHidden();
}
