/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

//! @todo use Observable here, so we can use event driven operations later

#include "PVRChannelGroup.h"

#include "ServiceBroker.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_channel_groups.h"
#include "pvr/PVRCachedImages.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelsPath.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgChannelData.h"
#include "pvr/epg/EpgInfoTag.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

using namespace PVR;

CPVRChannelGroup::CPVRChannelGroup(const CPVRChannelsPath& path,
                                   const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup)
  : m_allChannelsGroup(allChannelsGroup), m_path(path)
{
  GetSettings()->RegisterCallback(this);
}

CPVRChannelGroup::CPVRChannelGroup(const PVR_CHANNEL_GROUP& group,
                                   const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup)
  : m_iPosition(group.iPosition)
  , m_allChannelsGroup(allChannelsGroup)
  , m_path(group.bIsRadio, group.strGroupName)
{
  GetSettings()->RegisterCallback(this);
}

CPVRChannelGroup::~CPVRChannelGroup()
{
  GetSettings()->UnregisterCallback(this);
  Unload();
}

bool CPVRChannelGroup::operator==(const CPVRChannelGroup& right) const
{
  return (m_iGroupType == right.m_iGroupType &&
          m_iGroupId == right.m_iGroupId &&
          m_iPosition == right.m_iPosition &&
          m_path == right.m_path);
}

bool CPVRChannelGroup::operator!=(const CPVRChannelGroup& right) const
{
  return !(*this == right);
}

CCriticalSection CPVRChannelGroup::m_settingsSingletonCritSection;
std::weak_ptr<CPVRChannelGroupSettings> CPVRChannelGroup::m_settingsSingleton;

std::shared_ptr<CPVRChannelGroupSettings> CPVRChannelGroup::GetSettings() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (!m_settings)
  {
    std::unique_lock<CCriticalSection> singletonLock(m_settingsSingletonCritSection);
    const std::shared_ptr<CPVRChannelGroupSettings> settings = m_settingsSingleton.lock();
    if (settings)
    {
      m_settings = settings;
    }
    else
    {
      m_settings = std::make_shared<CPVRChannelGroupSettings>();
      m_settingsSingleton = m_settings;
    }
  }
  return m_settings;
}

bool CPVRChannelGroup::LoadFromDatabase(
    const std::map<std::pair<int, int>, std::shared_ptr<CPVRChannel>>& channels,
    const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  const int iChannelCount = m_iGroupId > 0 ? LoadFromDatabase(clients) : 0;
  CLog::LogFC(LOGDEBUG, LOGPVR, "Fetched {} {} group members from the database for group '{}'",
              iChannelCount, IsRadio() ? "radio" : "TV", GroupName());

  for (const auto& groupMember : m_members)
  {
    if (groupMember.second->Channel())
      continue;

    auto channel = channels.find(groupMember.first);
    if (channel == channels.end())
    {
      CLog::Log(LOGERROR, "Cannot find group member '{},{}' in channels!", groupMember.first.first,
                groupMember.first.second);
      // No workaround here, please. We need to find and fix the root cause of this case!
    }
    groupMember.second->SetChannel((*channel).second);
  }

  m_bLoaded = true;
  return true;
}

void CPVRChannelGroup::Unload()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_sortedMembers.clear();
  m_members.clear();
  m_failedClients.clear();
}

bool CPVRChannelGroup::UpdateFromClients(const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  if (GroupType() == PVR_GROUP_TYPE_USER_DEFINED || !GetSettings()->SyncChannelGroups())
    return true;

  // get the channel group members from the backends.
  std::vector<std::shared_ptr<CPVRChannelGroupMember>> groupMembers;
  CServiceBroker::GetPVRManager().Clients()->GetChannelGroupMembers(clients, this, groupMembers,
                                                                    m_failedClients);
  return UpdateGroupEntries(groupMembers);
}

const CPVRChannelsPath& CPVRChannelGroup::GetPath() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_path;
}

void CPVRChannelGroup::SetPath(const CPVRChannelsPath& path)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_path != path)
  {
    m_path = path;
    if (m_bLoaded)
    {
      // note: path contains both the radio flag and the group name, which are stored in the db
      m_bChanged = true;
      Persist(); //! @todo why must we persist immediately?
    }
  }
}

bool CPVRChannelGroup::SetChannelNumber(const std::shared_ptr<CPVRChannel>& channel, const CPVRChannelNumber& channelNumber)
{
  bool bReturn(false);
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (auto& member : m_sortedMembers)
  {
    if (*member->Channel() == *channel)
    {
      if (member->ChannelNumber() != channelNumber)
      {
        bReturn = true;
        member->SetChannelNumber(channelNumber);
      }
      break;
    }
  }

  return bReturn;
}

/********** sort methods **********/

struct sortByClientChannelNumber
{
  bool operator()(const std::shared_ptr<CPVRChannelGroupMember>& channel1,
                  const std::shared_ptr<CPVRChannelGroupMember>& channel2) const
  {
    if (channel1->ClientPriority() == channel2->ClientPriority())
    {
      if (channel1->ClientChannelNumber() == channel2->ClientChannelNumber())
        return channel1->Channel()->ChannelName() < channel2->Channel()->ChannelName();

      return channel1->ClientChannelNumber() < channel2->ClientChannelNumber();
    }
    return channel1->ClientPriority() > channel2->ClientPriority();
  }
};

struct sortByChannelNumber
{
  bool operator()(const std::shared_ptr<CPVRChannelGroupMember>& channel1,
                  const std::shared_ptr<CPVRChannelGroupMember>& channel2) const
  {
    return channel1->ChannelNumber() < channel2->ChannelNumber();
  }
};

void CPVRChannelGroup::Sort()
{
  if (GetSettings()->UseBackendChannelOrder())
    SortByClientChannelNumber();
  else
    SortByChannelNumber();
}

bool CPVRChannelGroup::SortAndRenumber()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  Sort();
  return Renumber();
}

void CPVRChannelGroup::SortByClientChannelNumber()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  std::sort(m_sortedMembers.begin(), m_sortedMembers.end(), sortByClientChannelNumber());
}

void CPVRChannelGroup::SortByChannelNumber()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  std::sort(m_sortedMembers.begin(), m_sortedMembers.end(), sortByChannelNumber());
}

bool CPVRChannelGroup::UpdateClientPriorities()
{
  const std::shared_ptr<CPVRClients> clients = CServiceBroker::GetPVRManager().Clients();
  bool bChanged = false;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  const bool bUseBackendChannelOrder = GetSettings()->UseBackendChannelOrder();
  for (auto& member : m_sortedMembers)
  {
    int iNewPriority = 0;

    if (bUseBackendChannelOrder)
    {
      std::shared_ptr<CPVRClient> client;
      if (!clients->GetCreatedClient(member->Channel()->ClientID(), client))
        continue;

      iNewPriority = client->GetPriority();
    }
    else
    {
      iNewPriority = 0;
    }

    bChanged |= (member->ClientPriority() != iNewPriority);
    member->SetClientPriority(iNewPriority);
  }

  return bChanged;
}

/********** getters **********/
std::shared_ptr<CPVRChannelGroupMember> CPVRChannelGroup::GetByUniqueID(
    const std::pair<int, int>& id) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto it = m_members.find(id);
  return it != m_members.end() ? it->second : std::shared_ptr<CPVRChannelGroupMember>();
}

std::shared_ptr<CPVRChannel> CPVRChannelGroup::GetByUniqueID(int iUniqueChannelId, int iClientID) const
{
  const std::shared_ptr<CPVRChannelGroupMember> groupMember =
      GetByUniqueID(std::make_pair(iClientID, iUniqueChannelId));
  return groupMember ? groupMember->Channel() : std::shared_ptr<CPVRChannel>();
}

std::shared_ptr<CPVRChannel> CPVRChannelGroup::GetByChannelID(int iChannelID) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& memberPair : m_members)
  {
    if (memberPair.second->Channel()->ChannelID() == iChannelID)
      return memberPair.second->Channel();
  }

  return {};
}

std::shared_ptr<CPVRChannelGroupMember> CPVRChannelGroup::GetLastPlayedChannelGroupMember(
    int iCurrentChannel /* = -1 */) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  std::shared_ptr<CPVRChannelGroupMember> groupMember;
  for (const auto& memberPair : m_members)
  {
    const std::shared_ptr<CPVRChannel> channel = memberPair.second->Channel();
    if (channel->ChannelID() != iCurrentChannel &&
        CServiceBroker::GetPVRManager().Clients()->IsCreatedClient(channel->ClientID()) &&
        channel->LastWatched() > 0 &&
        (!groupMember || channel->LastWatched() > groupMember->Channel()->LastWatched()))
    {
      groupMember = memberPair.second;
    }
  }

  return groupMember;
}

GroupMemberPair CPVRChannelGroup::GetLastAndPreviousToLastPlayedChannelGroupMember() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_sortedMembers.empty())
    return {};

  auto members = m_sortedMembers;
  lock.unlock();

  std::sort(members.begin(), members.end(), [](const auto& a, const auto& b) {
    return a->Channel()->LastWatched() > b->Channel()->LastWatched();
  });

  std::shared_ptr<CPVRChannelGroupMember> last;
  std::shared_ptr<CPVRChannelGroupMember> previousToLast;
  if (members[0]->Channel()->LastWatched())
  {
    last = members[0];
    if (members.size() > 1 && members[1]->Channel()->LastWatched())
      previousToLast = members[1];
  }

  return {last, previousToLast};
}

CPVRChannelNumber CPVRChannelGroup::GetChannelNumber(const std::shared_ptr<CPVRChannel>& channel) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::shared_ptr<CPVRChannelGroupMember> member = GetByUniqueID(channel->StorageId());
  return member ? member->ChannelNumber() : CPVRChannelNumber();
}

CPVRChannelNumber CPVRChannelGroup::GetClientChannelNumber(const std::shared_ptr<CPVRChannel>& channel) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::shared_ptr<CPVRChannelGroupMember> member = GetByUniqueID(channel->StorageId());
  return member ? member->ClientChannelNumber() : CPVRChannelNumber();
}

std::shared_ptr<CPVRChannelGroupMember> CPVRChannelGroup::GetByChannelNumber(
    const CPVRChannelNumber& channelNumber) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const bool bUseBackendChannelNumbers = GetSettings()->UseBackendChannelNumbers();
  for (const auto& member : m_sortedMembers)
  {
    CPVRChannelNumber activeChannelNumber =
        bUseBackendChannelNumbers ? member->ClientChannelNumber() : member->ChannelNumber();
    if (activeChannelNumber == channelNumber)
      return member;
  }

  return {};
}

std::shared_ptr<CPVRChannelGroupMember> CPVRChannelGroup::GetNextChannelGroupMember(
    const std::shared_ptr<CPVRChannelGroupMember>& groupMember) const
{
  std::shared_ptr<CPVRChannelGroupMember> nextMember;

  if (groupMember)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    for (auto it = m_sortedMembers.cbegin(); !nextMember && it != m_sortedMembers.cend(); ++it)
    {
      if (*it == groupMember)
      {
        do
        {
          if ((++it) == m_sortedMembers.cend())
            it = m_sortedMembers.cbegin();
          if ((*it)->Channel() && !(*it)->Channel()->IsHidden())
            nextMember = *it;
        } while (!nextMember && *it != groupMember);

        break;
      }
    }
  }

  return nextMember;
}

std::shared_ptr<CPVRChannelGroupMember> CPVRChannelGroup::GetPreviousChannelGroupMember(
    const std::shared_ptr<CPVRChannelGroupMember>& groupMember) const
{
  std::shared_ptr<CPVRChannelGroupMember> previousMember;

  if (groupMember)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    for (auto it = m_sortedMembers.crbegin(); !previousMember && it != m_sortedMembers.crend();
         ++it)
    {
      if (*it == groupMember)
      {
        do
        {
          if ((++it) == m_sortedMembers.crend())
            it = m_sortedMembers.crbegin();
          if ((*it)->Channel() && !(*it)->Channel()->IsHidden())
            previousMember = *it;
        } while (!previousMember && *it != groupMember);

        break;
      }
    }
  }
  return previousMember;
}

std::vector<std::shared_ptr<CPVRChannelGroupMember>> CPVRChannelGroup::GetMembers(
    Include eFilter /* = Include::ALL */) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (eFilter == Include::ALL)
    return m_sortedMembers;

  std::vector<std::shared_ptr<CPVRChannelGroupMember>> members;
  for (const auto& member : m_sortedMembers)
  {
    switch (eFilter)
    {
      case Include::ONLY_HIDDEN:
        if (!member->Channel()->IsHidden())
          continue;
        break;
      case Include::ONLY_VISIBLE:
        if (member->Channel()->IsHidden())
          continue;
       break;
      default:
        break;
    }

    members.emplace_back(member);
  }

  return members;
}

void CPVRChannelGroup::GetChannelNumbers(std::vector<std::string>& channelNumbers) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const bool bUseBackendChannelNumbers = GetSettings()->UseBackendChannelNumbers();
  for (const auto& member : m_sortedMembers)
  {
    CPVRChannelNumber activeChannelNumber =
        bUseBackendChannelNumbers ? member->ClientChannelNumber() : member->ChannelNumber();
    channelNumbers.emplace_back(activeChannelNumber.FormattedChannelNumber());
  }
}

int CPVRChannelGroup::LoadFromDatabase(const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  const std::shared_ptr<CPVRDatabase> database(CServiceBroker::GetPVRManager().GetTVDatabase());
  if (!database)
    return -1;

  const std::vector<std::shared_ptr<CPVRChannelGroupMember>> results =
      database->Get(*this, clients);

  std::vector<std::shared_ptr<CPVRChannelGroupMember>> membersToDelete;
  if (!results.empty())
  {
    const std::shared_ptr<CPVRClients> clients = CServiceBroker::GetPVRManager().Clients();

    std::unique_lock<CCriticalSection> lock(m_critSection);
    for (const auto& member : results)
    {
      // Consistency check.
      if (member->ClientID() > 0 && member->ChannelUID() > 0 && member->IsRadio() == IsRadio())
      {
        // Ignore data from unknown/disabled clients
        if (clients->IsEnabledClient(member->ClientID()))
        {
          m_sortedMembers.emplace_back(member);
          m_members.emplace(std::make_pair(member->ClientID(), member->ChannelUID()), member);
        }
      }
      else
      {
        CLog::LogF(LOGWARNING,
                   "Skipping member with channel database id {} of {} channel group '{}'. "
                   "Channel not found in the database or radio flag changed.",
                   member->ChannelDatabaseID(), IsRadio() ? "radio" : "TV", GroupName());
        membersToDelete.emplace_back(member);
      }
    }

    SortByChannelNumber();
  }

  DeleteGroupMembersFromDb(membersToDelete);

  return results.size() - membersToDelete.size();
}

void CPVRChannelGroup::DeleteGroupMembersFromDb(
    const std::vector<std::shared_ptr<CPVRChannelGroupMember>>& membersToDelete)
{
  if (!membersToDelete.empty())
  {
    const std::shared_ptr<CPVRDatabase> database = CServiceBroker::GetPVRManager().GetTVDatabase();
    if (!database)
    {
      CLog::LogF(LOGERROR, "No TV database");
      return;
    }

    // Note: We must lock the db the whole time, otherwise races may occur.
    database->Lock();

    bool commitPending = false;

    for (const auto& member : membersToDelete)
    {
      commitPending |= member->QueueDelete();

      size_t queryCount = database->GetDeleteQueriesCount();
      if (queryCount > CHANNEL_COMMIT_QUERY_COUNT_LIMIT)
        database->CommitDeleteQueries();
    }

    if (commitPending)
      database->CommitDeleteQueries();

    database->Unlock();
  }
}

bool CPVRChannelGroup::UpdateFromClient(const std::shared_ptr<CPVRChannelGroupMember>& groupMember)
{
  bool bChanged = false;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  const std::shared_ptr<CPVRChannel> channel = groupMember->Channel();
  const std::shared_ptr<CPVRChannelGroupMember> existingMember =
      GetByUniqueID(channel->StorageId());
  if (existingMember)
  {
    // update existing channel
    if (IsInternalGroup() && existingMember->Channel()->UpdateFromClient(channel))
    {
      CLog::LogFC(LOGDEBUG, LOGPVR, "Updated {} channel '{}' from PVR client",
                  IsRadio() ? "radio" : "TV", channel->ChannelName());
      bChanged = true;
    }

    existingMember->SetClientChannelNumber(channel->ClientChannelNumber());
    existingMember->SetOrder(channel->ClientOrder());

    if (existingMember->NeedsSave())
    {
      CLog::LogFC(LOGDEBUG, LOGPVR, "Updated {} channel group member '{}' in group '{}'",
                  IsRadio() ? "radio" : "TV", channel->ChannelName(), GroupName());
      bChanged = true;
    }
  }
  else
  {
    if (groupMember->GroupID() == -1)
      groupMember->SetGroupID(GroupID());

    m_sortedMembers.emplace_back(groupMember);
    m_members.emplace(channel->StorageId(), groupMember);

    CLog::LogFC(LOGDEBUG, LOGPVR, "Added {} channel group member '{}' to group '{}'",
                IsRadio() ? "radio" : "TV", channel->ChannelName(), GroupName());

    // create EPG for new channel
    if (IsInternalGroup() && channel->CreateEPG())
    {
      CLog::LogFC(LOGDEBUG, LOGPVR, "Created EPG for {} channel '{}' from PVR client",
                  IsRadio() ? "radio" : "TV", channel->ChannelName());
    }

    bChanged = true;
  }

  return bChanged;
}

bool CPVRChannelGroup::AddAndUpdateGroupMembers(
    const std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers)
{
  bool bChanged = false;

  // go through the group member list and check for updated or new members
  for (const auto& groupMember : groupMembers)
  {
    bChanged |= UpdateFromClient(groupMember);
  }

  return bChanged;
}

bool CPVRChannelGroup::HasValidDataForClient(int iClientId) const
{
  return std::find(m_failedClients.begin(), m_failedClients.end(), iClientId) ==
         m_failedClients.end();
}

bool CPVRChannelGroup::HasValidDataForAllClients() const
{
  return m_failedClients.empty();
}

bool CPVRChannelGroup::UpdateChannelNumbersFromAllChannelsGroup()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  bool bChanged = false;

  if (!IsInternalGroup())
  {
    // If we don't sync channel groups make sure the channel numbers are set from
    // the all channels group using the non default renumber call before sorting
    if (Renumber(IGNORE_NUMBERING_FROM_ONE) || SortAndRenumber())
      bChanged = true;
  }

  m_events.Publish(IsInternalGroup() || bChanged ? PVREvent::ChannelGroupInvalidated
                                                 : PVREvent::ChannelGroup);

  return bChanged;
}

std::vector<std::shared_ptr<CPVRChannelGroupMember>> CPVRChannelGroup::RemoveDeletedGroupMembers(
    const std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers)
{
  std::vector<std::shared_ptr<CPVRChannelGroupMember>> membersToRemove;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  // put group members into map to speedup the following lookups
  std::map<std::pair<int, int>, std::shared_ptr<CPVRChannelGroupMember>> membersMap;
  std::transform(groupMembers.begin(), groupMembers.end(),
                 std::inserter(membersMap, membersMap.end()),
                 [](const std::shared_ptr<CPVRChannelGroupMember>& member) {
                   return std::make_pair(member->Channel()->StorageId(), member);
                 });

  // check for deleted channels
  for (auto it = m_sortedMembers.begin(); it != m_sortedMembers.end();)
  {
    const std::shared_ptr<CPVRChannel> channel = (*it)->Channel();
    auto mapIt = membersMap.find(channel->StorageId());
    if (mapIt == membersMap.end())
    {
      if (HasValidDataForClient(channel->ClientID()))
      {
        CLog::Log(LOGINFO, "Removed stale {} channel '{}' from group '{}'",
                  IsRadio() ? "radio" : "TV", channel->ChannelName(), GroupName());
        membersToRemove.emplace_back(*it);

        m_members.erase(channel->StorageId());
        it = m_sortedMembers.erase(it);
        continue;
      }
    }
    else
    {
      membersMap.erase(mapIt);
    }
    ++it;
  }

  DeleteGroupMembersFromDb(membersToRemove);

  return membersToRemove;
}

bool CPVRChannelGroup::UpdateGroupEntries(
    const std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers)
{
  bool bReturn = false;
  bool bChanged = false;
  bool bRemoved = false;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  bRemoved = !RemoveDeletedGroupMembers(groupMembers).empty();
  bChanged = AddAndUpdateGroupMembers(groupMembers) || bRemoved;
  bChanged |= UpdateClientPriorities();

  if (bChanged)
  {
    // renumber to make sure all group members have a channel number. New members were added at the
    // back, so they'll get the highest numbers
    bool bRenumbered = SortAndRenumber();
    bReturn = Persist();
    m_events.Publish(HasNewChannels() || bRemoved || bRenumbered ? PVREvent::ChannelGroupInvalidated
                                                                 : PVREvent::ChannelGroup);
  }
  else
  {
    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannelGroup::RemoveFromGroup(const std::shared_ptr<CPVRChannel>& channel)
{
  bool bReturn = false;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (auto it = m_sortedMembers.begin(); it != m_sortedMembers.end(); ++it)
  {
    const auto storageId = (*it)->Channel()->StorageId();
    if (channel->StorageId() == storageId)
    {
      m_members.erase(storageId);
      m_sortedMembers.erase(it);
      bReturn = true;
      break;
    }
  }

  // no need to renumber if nothing was removed
  if (bReturn)
    Renumber();

  return bReturn;
}

bool CPVRChannelGroup::AppendToGroup(const std::shared_ptr<CPVRChannel>& channel)
{
  bool bReturn = false;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (!CPVRChannelGroup::IsGroupMember(channel))
  {
    const std::shared_ptr<CPVRChannelGroupMember> allGroupMember =
        m_allChannelsGroup->GetByUniqueID(channel->StorageId());

    if (allGroupMember)
    {
      unsigned int channelNumberMax = 0;
      for (const auto& member : m_sortedMembers)
      {
        if (member->ChannelNumber().GetChannelNumber() > channelNumberMax)
          channelNumberMax = member->ChannelNumber().GetChannelNumber();
      }

      const auto newMember = std::make_shared<CPVRChannelGroupMember>(GroupID(), GroupName(),
                                                                      allGroupMember->Channel());
      newMember->SetChannelNumber(CPVRChannelNumber(channelNumberMax + 1, 0));
      newMember->SetClientPriority(allGroupMember->ClientPriority());

      m_sortedMembers.emplace_back(newMember);
      m_members.emplace(allGroupMember->Channel()->StorageId(), newMember);

      SortAndRenumber();
      bReturn = true;
    }
  }
  return bReturn;
}

bool CPVRChannelGroup::IsGroupMember(const std::shared_ptr<CPVRChannel>& channel) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_members.find(channel->StorageId()) != m_members.end();
}

bool CPVRChannelGroup::Persist()
{
  bool bReturn(true);
  const std::shared_ptr<CPVRDatabase> database(CServiceBroker::GetPVRManager().GetTVDatabase());

  std::unique_lock<CCriticalSection> lock(m_critSection);

  // do not persist if the group is not fully loaded and was saved before.
  if (!m_bLoaded && m_iGroupId != INVALID_GROUP_ID)
    return bReturn;

  // Mark newly created groups as loaded so future updates will also be persisted...
  if (m_iGroupId == INVALID_GROUP_ID)
    m_bLoaded = true;

  if (database)
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Persisting channel group '{}' with {} channels", GroupName(),
                static_cast<int>(m_members.size()));

    bReturn = database->Persist(*this);
    m_bChanged = false;
  }
  else
  {
    bReturn = false;
  }

  return bReturn;
}

void CPVRChannelGroup::Delete()
{
  const std::shared_ptr<CPVRDatabase> database = CServiceBroker::GetPVRManager().GetTVDatabase();
  if (!database)
  {
    CLog::LogF(LOGERROR, "No TV database");
    return;
  }

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_iGroupId > 0)
  {
    if (database->Delete(*this))
      m_bDeleted = true;
  }
}

bool CPVRChannelGroup::Renumber(RenumberMode mode /* = NORMAL */)
{
  bool bReturn(false);
  unsigned int iChannelNumber(0);

  std::unique_lock<CCriticalSection> lock(m_critSection);
  const bool bUseBackendChannelNumbers = GetSettings()->UseBackendChannelNumbers();
  const bool bStartGroupChannelNumbersFromOne = GetSettings()->StartGroupChannelNumbersFromOne();

  CPVRChannelNumber currentChannelNumber;
  CPVRChannelNumber currentClientChannelNumber;
  for (auto& sortedMember : m_sortedMembers)
  {
    currentClientChannelNumber = sortedMember->ClientChannelNumber();
    if (!currentClientChannelNumber.IsValid())
      currentClientChannelNumber =
          m_allChannelsGroup->GetClientChannelNumber(sortedMember->Channel());

    if (bUseBackendChannelNumbers)
    {
      currentChannelNumber = currentClientChannelNumber;
    }
    else if (sortedMember->Channel()->IsHidden())
    {
      currentChannelNumber = CPVRChannelNumber(0, 0);
    }
    else
    {
      if (IsInternalGroup())
      {
        currentChannelNumber = CPVRChannelNumber(++iChannelNumber, 0);
      }
      else
      {
        if (bStartGroupChannelNumbersFromOne && mode != IGNORE_NUMBERING_FROM_ONE)
          currentChannelNumber = CPVRChannelNumber(++iChannelNumber, 0);
        else
          currentChannelNumber = m_allChannelsGroup->GetChannelNumber(sortedMember->Channel());
      }
    }

    if (sortedMember->ChannelNumber() != currentChannelNumber ||
        sortedMember->ClientChannelNumber() != currentClientChannelNumber)
    {
      bReturn = true;
      sortedMember->SetChannelNumber(currentChannelNumber);
      sortedMember->SetClientChannelNumber(currentClientChannelNumber);
    }
  }

  if (bReturn)
    Sort();

  return bReturn;
}

bool CPVRChannelGroup::HasNewChannels() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& memberPair : m_members)
  {
    if (memberPair.second->Channel()->ChannelID() <= 0)
      return true;
  }

  return false;
}

bool CPVRChannelGroup::HasChanges() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_bChanged;
}

bool CPVRChannelGroup::IsNew() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_iGroupId <= 0;
}

void CPVRChannelGroup::UseBackendChannelOrderChanged()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  UpdateClientPriorities();
  OnSettingChanged();
}

void CPVRChannelGroup::UseBackendChannelNumbersChanged()
{
  OnSettingChanged();
}

void CPVRChannelGroup::StartGroupChannelNumbersFromOneChanged()
{
  OnSettingChanged();
}

void CPVRChannelGroup::OnSettingChanged()
{
  //! @todo while pvr manager is starting up do accept setting changes.
  if (!CServiceBroker::GetPVRManager().IsStarted())
  {
    CLog::Log(LOGWARNING, "Channel group setting change ignored while PVR Manager is starting");
    return;
  }

  std::unique_lock<CCriticalSection> lock(m_critSection);

  CLog::LogFC(LOGDEBUG, LOGPVR,
              "Renumbering channel group '{}' to use the backend channel order and/or numbers",
              GroupName());

  // If we don't sync channel groups make sure the channel numbers are set from
  // the all channels group using the non default renumber call before sorting
  if (!GetSettings()->SyncChannelGroups())
    Renumber(IGNORE_NUMBERING_FROM_ONE);

  const bool bRenumbered = SortAndRenumber();
  Persist();

  m_events.Publish(bRenumbered ? PVREvent::ChannelGroupInvalidated : PVREvent::ChannelGroup);
}

int CPVRChannelGroup::GroupID() const
{
  return m_iGroupId;
}

void CPVRChannelGroup::SetGroupID(int iGroupId)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (iGroupId >= 0 && m_iGroupId != iGroupId)
  {
    m_iGroupId = iGroupId;

    // propagate the new id to the group members
    for (const auto& member : m_members)
      member.second->SetGroupID(iGroupId);
  }
}

void CPVRChannelGroup::SetGroupType(int iGroupType)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_iGroupType != iGroupType)
  {
    m_iGroupType = iGroupType;
    if (m_bLoaded)
      m_bChanged = true;
  }
}

int CPVRChannelGroup::GroupType() const
{
  return m_iGroupType;
}

std::string CPVRChannelGroup::GroupName() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_path.GetGroupName();
}

void CPVRChannelGroup::SetGroupName(const std::string& strGroupName)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_path.GetGroupName() != strGroupName)
  {
    m_path = CPVRChannelsPath(m_path.IsRadio(), strGroupName);
    if (m_bLoaded)
    {
      m_bChanged = true;
      Persist(); //! @todo why must we persist immediately?
    }
  }
}

bool CPVRChannelGroup::IsRadio() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_path.IsRadio();
}

time_t CPVRChannelGroup::LastWatched() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_iLastWatched;
}

void CPVRChannelGroup::SetLastWatched(time_t iLastWatched)
{
  const std::shared_ptr<CPVRDatabase> database(CServiceBroker::GetPVRManager().GetTVDatabase());

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_iLastWatched != iLastWatched)
  {
    m_iLastWatched = iLastWatched;
    if (m_bLoaded && database)
      database->UpdateLastWatched(*this);
  }
}

uint64_t CPVRChannelGroup::LastOpened() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_iLastOpened;
}

void CPVRChannelGroup::SetLastOpened(uint64_t iLastOpened)
{
  const std::shared_ptr<CPVRDatabase> database(CServiceBroker::GetPVRManager().GetTVDatabase());

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_iLastOpened != iLastOpened)
  {
    m_iLastOpened = iLastOpened;
    if (m_bLoaded && database)
      database->UpdateLastOpened(*this);
  }
}

bool CPVRChannelGroup::UpdateChannel(const std::pair<int, int>& storageId,
                                     const std::string& strChannelName,
                                     const std::string& strIconPath,
                                     int iEPGSource,
                                     int iChannelNumber,
                                     bool bHidden,
                                     bool bEPGEnabled,
                                     bool bParentalLocked,
                                     bool bUserSetIcon,
                                     bool bUserSetHidden)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  /* get the real channel from the group */
  const std::shared_ptr<CPVRChannel> channel = GetByUniqueID(storageId)->Channel();
  if (!channel)
    return false;

  channel->SetChannelName(strChannelName, true);
  channel->SetHidden(bHidden, bUserSetHidden);
  channel->SetLocked(bParentalLocked);
  channel->SetIconPath(strIconPath, bUserSetIcon);

  if (iEPGSource == 0)
    channel->SetEPGScraper("client");

  //! @todo add other scrapers
  channel->SetEPGEnabled(bEPGEnabled);

  /* set new values in the channel tag */
  if (bHidden)
  {
    // sort or previous changes will be overwritten
    Sort();

    RemoveFromGroup(channel);
  }
  else if (iChannelNumber > 0)
  {
    SetChannelNumber(channel, CPVRChannelNumber(iChannelNumber, 0));
  }

  return true;
}

size_t CPVRChannelGroup::Size() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_members.size();
}

bool CPVRChannelGroup::HasChannels() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return !m_members.empty();
}

bool CPVRChannelGroup::CreateChannelEpgs(bool bForce /* = false */)
{
  /* used only by internal channel groups */
  return true;
}

bool CPVRChannelGroup::SetHidden(bool bHidden)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_bHidden != bHidden)
  {
    m_bHidden = bHidden;
    if (m_bLoaded)
      m_bChanged = true;
  }

  return m_bChanged;
}

bool CPVRChannelGroup::IsHidden() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_bHidden;
}

int CPVRChannelGroup::GetPosition() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_iPosition;
}

void CPVRChannelGroup::SetPosition(int iPosition)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_iPosition != iPosition)
  {
    m_iPosition = iPosition;
    if (m_bLoaded)
      m_bChanged = true;
  }
}

int CPVRChannelGroup::CleanupCachedImages()
{
  std::vector<std::string> urlsToCheck;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    for (const auto& groupMember : m_members)
    {
      urlsToCheck.emplace_back(groupMember.second->Channel()->ClientIconPath());
    }
  }

  const std::string owner =
      StringUtils::Format(CPVRChannel::IMAGE_OWNER_PATTERN, IsRadio() ? "radio" : "tv");
  return CPVRCachedImages::Cleanup({{owner, ""}}, urlsToCheck);
}
