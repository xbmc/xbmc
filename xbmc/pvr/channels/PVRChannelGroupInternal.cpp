/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupInternal.h"

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

CPVRChannelGroupInternal::CPVRChannelGroupInternal(bool bRadio)
  : CPVRChannelGroup(
        CPVRChannelsPath(bRadio, g_localizeStrings.Get(19287), PVR_GROUP_CLIENT_ID_LOCAL),
        PVR_GROUP_TYPE_ALL_CHANNELS,
        nullptr),
    m_iHiddenChannels(0)
{
}

CPVRChannelGroupInternal::CPVRChannelGroupInternal(const CPVRChannelsPath& path)
  : CPVRChannelGroup(path, PVR_GROUP_TYPE_ALL_CHANNELS, nullptr), m_iHiddenChannels(0)
{
}

CPVRChannelGroupInternal::~CPVRChannelGroupInternal()
{
}

bool CPVRChannelGroupInternal::LoadFromDatabase(
    const std::map<std::pair<int, int>, std::shared_ptr<CPVRChannel>>& channels,
    const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  if (CPVRChannelGroup::LoadFromDatabase(channels, clients))
  {
    for (const auto& groupMember : m_members)
    {
      groupMember.second->Channel()->CreateEPG();
    }

    UpdateChannelPaths();
    return true;
  }

  CLog::LogF(LOGERROR, "Failed to load channels");
  return false;
}

void CPVRChannelGroupInternal::Unload()
{
  CPVRChannelGroup::Unload();
}

void CPVRChannelGroupInternal::CheckGroupName()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  /* check whether the group name is still correct, or channels will fail to load after the language setting changed */
  const std::string& strNewGroupName = g_localizeStrings.Get(19287);
  if (GroupName() != strNewGroupName)
  {
    SetGroupName(strNewGroupName);
    UpdateChannelPaths();
  }
}

void CPVRChannelGroupInternal::UpdateChannelPaths()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_iHiddenChannels = 0;
  for (auto& groupMemberPair : m_members)
  {
    if (groupMemberPair.second->Channel()->IsHidden())
      ++m_iHiddenChannels;
    else
      groupMemberPair.second->SetGroupName(GroupName());
  }
}

bool CPVRChannelGroupInternal::UpdateFromClients(
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

std::vector<std::shared_ptr<CPVRChannelGroupMember>> CPVRChannelGroupInternal::
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
        // since channel was not found in the internal group, it was deleted from the backend

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

bool CPVRChannelGroupInternal::AppendToGroup(
    const std::shared_ptr<CPVRChannelGroupMember>& groupMember)
{
  if (IsGroupMember(groupMember))
    return false;

  groupMember->Channel()->SetHidden(false, true);

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_iHiddenChannels > 0)
    m_iHiddenChannels--;

  const size_t iChannelNumber = m_members.size() - m_iHiddenChannels;
  allChannelsGroupMember->SetChannelNumber(CPVRChannelNumber(static_cast<int>(iChannelNumber), 0));

  SortAndRenumber();
  return true;
}

bool CPVRChannelGroupInternal::RemoveFromGroup(
    const std::shared_ptr<CPVRChannelGroupMember>& groupMember)
{
  if (!IsGroupMember(groupMember))
    return false;

  groupMember->Channel()->SetHidden(true, true);

  std::unique_lock<CCriticalSection> lock(m_critSection);

  ++m_iHiddenChannels;

  SortAndRenumber();
  return true;
}

bool CPVRChannelGroupInternal::IsGroupMember(
    const std::shared_ptr<CPVRChannelGroupMember>& groupMember) const
{
  return !groupMember->Channel()->IsHidden();
}
