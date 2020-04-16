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
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelsPath.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgChannelData.h"
#include "pvr/epg/EpgInfoTag.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace PVR;

CPVRChannelGroup::CPVRChannelGroup(const CPVRChannelsPath& path,
                                   int iGroupId /* = INVALID_GROUP_ID */,
                                   const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup /* = {} */)
  : m_iGroupId(iGroupId)
  , m_allChannelsGroup(allChannelsGroup)
  , m_path(path)
{
  OnInit();
}

CPVRChannelGroup::CPVRChannelGroup(const PVR_CHANNEL_GROUP& group,
                                   const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup)
  : m_iPosition(group.iPosition)
  , m_allChannelsGroup(allChannelsGroup)
  , m_path(group.bIsRadio, group.strGroupName)
{
  OnInit();
}

CPVRChannelGroup::~CPVRChannelGroup()
{
  CServiceBroker::GetSettingsComponent()->GetSettings()->UnregisterCallback(this);
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

std::shared_ptr<PVRChannelGroupMember> CPVRChannelGroup::EmptyMember = std::make_shared<PVRChannelGroupMember>();

void CPVRChannelGroup::OnInit()
{
  CServiceBroker::GetSettingsComponent()->GetSettings()->RegisterCallback(this, {
    CSettings::SETTING_PVRMANAGER_BACKENDCHANNELORDER,
    CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS,
    CSettings::SETTING_PVRMANAGER_STARTGROUPCHANNELNUMBERSFROMONE
  });
}

bool CPVRChannelGroup::Load(std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove)
{
  /* make sure this container is empty before loading */
  Unload();

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_bSyncChannelGroups = settings->GetBool(CSettings::SETTING_PVRMANAGER_SYNCCHANNELGROUPS);
  m_bUsingBackendChannelOrder = settings->GetBool(CSettings::SETTING_PVRMANAGER_BACKENDCHANNELORDER);
  m_bUsingBackendChannelNumbers = settings->GetBool(CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS) &&
                                  CServiceBroker::GetPVRManager().Clients()->EnabledClientAmount() == 1;
  m_bStartGroupChannelNumbersFromOne = settings->GetBool(CSettings::SETTING_PVRMANAGER_STARTGROUPCHANNELNUMBERSFROMONE) && !m_bUsingBackendChannelNumbers;

  int iChannelCount = m_iGroupId > 0 ? LoadFromDb() : 0;
  CLog::LogFC(LOGDEBUG, LOGPVR, "%d channels loaded from the database for group '%s'", iChannelCount, GroupName().c_str());

  if (!Update(channelsToRemove))
  {
    CLog::LogF(LOGERROR, "Failed to update channels for group '%s'", GroupName().c_str());
    return false;
  }

  if (Size() - iChannelCount > 0)
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "%d channels added from clients to group '%s'",
                static_cast<int>(Size() - iChannelCount), GroupName().c_str());
  }

  SortAndRenumber();

  m_bLoaded = true;

  return true;
}

void CPVRChannelGroup::Unload()
{
  CSingleLock lock(m_critSection);
  m_sortedMembers.clear();
  m_members.clear();
  m_failedClientsForChannels.clear();
  m_failedClientsForChannelGroupMembers.clear();
}

bool CPVRChannelGroup::Update(std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove)
{
  if (GroupType() == PVR_GROUP_TYPE_USER_DEFINED ||
      !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRMANAGER_SYNCCHANNELGROUPS))
    return true;

  CPVRChannelGroup PVRChannels_tmp(m_path, m_iGroupId, m_allChannelsGroup);
  PVRChannels_tmp.SetPreventSortAndRenumber();
  PVRChannels_tmp.LoadFromClients();
  m_failedClientsForChannelGroupMembers = PVRChannels_tmp.m_failedClientsForChannelGroupMembers;
  return UpdateGroupEntries(PVRChannels_tmp, channelsToRemove);
}

const CPVRChannelsPath& CPVRChannelGroup::GetPath() const
{
  CSingleLock lock(m_critSection);
  return m_path;
}

void CPVRChannelGroup::SetPath(const CPVRChannelsPath& path)
{
  CSingleLock lock(m_critSection);
  if (m_path != path)
  {
    m_path = path;
    m_bChanged = true;
    Persist();
  }
}

bool CPVRChannelGroup::SetChannelNumber(const std::shared_ptr<CPVRChannel>& channel, const CPVRChannelNumber& channelNumber)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (auto& member : m_sortedMembers)
  {
    if (*member->channel == *channel)
    {
      if (member->channelNumber != channelNumber)
      {
        m_bChanged = true;
        bReturn = true;
        member->channelNumber = channelNumber;
      }
      break;
    }
  }

  return bReturn;
}

/********** sort methods **********/

struct sortByClientChannelNumber
{
  bool operator()(const std::shared_ptr<PVRChannelGroupMember>& channel1, const std::shared_ptr<PVRChannelGroupMember>& channel2) const
  {
    if (channel1->iClientPriority == channel2->iClientPriority)
    {
      if (channel1->clientChannelNumber == channel2->clientChannelNumber)
        return channel1->channel->ChannelName() < channel2->channel->ChannelName();

      return channel1->clientChannelNumber < channel2->clientChannelNumber;
    }
    return channel1->iClientPriority > channel2->iClientPriority;
  }
};

struct sortByChannelNumber
{
  bool operator()(const std::shared_ptr<PVRChannelGroupMember>& channel1, const std::shared_ptr<PVRChannelGroupMember>& channel2) const
  {
    return channel1->channelNumber < channel2->channelNumber;
  }
};

void CPVRChannelGroup::Sort()
{
  if (m_bUsingBackendChannelOrder)
    SortByClientChannelNumber();
  else
    SortByChannelNumber();
}

bool CPVRChannelGroup::SortAndRenumber()
{
  if (PreventSortAndRenumber())
    return true;

  CSingleLock lock(m_critSection);
  Sort();

  bool bReturn = Renumber();
  return bReturn;
}

void CPVRChannelGroup::SortByClientChannelNumber()
{
  CSingleLock lock(m_critSection);
  if (!PreventSortAndRenumber())
    sort(m_sortedMembers.begin(), m_sortedMembers.end(), sortByClientChannelNumber());
}

void CPVRChannelGroup::SortByChannelNumber()
{
  CSingleLock lock(m_critSection);
  if (!PreventSortAndRenumber())
    sort(m_sortedMembers.begin(), m_sortedMembers.end(), sortByChannelNumber());
}

bool CPVRChannelGroup::UpdateClientPriorities()
{
  const std::shared_ptr<CPVRClients> clients = CServiceBroker::GetPVRManager().Clients();
  bool bChanged = false;

  CSingleLock lock(m_critSection);

  for (auto& member : m_sortedMembers)
  {
    int iNewPriority = 0;

    if (m_bUsingBackendChannelOrder)
    {
      std::shared_ptr<CPVRClient> client;
      if (!clients->GetCreatedClient(member->channel->ClientID(), client))
        continue;

      iNewPriority = client->GetPriority();
    }
    else
    {
      iNewPriority = 0;
    }

    bChanged |= (member->iClientPriority != iNewPriority);
    member->iClientPriority = iNewPriority;
  }

  return bChanged;
}

/********** getters **********/
std::shared_ptr<PVRChannelGroupMember>& CPVRChannelGroup::GetByUniqueID(const std::pair<int, int>& id)
{
  CSingleLock lock(m_critSection);
  const auto it = m_members.find(id);
  return it != m_members.end() ? it->second : CPVRChannelGroup::EmptyMember;
}

const std::shared_ptr<PVRChannelGroupMember>& CPVRChannelGroup::GetByUniqueID(const std::pair<int, int>& id) const
{
  CSingleLock lock(m_critSection);
  const auto it = m_members.find(id);
  return it != m_members.end() ? it->second : CPVRChannelGroup::EmptyMember;
}

std::shared_ptr<CPVRChannel> CPVRChannelGroup::GetByUniqueID(int iUniqueChannelId, int iClientID) const
{
  return GetByUniqueID(std::make_pair(iClientID, iUniqueChannelId))->channel;
}

std::shared_ptr<CPVRChannel> CPVRChannelGroup::GetByChannelID(int iChannelID) const
{
  CSingleLock lock(m_critSection);

  for (const auto& memberPair : m_members)
  {
    if (memberPair.second->channel->ChannelID() == iChannelID)
      return memberPair.second->channel;
  }

  return {};
}

std::shared_ptr<CPVRChannel> CPVRChannelGroup::GetByChannelEpgID(int iEpgID) const
{
  CSingleLock lock(m_critSection);

  for (const auto& memberPair : m_members)
  {
    if (memberPair.second->channel->EpgID() == iEpgID)
      return memberPair.second->channel;
  }

  return {};
}

std::shared_ptr<CPVRChannel> CPVRChannelGroup::GetLastPlayedChannel(int iCurrentChannel /* = -1 */) const
{
  CSingleLock lock(m_critSection);

  std::shared_ptr<CPVRChannel> returnChannel, channel;
  for (const auto& memberPair : m_members)
  {
    channel = memberPair.second->channel;
    if (channel->ChannelID() != iCurrentChannel &&
        CServiceBroker::GetPVRManager().Clients()->IsCreatedClient(channel->ClientID()) &&
        channel->LastWatched() > 0 &&
        (!returnChannel || channel->LastWatched() > returnChannel->LastWatched()))
    {
      returnChannel = channel;
    }
  }

  return returnChannel;
}

CPVRChannelNumber CPVRChannelGroup::GetChannelNumber(const std::shared_ptr<CPVRChannel>& channel) const
{
  CSingleLock lock(m_critSection);
  const std::shared_ptr<PVRChannelGroupMember>& member = GetByUniqueID(channel->StorageId());
  return member->channelNumber;
}

CPVRChannelNumber CPVRChannelGroup::GetClientChannelNumber(const std::shared_ptr<CPVRChannel>& channel) const
{
  CSingleLock lock(m_critSection);
  const std::shared_ptr<PVRChannelGroupMember>& member = GetByUniqueID(channel->StorageId());
  return member->clientChannelNumber;
}

std::shared_ptr<CPVRChannel> CPVRChannelGroup::GetByChannelNumber(const CPVRChannelNumber& channelNumber) const
{
  CSingleLock lock(m_critSection);

  for (const auto& member : m_sortedMembers)
  {
    CPVRChannelNumber activeChannelNumber = m_bUsingBackendChannelNumbers ? member->clientChannelNumber : member->channelNumber;
    if (activeChannelNumber == channelNumber)
      return member->channel;
  }

  return {};
}

std::shared_ptr<CPVRChannel> CPVRChannelGroup::GetNextChannel(const std::shared_ptr<CPVRChannel>& channel) const
{
  std::shared_ptr<CPVRChannel> nextChannel;

  if (channel)
  {
    CSingleLock lock(m_critSection);
    for (auto it = m_sortedMembers.cbegin(); !nextChannel && it != m_sortedMembers.cend(); ++it)
    {
      if ((*it)->channel == channel)
      {
        do
        {
          if ((++it) == m_sortedMembers.end())
            it = m_sortedMembers.begin();
          if ((*it)->channel && !(*it)->channel->IsHidden())
            nextChannel = (*it)->channel;
        } while (!nextChannel && (*it)->channel != channel);

        break;
      }
    }
  }

  return nextChannel;
}

std::shared_ptr<CPVRChannel> CPVRChannelGroup::GetPreviousChannel(const std::shared_ptr<CPVRChannel>& channel) const
{
  std::shared_ptr<CPVRChannel> previousChannel;

  if (channel)
  {
    CSingleLock lock(m_critSection);
    for (auto it = m_sortedMembers.rbegin(); !previousChannel && it != m_sortedMembers.rend(); ++it)
    {
      if ((*it)->channel == channel)
      {
        do
        {
          if ((++it) == m_sortedMembers.rend())
            it = m_sortedMembers.rbegin();
          if ((*it)->channel && !(*it)->channel->IsHidden())
            previousChannel = (*it)->channel;
        } while (!previousChannel && (*it)->channel != channel);

        break;
      }
    }
  }
  return previousChannel;
}

std::vector<std::shared_ptr<PVRChannelGroupMember>> CPVRChannelGroup::GetMembers(Include eFilter /* = Include::ALL */) const
{
  CSingleLock lock(m_critSection);
  if (eFilter == Include::ALL)
    return m_sortedMembers;

  std::vector<std::shared_ptr<PVRChannelGroupMember>> members;
  for (const auto& member : m_sortedMembers)
  {
    switch (eFilter)
    {
      case Include::ONLY_HIDDEN:
        if (!member->channel->IsHidden())
          continue;
        break;
      case Include::ONLY_VISIBLE:
        if (member->channel->IsHidden())
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
  CSingleLock lock(m_critSection);
  for (const auto& member : m_sortedMembers)
  {
    CPVRChannelNumber activeChannelNumber = m_bUsingBackendChannelNumbers ? member->clientChannelNumber : member->channelNumber;
    channelNumbers.emplace_back(activeChannelNumber.FormattedChannelNumber());
  }
}

int CPVRChannelGroup::LoadFromDb(bool bCompress /* = false */)
{
  const std::shared_ptr<CPVRDatabase> database(CServiceBroker::GetPVRManager().GetTVDatabase());
  if (!database)
    return -1;

  int iChannelCount = Size();

  database->Get(*this, *m_allChannelsGroup);

  return Size() - iChannelCount;
}

bool CPVRChannelGroup::LoadFromClients()
{
  /* get the channels from the backends */
  return CServiceBroker::GetPVRManager().Clients()->GetChannelGroupMembers(this, m_failedClientsForChannelGroupMembers) == PVR_ERROR_NO_ERROR;
}

bool CPVRChannelGroup::AddAndUpdateChannels(const CPVRChannelGroup& channels, bool bUseBackendChannelNumbers)
{
  bool bReturn(false);

  /* go through the channel list and check for new channels.
     channels will only by updated in CPVRChannelGroupInternal to prevent dupe updates */
  for (const auto& newMemberPair : channels.m_members)
  {
    /* check whether this channel is known in the internal group */
    const std::shared_ptr<PVRChannelGroupMember>& existingAllChannelsMember = m_allChannelsGroup->GetByUniqueID(newMemberPair.first);
    if (!existingAllChannelsMember->channel)
      continue;

    const std::shared_ptr<PVRChannelGroupMember>& newMember = newMemberPair.second;
    /* if it's found, add the channel to this group */
    if (!IsGroupMember(existingAllChannelsMember->channel))
    {
      AddToGroup(existingAllChannelsMember->channel,
                 newMember->channelNumber,
                 newMember->iOrder,
                 bUseBackendChannelNumbers, newMember->clientChannelNumber);

      bReturn = true;
      CLog::Log(LOGINFO, "Added %s channel '%s' to group '%s'",
                IsRadio() ? "radio" : "TV", existingAllChannelsMember->channel->ChannelName().c_str(), GroupName().c_str());
    }
    else
    {
      CSingleLock lock(m_critSection);
      std::shared_ptr<PVRChannelGroupMember>& existingMember = GetByUniqueID(newMemberPair.first);

      if ((existingMember->channelNumber != newMember->channelNumber && !m_bStartGroupChannelNumbersFromOne) ||
          existingMember->clientChannelNumber != newMember->clientChannelNumber ||
          existingMember->iOrder != newMember->iOrder)
      {
        existingMember->channelNumber = newMember->channelNumber;
        existingMember->clientChannelNumber = newMember->clientChannelNumber;
        existingMember->iOrder = newMember->iOrder;
        bReturn = true;
      }

      CLog::Log(LOGINFO, "Updated %s channel '%s' in group '%s'",
                IsRadio() ? "radio" : "TV", existingMember->channel->ChannelName().c_str(), GroupName().c_str());
    }
  }

  SortAndRenumber();

  return bReturn;
}

bool CPVRChannelGroup::IsMissingChannelsFromClient(int iClientId) const
{
  return std::find(m_failedClientsForChannels.begin(),
                   m_failedClientsForChannels.end(),
                   iClientId) != m_failedClientsForChannels.end();
}

bool CPVRChannelGroup::IsMissingChannelGroupMembersFromClient(int iClientId) const
{
  return std::find(m_failedClientsForChannelGroupMembers.begin(),
                   m_failedClientsForChannelGroupMembers.end(),
                   iClientId) != m_failedClientsForChannelGroupMembers.end();
}

void CPVRChannelGroup::UpdateClientOrder()
{
  CSingleLock lock(m_critSection);

  for (const auto& member : m_sortedMembers)
    member->channel->SetClientOrder(member->iOrder);
}

void CPVRChannelGroup::UpdateChannelNumbers()
{
  CSingleLock lock(m_critSection);

  for (const auto& member : m_sortedMembers)
  {
    member->channel->SetChannelNumber(m_bUsingBackendChannelNumbers ? member->clientChannelNumber : member->channelNumber);
    member->channel->SetClientChannelNumber(member->clientChannelNumber);
  }
}

bool CPVRChannelGroup::UpdateChannelNumbersFromAllChannelsGroup()
{
  CSingleLock lock(m_critSection);

  bool bChanged = false;

  if (!IsInternalGroup())
  {
    // If we don't sync channel groups make sure the channel numbers are set from
    // the all channels group using the non default renumber call before sorting
    if (Renumber(IGNORE_NUMBERING_FROM_ONE) || SortAndRenumber())
    {
      Persist();
      bChanged = true;
    }
  }

  m_events.Publish(IsInternalGroup() || bChanged ? PVREvent::ChannelGroupInvalidated
                                                 : PVREvent::ChannelGroup);

  return bChanged;
}

std::vector<std::shared_ptr<CPVRChannel>> CPVRChannelGroup::RemoveDeletedChannels(const CPVRChannelGroup& channels)
{
  std::vector<std::shared_ptr<CPVRChannel>> removedChannels;
  CSingleLock lock(m_critSection);

  /* check for deleted channels */
  for (std::vector<std::shared_ptr<PVRChannelGroupMember>>::iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end();)
  {
    const std::shared_ptr<CPVRChannel> channel = (*it)->channel;
    if (channels.m_members.find(channel->StorageId()) == channels.m_members.end())
    {
      /* channel was not found */
      CLog::Log(LOGINFO,"Deleted %s channel '%s' from group '%s'",
                IsRadio() ? "radio" : "TV", channel->ChannelName().c_str(), GroupName().c_str());

      removedChannels.emplace_back(channel);

      m_members.erase(channel->StorageId());
      it = m_sortedMembers.erase(it);
      m_bChanged = true;
    }
    else
    {
      ++it;
    }
  }

  return removedChannels;
}

bool CPVRChannelGroup::UpdateGroupEntries(const CPVRChannelGroup& channels, std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove)
{
  bool bReturn(false);
  bool bChanged(false);
  bool bRemoved(false);

  CSingleLock lock(m_critSection);
  /* sort by client channel number if this is the first time or if SETTING_PVRMANAGER_BACKENDCHANNELORDER is true */
  bool bUseBackendChannelNumbers(m_members.empty() || m_bUsingBackendChannelOrder);

  SetPreventSortAndRenumber(true);
  channelsToRemove = RemoveDeletedChannels(channels);
  bRemoved = !channelsToRemove.empty();
  bChanged = AddAndUpdateChannels(channels, bUseBackendChannelNumbers) || bRemoved;
  SetPreventSortAndRenumber(false);

  bChanged |= UpdateClientPriorities();

  if (bChanged)
  {
    /* renumber to make sure all channels have a channel number.
       new channels were added at the back, so they'll get the highest numbers */
    bool bRenumbered = SortAndRenumber();

    m_bChanged = true;
    bReturn = Persist();

    m_events.Publish(HasNewChannels() || bRemoved || bRenumbered ? PVREvent::ChannelGroupInvalidated : PVREvent::ChannelGroup);
  }
  else
  {
    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannelGroup::RemoveFromGroup(const std::shared_ptr<CPVRChannel>& channel)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (std::vector<std::shared_ptr<PVRChannelGroupMember>>::iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end();)
  {
    if (*channel == *((*it)->channel))
    {
      //! @todo notify observers
      m_members.erase((*it)->channel->StorageId());
      it = m_sortedMembers.erase(it);
      bReturn = true;
      m_bChanged = true;
      break;
    }
    else
    {
      ++it;
    }
  }

  // no need to renumber if nothing was removed
  if (bReturn)
    Renumber();

  return bReturn;
}

bool CPVRChannelGroup::AddToGroup(const std::shared_ptr<CPVRChannel>& channel, const CPVRChannelNumber& channelNumber, int iOrder, bool bUseBackendChannelNumbers, const CPVRChannelNumber& clientChannelNumber /* = {} */)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  if (!CPVRChannelGroup::IsGroupMember(channel))
  {
    const std::shared_ptr<PVRChannelGroupMember>& realMember = IsInternalGroup() ?
        GetByUniqueID(channel->StorageId()) :
        m_allChannelsGroup->GetByUniqueID(channel->StorageId());

    if (realMember->channel)
    {
      unsigned int iChannelNumber = channelNumber.GetChannelNumber();
      if (!channelNumber.IsValid())
        iChannelNumber = realMember->channelNumber.GetChannelNumber();

      CPVRChannelNumber clientChannelNumberToUse = clientChannelNumber;
      if (!clientChannelNumber.IsValid())
        clientChannelNumberToUse = realMember->clientChannelNumber;

      auto newMember = std::make_shared<PVRChannelGroupMember>(realMember->channel, CPVRChannelNumber(iChannelNumber, channelNumber.GetSubChannelNumber()), realMember->iClientPriority, iOrder, clientChannelNumberToUse);
      m_sortedMembers.emplace_back(newMember);
      m_members.insert(std::make_pair(realMember->channel->StorageId(), newMember));
      m_bChanged = true;

      SortAndRenumber();

      //! @todo notify observers
      bReturn = true;
    }
  }

  return bReturn;
}

bool CPVRChannelGroup::AppendToGroup(const std::shared_ptr<CPVRChannel>& channel)
{
  CSingleLock lock(m_critSection);

  unsigned int channelNumberMax = 0;
  for (const auto& member : m_sortedMembers)
  {
    if (member->channelNumber.GetChannelNumber() > channelNumberMax)
      channelNumberMax = member->channelNumber.GetChannelNumber();
  }

  return AddToGroup(channel, CPVRChannelNumber(channelNumberMax + 1, 0), 0, false);
}

bool CPVRChannelGroup::IsGroupMember(const std::shared_ptr<CPVRChannel>& channel) const
{
  CSingleLock lock(m_critSection);
  return m_members.find(channel->StorageId()) != m_members.end();
}

bool CPVRChannelGroup::IsGroupMember(int iChannelId) const
{
  CSingleLock lock(m_critSection);

  for (const auto& memberPair : m_members)
  {
    if (iChannelId == memberPair.second->channel->ChannelID())
      return true;
  }

  return false;
}

bool CPVRChannelGroup::Persist()
{
  bool bReturn(true);
  const std::shared_ptr<CPVRDatabase> database(CServiceBroker::GetPVRManager().GetTVDatabase());

  CSingleLock lock(m_critSection);

  /* only persist if the group has changes and is fully loaded or never has been saved before */
  if (!HasChanges() || (!m_bLoaded && m_iGroupId != INVALID_GROUP_ID))
    return bReturn;

  // Mark newly created groups as loaded so future updates will also be persisted...
  if (m_iGroupId == INVALID_GROUP_ID)
    m_bLoaded = true;

  if (database)
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Persisting channel group '%s' with %d channels",
                GroupName().c_str(), (int) m_members.size());
    m_bChanged = false;

    bReturn = database->Persist(*this);
  }
  else
  {
    bReturn = false;
  }

  return bReturn;
}

bool CPVRChannelGroup::Renumber(RenumberMode mode /* = NORMAL */)
{
  if (PreventSortAndRenumber())
    return true;

  bool bReturn(false);
  unsigned int iChannelNumber(0);
  bool bUsingBackendChannelNumbers(CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS) &&
                                 CServiceBroker::GetPVRManager().Clients()->EnabledClientAmount() == 1);
  bool bStartGroupChannelNumbersFromOne(CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRMANAGER_STARTGROUPCHANNELNUMBERSFROMONE) &&
                                        !bUsingBackendChannelNumbers);

  CSingleLock lock(m_critSection);

  CPVRChannelNumber currentChannelNumber;
  CPVRChannelNumber currentClientChannelNumber;
  for (auto& sortedMember : m_sortedMembers)
  {
    currentClientChannelNumber = sortedMember->clientChannelNumber;

    if (sortedMember->channel->IsHidden())
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
          currentChannelNumber = m_allChannelsGroup->GetChannelNumber(sortedMember->channel);

        if (!sortedMember->clientChannelNumber.IsValid())
          currentClientChannelNumber = m_allChannelsGroup->GetClientChannelNumber(sortedMember->channel);
      }
    }

    if (sortedMember->channelNumber != currentChannelNumber || sortedMember->clientChannelNumber != currentClientChannelNumber)
    {
      bReturn = true;
      m_bChanged = true;
      sortedMember->channelNumber = currentChannelNumber;
      sortedMember->clientChannelNumber = currentClientChannelNumber;

      auto& unsortedMember = GetByUniqueID(sortedMember->channel->StorageId());
      unsortedMember->channelNumber = sortedMember->channelNumber;
      unsortedMember->clientChannelNumber = sortedMember->clientChannelNumber;
    }
  }

  Sort();

  return bReturn;
}

bool CPVRChannelGroup::HasChangedChannels() const
{
  CSingleLock lock(m_critSection);

  for (const auto& memberPair : m_members)
  {
    if (memberPair.second->channel->IsChanged())
      return true;
  }

  return false;
}

bool CPVRChannelGroup::HasNewChannels() const
{
  CSingleLock lock(m_critSection);

  for (const auto& memberPair : m_members)
  {
    if (memberPair.second->channel->ChannelID() <= 0)
      return true;
  }

  return false;
}

bool CPVRChannelGroup::HasChanges() const
{
  CSingleLock lock(m_critSection);
  return m_bChanged || HasNewChannels() || HasChangedChannels();
}

void CPVRChannelGroup::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  //! @todo while pvr manager is starting up do accept setting changes.
  if(!CServiceBroker::GetPVRManager().IsStarted())
  {
    CLog::Log(LOGWARNING, "Channel group setting change ignored while PVR Manager is starting");
    return;
  }

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_PVRMANAGER_SYNCCHANNELGROUPS ||
      settingId == CSettings::SETTING_PVRMANAGER_BACKENDCHANNELORDER ||
      settingId == CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS ||
      settingId == CSettings::SETTING_PVRMANAGER_STARTGROUPCHANNELNUMBERSFROMONE)
  {
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    m_bSyncChannelGroups = settings->GetBool(CSettings::SETTING_PVRMANAGER_SYNCCHANNELGROUPS);
    bool bUsingBackendChannelOrder = settings->GetBool(CSettings::SETTING_PVRMANAGER_BACKENDCHANNELORDER);
    bool bUsingBackendChannelNumbers = settings->GetBool(CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS) &&
                                       CServiceBroker::GetPVRManager().Clients()->EnabledClientAmount() == 1;
    bool bStartGroupChannelNumbersFromOne = settings->GetBool(CSettings::SETTING_PVRMANAGER_STARTGROUPCHANNELNUMBERSFROMONE) && !bUsingBackendChannelNumbers;

    CSingleLock lock(m_critSection);

    bool bChannelNumbersChanged = m_bUsingBackendChannelNumbers != bUsingBackendChannelNumbers;
    bool bChannelOrderChanged = m_bUsingBackendChannelOrder != bUsingBackendChannelOrder;
    bool bGroupChannelNumbersFromOneChanged = m_bStartGroupChannelNumbersFromOne != bStartGroupChannelNumbersFromOne;

    m_bUsingBackendChannelOrder = bUsingBackendChannelOrder;
    m_bUsingBackendChannelNumbers = bUsingBackendChannelNumbers;
    m_bStartGroupChannelNumbersFromOne = bStartGroupChannelNumbersFromOne;

    /* check whether this channel group has to be renumbered */
    if (bChannelOrderChanged || bChannelNumbersChanged || bGroupChannelNumbersFromOneChanged)
    {
      CLog::LogFC(LOGDEBUG, LOGPVR, "Renumbering channel group '%s' to use the backend channel order and/or numbers",
                  GroupName().c_str());

      if (bChannelOrderChanged)
        UpdateClientPriorities();

      // If we don't sync channel groups make sure the channel numbers are set from
      // the all channels group using the non default renumber call before sorting
      if (!m_bSyncChannelGroups)
        Renumber(IGNORE_NUMBERING_FROM_ONE);

      bool bRenumbered = SortAndRenumber();
      Persist();

      if (m_bIsSelectedGroup)
      {
        for (const auto& member : m_sortedMembers)
        {
          member->channel->SetClientOrder(member->iOrder);
          member->channel->SetChannelNumber(m_bUsingBackendChannelNumbers ? member->clientChannelNumber : member->channelNumber);
          member->channel->SetClientChannelNumber(member->clientChannelNumber);
        }
      }

      m_events.Publish(bRenumbered ? PVREvent::ChannelGroupInvalidated : PVREvent::ChannelGroup);
    }
  }
}

CDateTime CPVRChannelGroup::GetEPGDate(EpgDateType epgDateType) const
{
  CDateTime date;
  std::shared_ptr<CPVREpg> epg;
  std::shared_ptr<CPVRChannel> channel;
  CSingleLock lock(m_critSection);

  for (const auto& memberPair : m_members)
  {
    channel = memberPair.second->channel;
    if (!channel->IsHidden() && (epg = channel->GetEPG()))
    {
      CDateTime epgDate;
      switch (epgDateType)
      {
        case EPG_FIRST_DATE:
          epgDate = epg->GetFirstDate();
          if (epgDate.IsValid() && (!date.IsValid() || epgDate < date))
            date = epgDate;
          break;

        case EPG_LAST_DATE:
          epgDate = epg->GetLastDate();
          if (epgDate.IsValid() && (!date.IsValid() || epgDate > date))
            date = epgDate;
          break;
      }
    }
  }

  return date;
}

CDateTime CPVRChannelGroup::GetFirstEPGDate() const
{
  return GetEPGDate(EPG_FIRST_DATE);
}

CDateTime CPVRChannelGroup::GetLastEPGDate() const
{
  return GetEPGDate(EPG_LAST_DATE);
}

int CPVRChannelGroup::GroupID() const
{
  return m_iGroupId;
}

void CPVRChannelGroup::SetGroupID(int iGroupId)
{
  if (iGroupId >= 0)
    m_iGroupId = iGroupId;
}

void CPVRChannelGroup::SetGroupType(int iGroupType)
{
  m_iGroupType = iGroupType;
}

int CPVRChannelGroup::GroupType() const
{
  return m_iGroupType;
}

std::string CPVRChannelGroup::GroupName() const
{
  CSingleLock lock(m_critSection);
  return m_path.GetGroupName();
}

void CPVRChannelGroup::SetGroupName(const std::string& strGroupName)
{
  CSingleLock lock(m_critSection);
  if (m_path.GetGroupName() != strGroupName)
  {
    m_path = CPVRChannelsPath(m_path.IsRadio(), strGroupName);
    m_bChanged = true;
    Persist();
  }
}

bool CPVRChannelGroup::IsRadio() const
{
  CSingleLock lock(m_critSection);
  return m_path.IsRadio();
}

time_t CPVRChannelGroup::LastWatched() const
{
  CSingleLock lock(m_critSection);
  return m_iLastWatched;
}

bool CPVRChannelGroup::SetLastWatched(time_t iLastWatched)
{
  const std::shared_ptr<CPVRDatabase> database(CServiceBroker::GetPVRManager().GetTVDatabase());

  CSingleLock lock(m_critSection);

  if (m_iLastWatched != iLastWatched)
  {
    m_iLastWatched = iLastWatched;

    /* update the database immediately */
    if (database)
      return database->UpdateLastWatched(*this);
  }

  return false;
}

bool CPVRChannelGroup::PreventSortAndRenumber() const
{
  CSingleLock lock(m_critSection);
  return m_bPreventSortAndRenumber;
}

void CPVRChannelGroup::SetPreventSortAndRenumber(bool bPreventSortAndRenumber /* = true */)
{
  CSingleLock lock(m_critSection);
  m_bPreventSortAndRenumber = bPreventSortAndRenumber;
}

bool CPVRChannelGroup::UpdateChannel(const std::pair<int, int>& storageId,
                                     const std::string& strChannelName,
                                     const std::string& strIconPath,
                                     int iEPGSource,
                                     int iChannelNumber,
                                     bool bHidden,
                                     bool bEPGEnabled,
                                     bool bParentalLocked,
                                     bool bUserSetIcon)
{
  CSingleLock lock(m_critSection);

  /* get the real channel from the group */
  const std::shared_ptr<PVRChannelGroupMember>& member = GetByUniqueID(storageId);
  if (!member->channel)
    return false;

  member->channel->SetChannelName(strChannelName, true);
  member->channel->SetHidden(bHidden);
  member->channel->SetLocked(bParentalLocked);
  member->channel->SetIconPath(strIconPath, bUserSetIcon);

  if (iEPGSource == 0)
    member->channel->SetEPGScraper("client");

  //! @todo add other scrapers
  member->channel->SetEPGEnabled(bEPGEnabled);

  /* set new values in the channel tag */
  if (bHidden)
  {
    // sort or previous changes will be overwritten
    Sort();

    RemoveFromGroup(member->channel);
  }
  else
  {
    SetChannelNumber(member->channel, CPVRChannelNumber(iChannelNumber, 0));
  }

  return true;
}

size_t CPVRChannelGroup::Size() const
{
  CSingleLock lock(m_critSection);
  return m_members.size();
}

bool CPVRChannelGroup::HasChannels() const
{
  CSingleLock lock(m_critSection);
  return !m_members.empty();
}

bool CPVRChannelGroup::CreateChannelEpgs(bool bForce /* = false */)
{
  /* used only by internal channel groups */
  return true;
}

void CPVRChannelGroup::SetHidden(bool bHidden)
{
  CSingleLock lock(m_critSection);

  if (m_bHidden != bHidden)
  {
    m_bHidden = bHidden;
    m_bChanged = true;
  }
}

bool CPVRChannelGroup::IsHidden() const
{
  CSingleLock lock(m_critSection);
  return m_bHidden;
}

int CPVRChannelGroup::GetPosition() const
{
  CSingleLock lock(m_critSection);
  return m_iPosition;
}

void CPVRChannelGroup::SetPosition(int iPosition)
{
  CSingleLock lock(m_critSection);

  if (m_iPosition != iPosition)
  {
    m_iPosition = iPosition;
    m_bChanged = true;
  }
}
