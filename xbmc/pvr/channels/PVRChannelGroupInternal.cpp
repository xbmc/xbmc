/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PVRChannelGroupInternal.h"

#include "dialogs/GUIDialogOK.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "epg/EpgContainer.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/addons/PVRClients.h"

#include <assert.h>

using namespace PVR;
using namespace EPG;

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
  g_PVRManager.UnregisterObserver(this);
}

bool CPVRChannelGroupInternal::Load(void)
{
  if (CPVRChannelGroup::Load())
  {
    UpdateChannelPaths();
    g_PVRManager.RegisterObserver(this);

    return true;
  }

  CLog::Log(LOGERROR, "PVRChannelGroupInternal - %s - failed to load channels", __FUNCTION__);
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
      m_iHiddenChannels++;
    else
      it->second.channel->UpdatePath(this);
  }
}

CPVRChannelPtr CPVRChannelGroupInternal::UpdateFromClient(const CPVRChannelPtr &channel, unsigned int iChannelNumber /* = 0 */)
{
  assert(channel.get());

  CSingleLock lock(m_critSection);
  const PVRChannelGroupMember& realChannel(GetByUniqueID(channel->StorageId()));
  if (realChannel.channel)
  {
    realChannel.channel->UpdateFromClient(channel);
    return realChannel.channel;
  }
  else
  {
    PVRChannelGroupMember newMember = { channel, iChannelNumber > 0l ? iChannelNumber : (int)m_sortedMembers.size() + 1 };
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
  return PVRChannels_tmp.LoadFromClients() && UpdateGroupEntries(PVRChannels_tmp);
}

bool CPVRChannelGroupInternal::AddToGroup(const CPVRChannelPtr &channel, int iChannelNumber /* = 0 */)
{
  CSingleLock lock(m_critSection);

  bool bReturn(false);

  /* get the group member, because we need the channel ID in this group, and the channel from this group */
  const PVRChannelGroupMember& groupMember(GetByUniqueID(channel->StorageId()));
  if (!groupMember.channel)
    return bReturn;

  /* switch the hidden flag */
  if (groupMember.channel->IsHidden())
  {
    groupMember.channel->SetHidden(false);
    if (m_iHiddenChannels > 0)
      m_iHiddenChannels--;

    SortAndRenumber();
  }

  /* move this channel and persist */
  bReturn = (iChannelNumber > 0l) ?
    MoveChannel(groupMember.iChannelNumber, iChannelNumber, true) :
    MoveChannel(groupMember.iChannelNumber, m_members.size() - m_iHiddenChannels, true);

  if (m_bLoaded)
    groupMember.channel->Persist();
  return bReturn;
}

bool CPVRChannelGroupInternal::RemoveFromGroup(const CPVRChannelPtr &channel)
{
  CSingleLock lock(m_critSection);
  assert(channel.get());

  if (!IsGroupMember(channel))
    return false;

  /* check if this channel is currently playing if we are hiding it */
  CPVRChannelPtr currentChannel(g_PVRManager.GetCurrentChannel());
  if (currentChannel && currentChannel == channel)
  {
    CGUIDialogOK::ShowAndGetInput(19098, 19102);
    return false;
  }

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

bool CPVRChannelGroupInternal::MoveChannel(unsigned int iOldChannelNumber, unsigned int iNewChannelNumber, bool bSaveInDb /* = true */)
{
  CSingleLock lock(m_critSection);
  /* new channel number out of range */
  if (iNewChannelNumber > m_members.size() - m_iHiddenChannels)
    iNewChannelNumber = m_members.size() - m_iHiddenChannels;

  return CPVRChannelGroup::MoveChannel(iOldChannelNumber, iNewChannelNumber, bSaveInDb);
}

int CPVRChannelGroupInternal::GetMembers(CFileItemList &results, bool bGroupMembers /* = true */) const
{
  int iOrigSize = results.Size();
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::const_iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end(); ++it)
    if (bGroupMembers != (*it).channel->IsHidden())
      results.Add(CFileItemPtr(new CFileItem((*it).channel)));

  return results.Size() - iOrigSize;
}

int CPVRChannelGroupInternal::LoadFromDb(bool bCompress /* = false */)
{
  CPVRDatabase *database = GetPVRDatabase();
  if (!database)
    return -1;

  int iChannelCount = Size();

  if (database->Get(*this) > 0)
  {
    if (bCompress)
      database->Compress(true);
  }
  else
  {
    CLog::Log(LOGINFO, "PVRChannelGroupInternal - %s - no channels in the database",
        __FUNCTION__);
  }

  SortByChannelNumber();

  return Size() - iChannelCount;
}

bool CPVRChannelGroupInternal::LoadFromClients(void)
{
  /* get the channels from the backends */
  return g_PVRClients->GetChannels(this) == PVR_ERROR_NO_ERROR;
}

bool CPVRChannelGroupInternal::IsGroupMember(const CPVRChannelPtr &channel) const
{
  return !channel->IsHidden();
}

bool CPVRChannelGroupInternal::AddAndUpdateChannels(const CPVRChannelGroup &channels, bool bUseBackendChannelNumbers)
{
  bool bReturn(false);
  SetPreventSortAndRenumber();

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
        CLog::Log(LOGINFO,"PVRChannelGroupInternal - %s - updated %s channel '%s'", __FUNCTION__, m_bRadio ? "radio" : "TV", it->second.channel->ChannelName().c_str());
      }
    }
    else
    {
      /* new channel */
      UpdateFromClient(it->second.channel, bUseBackendChannelNumbers ? it->second.channel->ClientChannelNumber() : 0);
      bReturn = true;
      CLog::Log(LOGINFO,"PVRChannelGroupInternal - %s - added %s channel '%s'", __FUNCTION__, m_bRadio ? "radio" : "TV", it->second.channel->ChannelName().c_str());
    }
  }

  SetPreventSortAndRenumber(false);
  if (m_bChanged)
    SortAndRenumber();

  return bReturn;
}

bool CPVRChannelGroupInternal::UpdateGroupEntries(const CPVRChannelGroup &channels)
{
  bool bReturn(false);

  if (CPVRChannelGroup::UpdateGroupEntries(channels))
  {
    /* try to find channel icons */
    if (g_advancedSettings.m_bPVRChannelIconsAutoScan)
      SearchAndSetChannelIcons();

    g_PVRTimers->UpdateChannels();
    Persist();

    bReturn = true;
  }

  return bReturn;
}

void CPVRChannelGroupInternal::CreateChannelEpg(CPVRChannelPtr channel, bool bForce /* = false */)
{
  if (!channel)
    return;

  CSingleLock lock(channel->m_critSection);
  if (!channel->m_bEPGCreated || bForce)
  {
    CEpg *epg = g_EpgContainer.CreateChannelEpg(channel);
    if (epg)
    {
      channel->m_bEPGCreated = true;
      if (epg->EpgID() != channel->m_iEpgId)
      {
        channel->m_iEpgId = epg->EpgID();
        channel->m_bChanged = true;
      }
    }
  }
}

bool CPVRChannelGroupInternal::CreateChannelEpgs(bool bForce /* = false */)
{
  if (!g_EpgContainer.IsStarted())
    return false;
  {
    CSingleLock lock(m_critSection);
    for (PVR_CHANNEL_GROUP_MEMBERS::iterator it = m_members.begin(); it != m_members.end(); ++it)
      CreateChannelEpg(it->second.channel);
  }

  if (HasChangedChannels())
  {
    g_EpgContainer.PersistTables();
    return Persist();
  }

  return true;
}

void CPVRChannelGroupInternal::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageManagerStateChanged)
  {
    g_PVRManager.TriggerEpgsCreate();
  }
}
