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
#include "messaging/helpers/DialogOKHelper.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/epg/EpgContainer.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <string>
#include <utility>
#include <vector>

using namespace PVR;
using namespace KODI::MESSAGING;

CPVRChannelGroupInternal::CPVRChannelGroupInternal(bool bRadio)
  : CPVRChannelGroup(CPVRChannelsPath(bRadio, g_localizeStrings.Get(19287)))
  , m_iHiddenChannels(0)
{
  m_iGroupType = PVR_GROUP_TYPE_INTERNAL;
}

CPVRChannelGroupInternal::~CPVRChannelGroupInternal()
{
  Unload();
  CServiceBroker::GetPVRManager().Events().Unsubscribe(this);
}

bool CPVRChannelGroupInternal::Load(std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove)
{
  if (CPVRChannelGroup::Load(channelsToRemove))
  {
    UpdateChannelPaths();
    CServiceBroker::GetPVRManager().Events().Subscribe(this, &CPVRChannelGroupInternal::OnPVRManagerEvent);
    return true;
  }

  CLog::LogF(LOGERROR, "Failed to load channels");
  return false;
}

void CPVRChannelGroupInternal::CheckGroupName()
{
  CSingleLock lock(m_critSection);

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
  CSingleLock lock(m_critSection);
  m_iHiddenChannels = 0;
  for (auto& groupMemberPair : m_members)
  {
    if (groupMemberPair.second->channel->IsHidden())
      ++m_iHiddenChannels;
    else
      groupMemberPair.second->channel->UpdatePath(GroupName());
  }
}

std::shared_ptr<CPVRChannel> CPVRChannelGroupInternal::UpdateFromClient(const std::shared_ptr<CPVRChannel>& channel, const CPVRChannelNumber& channelNumber, int iOrder, const CPVRChannelNumber& clientChannelNumber /* = {} */)
{
  CSingleLock lock(m_critSection);
  const std::shared_ptr<PVRChannelGroupMember>& realMember = GetByUniqueID(channel->StorageId());
  if (realMember->channel)
  {
    realMember->channel->UpdateFromClient(channel);
    return realMember->channel;
  }
  else
  {
    unsigned int iChannelNumber = channelNumber.GetChannelNumber();
    if (iChannelNumber == 0)
      iChannelNumber = static_cast<int>(m_sortedMembers.size()) + 1;

    channel->UpdatePath(GroupName());
    auto newMember = std::make_shared<PVRChannelGroupMember>(channel, CPVRChannelNumber(iChannelNumber, channelNumber.GetSubChannelNumber()), 0, iOrder, clientChannelNumber);
    m_sortedMembers.emplace_back(newMember);
    m_members.insert(std::make_pair(channel->StorageId(), newMember));
    m_bChanged = true;

    SortAndRenumber();
  }
  return channel;
}

bool CPVRChannelGroupInternal::Update(std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove)
{
  CPVRChannelGroupInternal PVRChannels_tmp(IsRadio());
  PVRChannels_tmp.SetPreventSortAndRenumber();
  PVRChannels_tmp.LoadFromClients();
  m_failedClients = PVRChannels_tmp.m_failedClients;
  return UpdateGroupEntries(PVRChannels_tmp, channelsToRemove);
}

bool CPVRChannelGroupInternal::AddToGroup(const std::shared_ptr<CPVRChannel>& channel, const CPVRChannelNumber& channelNumber, int iOrder, bool bUseBackendChannelNumbers, const CPVRChannelNumber& clientChannelNumber)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  /* get the group member, because we need the channel ID in this group, and the channel from this group */
  std::shared_ptr<PVRChannelGroupMember>& groupMember = GetByUniqueID(channel->StorageId());
  if (!groupMember->channel)
    return bReturn;

  bool bSort = false;

  /* switch the hidden flag */
  if (groupMember->channel->IsHidden())
  {
    groupMember->channel->SetHidden(false);
    if (m_iHiddenChannels > 0)
      m_iHiddenChannels--;

    bSort = true;
  }

  unsigned int iChannelNumber = channelNumber.GetChannelNumber();
  if (!channelNumber.IsValid() || iChannelNumber > (m_members.size() - m_iHiddenChannels))
    iChannelNumber = m_members.size() - m_iHiddenChannels;

  if (groupMember->channelNumber.GetChannelNumber() != iChannelNumber)
  {
    groupMember->channelNumber = CPVRChannelNumber(iChannelNumber, channelNumber.GetSubChannelNumber());
    bSort = true;
  }

  if (bSort)
    SortAndRenumber();

  if (m_bLoaded)
  {
    bReturn = Persist();
    groupMember->channel->Persist();
  }
  return bReturn;
}

bool CPVRChannelGroupInternal::AppendToGroup(const std::shared_ptr<CPVRChannel>& channel)
{
  CSingleLock lock(m_critSection);

  return AddToGroup(channel, CPVRChannelNumber(), 0, false);
}

bool CPVRChannelGroupInternal::RemoveFromGroup(const std::shared_ptr<CPVRChannel>& channel)
{
  if (!IsGroupMember(channel))
    return false;

  /* check if this channel is currently playing if we are hiding it */
  const std::shared_ptr<CPVRChannel> currentChannel = CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
  if (currentChannel && currentChannel == channel)
  {
    HELPERS::ShowOKDialogText(CVariant{19098}, CVariant{19102});
    return false;
  }

  CSingleLock lock(m_critSection);

  /* switch the hidden flag */
  if (!channel->IsHidden())
  {
    channel->SetHidden(true);
    ++m_iHiddenChannels;
  }
  else
  {
    channel->SetHidden(false);
    if (m_iHiddenChannels > 0)
      --m_iHiddenChannels;
  }

  /* renumber this list */
  SortAndRenumber();

  /* and persist */
  return channel->Persist() &&
      Persist();
}

int CPVRChannelGroupInternal::LoadFromDb(bool bCompress /* = false */)
{
  const std::shared_ptr<CPVRDatabase> database(CServiceBroker::GetPVRManager().GetTVDatabase());
  if (!database)
    return -1;

  int iChannelCount = Size();

  if (database->Get(*this, bCompress) == 0)
    CLog::LogFC(LOGDEBUG, LOGPVR, "No channels in the database");

  SortByChannelNumber();

  return Size() - iChannelCount;
}

bool CPVRChannelGroupInternal::LoadFromClients()
{
  /* get the channels from the backends */
  return CServiceBroker::GetPVRManager().Clients()->GetChannels(this, m_failedClients) ==
         PVR_ERROR_NO_ERROR;
}

bool CPVRChannelGroupInternal::IsGroupMember(const std::shared_ptr<CPVRChannel>& channel) const
{
  return !channel->IsHidden();
}

bool CPVRChannelGroupInternal::AddAndUpdateChannels(const CPVRChannelGroup& channels, bool bUseBackendChannelNumbers)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  /* go through the channel list and check for updated or new channels */
  for (auto& newMemberPair : channels.m_members)
  {
    /* check whether this channel is present in this container */
    std::shared_ptr<PVRChannelGroupMember>& existingMember = GetByUniqueID(newMemberPair.first);
    const std::shared_ptr<PVRChannelGroupMember>& newMember = newMemberPair.second;
    if (existingMember->channel)
    {
      /* if it's present, update the current tag */
      if (existingMember->channel->UpdateFromClient(newMember->channel))
      {
        bReturn = true;
        CLog::LogFC(LOGDEBUG, LOGPVR, "Updated {} channel '{}' from PVR client", IsRadio() ? "radio" : "TV", newMember->channel->ChannelName());
      }

      if ((existingMember->channelNumber != newMember->channelNumber && m_bSyncChannelGroups) ||
          existingMember->clientChannelNumber != newMember->clientChannelNumber ||
          existingMember->iOrder != newMember->iOrder)
      {
        if (m_bSyncChannelGroups)
          existingMember->channelNumber = newMember->channelNumber;
        existingMember->clientChannelNumber = newMember->clientChannelNumber;
        existingMember->iOrder = newMember->iOrder;
        bReturn = true;
      }
    }
    else
    {
      /* new channel */
      UpdateFromClient(newMember->channel, newMember->channelNumber, newMember->iOrder, newMember->clientChannelNumber);
      if (newMember->channel->CreateEPG())
      {
        CLog::LogFC(LOGDEBUG, LOGPVR, "Created EPG for {} channel '{}' from PVR client", IsRadio() ? "radio" : "TV", newMember->channel->ChannelName());
      }
      bReturn = true;
      CLog::LogFC(LOGDEBUG, LOGPVR, "Added {} channel '{}' from PVR client", IsRadio() ? "radio" : "TV", newMember->channel->ChannelName());
    }
  }

  if (m_bChanged)
    SortAndRenumber();

  return bReturn;
}

std::vector<std::shared_ptr<CPVRChannel>> CPVRChannelGroupInternal::RemoveDeletedChannels(const CPVRChannelGroup& channels)
{
  std::vector<std::shared_ptr<CPVRChannel>> removedChannels = CPVRChannelGroup::RemoveDeletedChannels(channels);
  if (!removedChannels.empty())
  {
    const std::shared_ptr<CPVRDatabase> database = CServiceBroker::GetPVRManager().GetTVDatabase();
    if (!database)
    {
      CLog::LogF(LOGERROR, "No TV database");
    }
    else
    {
      std::vector<std::shared_ptr<CPVREpg>> epgsToRemove;

      for (const auto& channel : removedChannels)
      {
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

      if (Size() == 0)
      {
        // Group is empty. Delete all group members from the database.
        commitPending = database->QueueDeleteChannelGroupMembersQuery(GroupID());
      }

      for (const auto& channel : removedChannels)
      {
        // since channel was not found in the internal group, it was deleted from the backend
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
  return removedChannels;
}

bool CPVRChannelGroupInternal::UpdateGroupEntries(const CPVRChannelGroup& channels, std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove)
{
  if (CPVRChannelGroup::UpdateGroupEntries(channels, channelsToRemove))
  {
    Persist();
    return true;
  }

  return false;
}

void CPVRChannelGroupInternal::CreateChannelEpg(const std::shared_ptr<CPVRChannel>& channel)
{
  if (channel)
    channel->CreateEPG();
}

bool CPVRChannelGroupInternal::CreateChannelEpgs(bool bForce /* = false */)
{
  if (!CServiceBroker::GetPVRManager().EpgContainer().IsStarted())
    return false;

  {
    CSingleLock lock(m_critSection);
    for (auto& groupMemberPair : m_members)
      CreateChannelEpg(groupMemberPair.second->channel);
  }

  if (HasChangedChannels())
  {
    return Persist();
  }

  return true;
}

void CPVRChannelGroupInternal::OnPVRManagerEvent(const PVR::PVREvent& event)
{
  if (event == PVREvent::ManagerStarted)
    CServiceBroker::GetPVRManager().TriggerEpgsCreate();
}
