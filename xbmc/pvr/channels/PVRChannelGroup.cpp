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
#include <iterator>
#include <memory>
#include <mutex>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

using namespace PVR;

CPVRChannelGroup::CPVRChannelGroup(const CPVRChannelsPath& path,
                                   const std::shared_ptr<const CPVRChannelGroup>& allChannelsGroup)
  : m_allChannelsGroup(allChannelsGroup), m_path(path)
{
  GetSettings()->RegisterCallback(this);
}

CPVRChannelGroup::~CPVRChannelGroup()
{
  GetSettings()->UnregisterCallback(this);
}

bool CPVRChannelGroup::operator==(const CPVRChannelGroup& right) const
{
  return (GroupType() == right.GroupType() && m_iGroupId == right.m_iGroupId &&
          m_iPosition == right.m_iPosition && m_path == right.m_path &&
          m_clientPriority == right.m_clientPriority && m_isUserSetName == right.m_isUserSetName &&
          m_clientGroupName == right.m_clientGroupName &&
          m_iClientPosition == right.m_iClientPosition);
}

bool CPVRChannelGroup::operator!=(const CPVRChannelGroup& right) const
{
  return !(*this == right);
}

void CPVRChannelGroup::FillAddonData(PVR_CHANNEL_GROUP& group) const
{
  group = {};
  group.bIsRadio = IsRadio();
  strncpy(group.strGroupName, ClientGroupName().c_str(), sizeof(group.strGroupName) - 1);
  group.iPosition = GetClientPosition();
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
    groupMember.second->SetGroupName(GroupName());

    auto channel = groupMember.second->Channel();
    if (channel)
      continue;

    auto channelIt = channels.find(groupMember.first);
    if (channelIt == channels.end())
    {
      CLog::Log(LOGERROR, "Cannot find group member '{},{}' in channels!", groupMember.first.first,
                groupMember.first.second);
      // No workaround here, please. We need to find and fix the root cause of this case!
    }

    channel = (*channelIt).second;
    groupMember.second->SetChannel(channel);

    // Create EPG for loaded channel
    channel->CreateEPG();
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

int CPVRChannelGroup::GetClientID() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_path.GetGroupClientID();
}

void CPVRChannelGroup::SetClientID(int clientID)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_path.GetGroupClientID() != clientID)
  {
    m_path = CPVRChannelsPath(m_path.IsRadio(), m_path.GetGroupName(), clientID);
    if (m_bLoaded)
    {
      m_bChanged = true;
      Persist(); //! @todo why must we persist immediately?
    }
  }
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

/********** sort methods **********/

struct sortByClientChannelNumber
{
  bool operator()(const std::shared_ptr<const CPVRChannelGroupMember>& channel1,
                  const std::shared_ptr<const CPVRChannelGroupMember>& channel2) const
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
  bool operator()(const std::shared_ptr<const CPVRChannelGroupMember>& channel1,
                  const std::shared_ptr<const CPVRChannelGroupMember>& channel2) const
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

void CPVRChannelGroup::UpdateClientPriorities()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  // Update own priority value
  m_clientPriority.reset();
  GetClientPriority();

  // Update group member's value and re-sort members
  if (UpdateMembersClientPriority())
    SortAndRenumber();
}

bool CPVRChannelGroup::ShouldBeIgnored(
    const std::vector<std::shared_ptr<CPVRChannelGroup>>& allChannelGroups) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  // Empty group should be ignored.
  return m_members.empty();
}

bool CPVRChannelGroup::UpdateMembersClientPriority()
{
  const std::shared_ptr<const CPVRClients> clients = CServiceBroker::GetPVRManager().Clients();
  bool bChanged = false;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  const bool bUseBackendChannelOrder = GetSettings()->UseBackendChannelOrder();
  for (auto& member : m_sortedMembers)
  {
    int iNewPriority = 0;

    if (bUseBackendChannelOrder)
    {
      const std::shared_ptr<const CPVRClient> client =
          clients->GetCreatedClient(member->Channel()->ClientID());
      if (!client)
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

std::shared_ptr<CPVRChannel> CPVRChannelGroup::GetByUniqueID(int iUniqueChannelId,
                                                             int iClientID) const
{
  const std::shared_ptr<const CPVRChannelGroupMember> groupMember =
      GetByUniqueID(std::make_pair(iClientID, iUniqueChannelId));
  return groupMember ? groupMember->Channel() : std::shared_ptr<CPVRChannel>();
}

std::shared_ptr<CPVRChannel> CPVRChannelGroup::GetByChannelID(int iChannelID) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto it =
      std::find_if(m_members.cbegin(), m_members.cend(), [iChannelID](const auto& member) {
        return member.second->Channel()->ChannelID() == iChannelID;
      });
  return it != m_members.cend() ? (*it).second->Channel() : std::shared_ptr<CPVRChannel>();
}

std::shared_ptr<CPVRChannelGroupMember> CPVRChannelGroup::GetLastPlayedChannelGroupMember(
    int iCurrentChannel /* = -1 */) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  std::shared_ptr<CPVRChannelGroupMember> groupMember;
  for (const auto& memberPair : m_members)
  {
    const std::shared_ptr<const CPVRChannel> channel = memberPair.second->Channel();
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

CPVRChannelNumber CPVRChannelGroup::GetChannelNumber(
    const std::shared_ptr<const CPVRChannel>& channel) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::shared_ptr<const CPVRChannelGroupMember> member = GetByUniqueID(channel->StorageId());
  return member ? member->ChannelNumber() : CPVRChannelNumber();
}

CPVRChannelNumber CPVRChannelGroup::GetClientChannelNumber(
    const std::shared_ptr<const CPVRChannel>& channel) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::shared_ptr<const CPVRChannelGroupMember> member = GetByUniqueID(channel->StorageId());
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
    const std::shared_ptr<const CPVRChannelGroupMember>& groupMember) const
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
    const std::shared_ptr<const CPVRChannelGroupMember>& groupMember) const
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
  const std::shared_ptr<const CPVRDatabase> database(
      CServiceBroker::GetPVRManager().GetTVDatabase());
  if (!database)
    return -1;

  const std::vector<std::shared_ptr<CPVRChannelGroupMember>> results =
      database->Get(*this, clients);

  std::vector<std::shared_ptr<CPVRChannelGroupMember>> membersToDelete;
  if (!results.empty())
  {
    const std::shared_ptr<const CPVRClients> allClients = CServiceBroker::GetPVRManager().Clients();

    std::unique_lock<CCriticalSection> lock(m_critSection);
    for (const auto& member : results)
    {
      // Consistency checks.

      if (GetClientID() != PVR_GROUP_CLIENT_ID_UNKNOWN &&
          GetClientID() != PVR_GROUP_CLIENT_ID_LOCAL && GetClientID() != member->ChannelClientID())
      {
        CLog::LogF(LOGWARNING,
                   "Skipping member with channel database id {} of {} channel group '{}'. "
                   "Channel client id does not match group client id.",
                   member->ChannelDatabaseID(), IsRadio() ? "radio" : "TV", GroupName());
        membersToDelete.emplace_back(member);
        continue;
      }

      if (member->ChannelClientID() > 0 && member->ChannelUID() > 0 &&
          member->IsRadio() == IsRadio())
      {
        // Ignore data from unknown/disabled clients
        if (allClients->IsEnabledClient(member->ChannelClientID()))
        {
          m_sortedMembers.emplace_back(member);
          m_members.emplace(std::make_pair(member->ChannelClientID(), member->ChannelUID()),
                            member);
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
    if (IsChannelsOwner() && existingMember->Channel()->UpdateFromClient(channel))
    {
      CLog::LogFC(LOGDEBUG, LOGPVR, "Updated {} channel '{}' from PVR client",
                  IsRadio() ? "radio" : "TV", channel->ChannelName());
      bChanged = true;
    }

    existingMember->SetClientChannelNumber(channel->ClientChannelNumber());
    existingMember->SetOrder(groupMember->Order());

    if (existingMember->NeedsSave())
    {
      CLog::LogFC(LOGDEBUG, LOGPVR, "Updated {} channel group member '{}' in group '{}'",
                  IsRadio() ? "radio" : "TV", channel->ChannelName(), GroupName());
      bChanged = true;
    }
  }
  else
  {
    if (groupMember->GroupID() == INVALID_GROUP_ID)
      groupMember->SetGroupID(GroupID());

    // Seeing this channel for the very first time. Remember when it was added.
    if (IsChannelsOwner() && !channel->DateTimeAdded().IsValid())
      channel->SetDateTimeAdded(CDateTime::GetUTCDateTime());

    m_sortedMembers.emplace_back(groupMember);
    m_members.emplace(channel->StorageId(), groupMember);

    CLog::LogFC(LOGDEBUG, LOGPVR, "Added {} channel group member '{}' to group '{}'",
                IsRadio() ? "radio" : "TV", channel->ChannelName(), GroupName());

    // Create EPG for new channel
    channel->CreateEPG();

    bChanged = true;
  }

  return bChanged;
}

bool CPVRChannelGroup::AddAndUpdateGroupMembers(
    const std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers)
{
  return std::accumulate(groupMembers.cbegin(), groupMembers.cend(), false,
                         [this](bool changed, const auto& groupMember) {
                           return UpdateFromClient(groupMember) ? true : changed;
                         });
}

bool CPVRChannelGroup::HasValidDataForClient(int iClientId) const
{
  return std::find(m_failedClients.begin(), m_failedClients.end(), iClientId) ==
         m_failedClients.end();
}

bool CPVRChannelGroup::HasValidDataForClients(
    const std::vector<std::shared_ptr<CPVRClient>>& clients) const
{
  return m_failedClients.empty() ||
         std::none_of(clients.cbegin(), clients.cend(),
                      [this](const std::shared_ptr<const CPVRClient>& client) {
                        return !HasValidDataForClient(client->GetID());
                      });
}

bool CPVRChannelGroup::UpdateChannelNumbersFromAllChannelsGroup()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  bool bChanged = false;

  if (!IsChannelsOwner())
  {
    // Make sure the channel numbers are set from the all channels group using the non default
    // renumber call before sorting.
    if (Renumber(IGNORE_NUMBERING_FROM_ONE) || SortAndRenumber())
      bChanged = true;
  }

  m_events.Publish(IsChannelsOwner() || bChanged ? PVREvent::ChannelGroupInvalidated
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

  // check for deleted/invalid channels
  for (auto it = m_sortedMembers.begin(); it != m_sortedMembers.end();)
  {
    const std::shared_ptr<const CPVRChannel> channel = (*it)->Channel();

    if (GetClientID() >= 0 && GetClientID() != channel->ClientID())
    {
      CLog::LogF(LOGDEBUG,
                 "Removed {} channel '{}' from group '{}' because it has the wrong client id",
                 IsRadio() ? "radio" : "TV", channel->ChannelName(), GroupName());
      membersToRemove.emplace_back(*it);

      m_members.erase(channel->StorageId());
      it = m_sortedMembers.erase(it);
      continue;
    }

    auto mapIt = membersMap.find(channel->StorageId());
    if (mapIt == membersMap.end())
    {
      if (HasValidDataForClient(channel->ClientID()))
      {
        CLog::LogFC(LOGDEBUG, LOGPVR, "Removed stale {} channel '{}' from group '{}'. clientId={}",
                    IsRadio() ? "radio" : "TV", channel->ChannelName(), GroupName(),
                    channel->ClientID());
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
  bChanged |= UpdateMembersClientPriority();

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

bool CPVRChannelGroup::RemoveFromGroup(
    const std::shared_ptr<const CPVRChannelGroupMember>& groupMember)
{
  bool bReturn = false;
  const auto channel = groupMember->Channel();

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

bool CPVRChannelGroup::AppendToGroup(
    const std::shared_ptr<const CPVRChannelGroupMember>& groupMember)
{
  bool bReturn = false;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (!CPVRChannelGroup::IsGroupMember(groupMember))
  {
    unsigned int channelNumberMax =
        std::accumulate(m_sortedMembers.cbegin(), m_sortedMembers.cend(), 0,
                        [](unsigned int last, const auto& member) {
                          return (member->ChannelNumber().GetChannelNumber() > last)
                                     ? member->ChannelNumber().GetChannelNumber()
                                     : last;
                        });

    const auto channel = groupMember->Channel();
    const auto newMember =
        std::make_shared<CPVRChannelGroupMember>(GroupID(), GroupName(), GetClientID(), channel);
    newMember->SetChannelNumber(CPVRChannelNumber(channelNumberMax + 1, 0));
    newMember->SetClientPriority(groupMember->ClientPriority());

    m_sortedMembers.emplace_back(newMember);
    m_members.emplace(channel->StorageId(), newMember);

    SortAndRenumber();
    bReturn = true;
  }
  return bReturn;
}

bool CPVRChannelGroup::IsGroupMember(
    const std::shared_ptr<const CPVRChannelGroupMember>& groupMember) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_members.find(groupMember->Channel()->StorageId()) != m_members.end();
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
    const auto channel = sortedMember->Channel();

    currentClientChannelNumber = sortedMember->ClientChannelNumber();
    if (!currentClientChannelNumber.IsValid())
      currentClientChannelNumber = channel->ClientChannelNumber();

    if (bUseBackendChannelNumbers)
    {
      currentChannelNumber = currentClientChannelNumber;
    }
    else if (channel->IsHidden())
    {
      currentChannelNumber = CPVRChannelNumber(0, 0);
    }
    else
    {
      if (IsChannelsOwner() ||
          (bStartGroupChannelNumbersFromOne && mode != IGNORE_NUMBERING_FROM_ONE))
        currentChannelNumber = CPVRChannelNumber(++iChannelNumber, 0);
      else
        currentChannelNumber = m_allChannelsGroup->GetChannelNumber(channel);
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
  return std::any_of(m_members.cbegin(), m_members.cend(),
                     [](const auto& member) { return member.second->Channel()->ChannelID() <= 0; });
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
  UpdateMembersClientPriority();
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

  // Make sure the channel numbers are set from the all channels group using the non default
  // renumber call before sorting.
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

std::string CPVRChannelGroup::GroupName() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_path.GetGroupName();
}

void CPVRChannelGroup::SetGroupName(const std::string& strGroupName,
                                    bool isUserSetName /* = false */)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_path.GetGroupName() != strGroupName)
  {
    m_isUserSetName = isUserSetName;
    m_path = CPVRChannelsPath(m_path.IsRadio(), strGroupName, m_path.GetGroupClientID());

    // Update group members, for which group name is part of their path
    for (auto& member : m_sortedMembers)
    {
      member->SetGroupName(strGroupName);
    }

    if (m_bLoaded)
    {
      m_bChanged = true;
      Persist(); //! @todo why must we persist immediately?
    }
  }
}

std::string CPVRChannelGroup::ClientGroupName() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_clientGroupName;
}

void CPVRChannelGroup::SetClientGroupName(const std::string& groupName)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_clientGroupName != groupName)
  {
    m_clientGroupName = groupName;

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

bool CPVRChannelGroup::HasHiddenChannels() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::any_of(m_members.cbegin(), m_members.cend(),
                     [](const auto& member) { return member.second->Channel()->IsHidden(); });
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

int CPVRChannelGroup::GetClientPosition() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_iClientPosition;
}

void CPVRChannelGroup::SetClientPosition(int iPosition)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_iClientPosition != iPosition)
  {
    m_iClientPosition = iPosition;
    if (m_bLoaded)
      m_bChanged = true;
  }
}

int CPVRChannelGroup::CleanupCachedImages()
{
  std::vector<std::string> urlsToCheck;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    std::transform(
        m_members.cbegin(), m_members.cend(), std::back_inserter(urlsToCheck),
        [](const auto& groupMember) { return groupMember.second->Channel()->ClientIconPath(); });
  }

  const std::string owner =
      StringUtils::Format(CPVRChannel::IMAGE_OWNER_PATTERN, IsRadio() ? "radio" : "tv");
  return CPVRCachedImages::Cleanup({{owner, ""}}, urlsToCheck);
}

int CPVRChannelGroup::GetClientPriority() const
{
  if (GetClientID() == PVR_GROUP_CLIENT_ID_UNKNOWN)
    return 0; // not yet fully migrated; try later.

  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (!m_clientPriority.has_value())
  {
    const auto client = CServiceBroker::GetPVRManager().GetClient(GetClientID());
    if (client)
      m_clientPriority = client->GetPriority();
    else
      m_clientPriority = 0;
  }
  return *m_clientPriority;
}
