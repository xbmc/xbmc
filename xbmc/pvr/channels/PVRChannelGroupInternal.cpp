/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupInternal.h"

#include <utility>

#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/timers/PVRTimers.h"

using namespace PVR;
using namespace KODI::MESSAGING;

CPVRChannelGroupInternal::CPVRChannelGroupInternal(bool bRadio) :
  m_iHiddenChannels(0)
{
  m_iGroupType = PVR_GROUP_TYPE_INTERNAL;
  m_bRadio = bRadio;
  m_strGroupName = g_localizeStrings.Get(19287);
}

CPVRChannelGroupInternal::CPVRChannelGroupInternal(const CPVRChannelGroup &group) :
    CPVRChannelGroup(group),
    m_iHiddenChannels(group.GetNumHiddenChannels())
{
}

CPVRChannelGroupInternal::~CPVRChannelGroupInternal(void)
{
  Unload();
  CServiceBroker::GetPVRManager().Events().Unsubscribe(this);
}

bool CPVRChannelGroupInternal::Load(void)
{
  if (CPVRChannelGroup::Load())
  {
    UpdateChannelPaths();
    CServiceBroker::GetPVRManager().Events().Subscribe(this, &CPVRChannelGroupInternal::OnPVRManagerEvent);
    return true;
  }

  CLog::LogF(LOGERROR, "Failed to load channels");
  return false;
}

void CPVRChannelGroupInternal::CheckGroupName(void)
{
  CSingleLock lock(m_critSection);

  /* check whether the group name is still correct, or channels will fail to load after the language setting changed */
  std::string strNewGroupName = g_localizeStrings.Get(19287);
  if (m_strGroupName != strNewGroupName)
  {
    SetGroupName(strNewGroupName, true);
    UpdateChannelPaths();
  }
}

void CPVRChannelGroupInternal::UpdateChannelPaths(void)
{
  CSingleLock lock(m_critSection);
  m_iHiddenChannels = 0;
  for (PVR_CHANNEL_GROUP_MEMBERS::iterator it = m_members.begin(); it != m_members.end(); ++it)
  {
    if (it->second.channel->IsHidden())
      ++m_iHiddenChannels;
    else
      it->second.channel->UpdatePath(this);
  }
}

CPVRChannelPtr CPVRChannelGroupInternal::UpdateFromClient(const CPVRChannelPtr &channel, const CPVRChannelNumber &channelNumber)
{
  CSingleLock lock(m_critSection);
  const PVRChannelGroupMember& realChannel(GetByUniqueID(channel->StorageId()));
  if (realChannel.channel)
  {
    realChannel.channel->UpdateFromClient(channel);
    return realChannel.channel;
  }
  else
  {
    unsigned int iChannelNumber = channelNumber.GetChannelNumber();
    if (iChannelNumber == 0)
      iChannelNumber = static_cast<int>(m_sortedMembers.size()) + 1;

    PVRChannelGroupMember newMember(channel, CPVRChannelNumber(iChannelNumber, channelNumber.GetSubChannelNumber()), 0);
    channel->UpdatePath(this);
    m_sortedMembers.push_back(newMember);
    m_members.insert(std::make_pair(channel->StorageId(), newMember));
    m_bChanged = true;

    SortAndRenumber();
  }
  return channel;
}

bool CPVRChannelGroupInternal::Update(void)
{
  CPVRChannelGroupInternal PVRChannels_tmp(m_bRadio);
  PVRChannels_tmp.SetPreventSortAndRenumber();
  PVRChannels_tmp.LoadFromClients();
  m_failedClientsForChannels = PVRChannels_tmp.m_failedClientsForChannels;
  return UpdateGroupEntries(PVRChannels_tmp);
}

bool CPVRChannelGroupInternal::AddToGroup(const CPVRChannelPtr &channel, const CPVRChannelNumber &channelNumber, bool bUseBackendChannelNumbers)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  /* get the group member, because we need the channel ID in this group, and the channel from this group */
  PVRChannelGroupMember& groupMember = GetByUniqueID(channel->StorageId());
  if (!groupMember.channel)
    return bReturn;

  bool bSort = false;

  /* switch the hidden flag */
  if (groupMember.channel->IsHidden())
  {
    groupMember.channel->SetHidden(false);
    if (m_iHiddenChannels > 0)
      m_iHiddenChannels--;

    bSort = true;
  }

  unsigned int iChannelNumber = channelNumber.GetChannelNumber();
  if (!channelNumber.IsValid() ||
      (!bUseBackendChannelNumbers && (iChannelNumber > m_members.size() - m_iHiddenChannels)))
    iChannelNumber = m_members.size() - m_iHiddenChannels;

  if (groupMember.channelNumber.GetChannelNumber() != iChannelNumber)
  {
    groupMember.channelNumber = CPVRChannelNumber(iChannelNumber, channelNumber.GetSubChannelNumber());
    bSort = true;
  }

  if (bSort)
    SortAndRenumber();

  if (m_bLoaded)
  {
    bReturn = Persist();
    groupMember.channel->Persist();
  }
  return bReturn;
}

bool CPVRChannelGroupInternal::RemoveFromGroup(const CPVRChannelPtr &channel)
{
  if (!IsGroupMember(channel))
    return false;

  /* check if this channel is currently playing if we are hiding it */
  CPVRChannelPtr currentChannel(CServiceBroker::GetPVRManager().GetPlayingChannel());
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
  const CPVRDatabasePtr database(CServiceBroker::GetPVRManager().GetTVDatabase());
  if (!database)
    return -1;

  int iChannelCount = Size();

  if (database->Get(*this, bCompress) == 0)
    CLog::LogFC(LOGDEBUG, LOGPVR, "No channels in the database");

  SortByChannelNumber();

  return Size() - iChannelCount;
}

bool CPVRChannelGroupInternal::LoadFromClients(void)
{
  /* get the channels from the backends */
  return CServiceBroker::GetPVRManager().Clients()->GetChannels(this, m_failedClientsForChannels) == PVR_ERROR_NO_ERROR;
}

bool CPVRChannelGroupInternal::IsGroupMember(const CPVRChannelPtr &channel) const
{
  return !channel->IsHidden();
}

bool CPVRChannelGroupInternal::AddAndUpdateChannels(const CPVRChannelGroup &channels, bool bUseBackendChannelNumbers)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  /* go through the channel list and check for updated or new channels */
  for (PVR_CHANNEL_GROUP_MEMBERS::const_iterator it = channels.m_members.begin(); it != channels.m_members.end(); ++it)
  {
    /* check whether this channel is present in this container */
    const PVRChannelGroupMember& existingChannel(GetByUniqueID(it->first));
    if (existingChannel.channel)
    {
      /* if it's present, update the current tag */
      if (existingChannel.channel->UpdateFromClient(it->second.channel))
      {
        bReturn = true;
        CLog::LogFC(LOGDEBUG, LOGPVR, "Updated %s channel '%s' from PVR client", m_bRadio ? "radio" : "TV", it->second.channel->ChannelName().c_str());
      }
    }
    else
    {
      /* new channel */
      UpdateFromClient(it->second.channel, bUseBackendChannelNumbers ? it->second.channel->ClientChannelNumber() : CPVRChannelNumber());
      bReturn = true;
      CLog::LogFC(LOGDEBUG, LOGPVR,"Added %s channel '%s' from PVR client", m_bRadio ? "radio" : "TV", it->second.channel->ChannelName().c_str());
    }
  }

  if (m_bChanged)
    SortAndRenumber();

  return bReturn;
}

std::vector<CPVRChannelPtr> CPVRChannelGroupInternal::RemoveDeletedChannels(const CPVRChannelGroup &channels)
{
  std::vector<CPVRChannelPtr> removedChannels = CPVRChannelGroup::RemoveDeletedChannels(channels);

  if (!removedChannels.empty())
  {
    CPVRChannelGroups* groups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bRadio);
    for (const auto& channel : removedChannels)
    {
      /* remove this channel from all non-system groups */
      groups->RemoveFromAllGroups(channel);

      /* do we have valid data from channel's client? */
      if (!IsMissingChannelsFromClient(channel->ClientID()))
      {
        /* since channel was not found in the internal group, it was deleted from the backend */
        channel->Delete();
      }
    }
  }

  return removedChannels;
}

bool CPVRChannelGroupInternal::UpdateGroupEntries(const CPVRChannelGroup &channels)
{
  bool bReturn(false);

  if (CPVRChannelGroup::UpdateGroupEntries(channels))
  {
    /* try to find channel icons */
    if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bPVRChannelIconsAutoScan)
      SearchAndSetChannelIcons();

    CServiceBroker::GetPVRManager().Timers()->UpdateChannels();
    Persist();

    bReturn = true;
  }

  return bReturn;
}

void CPVRChannelGroupInternal::CreateChannelEpg(const CPVRChannelPtr &channel, bool bForce /* = false */)
{
  if (channel)
    channel->CreateEPG(bForce);
}

bool CPVRChannelGroupInternal::CreateChannelEpgs(bool bForce /* = false */)
{
  if (!CServiceBroker::GetPVRManager().EpgContainer().IsStarted())
    return false;

  {
    CSingleLock lock(m_critSection);
    for (PVR_CHANNEL_GROUP_MEMBERS::iterator it = m_members.begin(); it != m_members.end(); ++it)
      CreateChannelEpg(it->second.channel);
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
