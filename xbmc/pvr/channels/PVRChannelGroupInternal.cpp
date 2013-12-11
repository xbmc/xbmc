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

#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

#include "PVRChannelGroupsContainer.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "epg/EpgContainer.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/addons/PVRClients.h"

using namespace PVR;
using namespace EPG;
using namespace std;

CPVRChannelGroupInternal::CPVRChannelGroupInternal(bool bRadio) :
  CPVRChannelGroup(bRadio, bRadio ? XBMC_INTERNAL_GROUP_RADIO : XBMC_INTERNAL_GROUP_TV, g_localizeStrings.Get(bRadio ? 19216 : 19217))
{
  m_iHiddenChannels = 0;
  m_iGroupType      = PVR_GROUP_TYPE_INTERNAL;
}

CPVRChannelGroupInternal::CPVRChannelGroupInternal(const CPVRChannelGroup &group) :
    CPVRChannelGroup(group)
{
  m_iHiddenChannels = group.GetNumHiddenChannels();
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
  CStdString strNewGroupName = g_localizeStrings.Get(m_bRadio ? 19216 : 19217);
  if (!m_strGroupName.Equals(strNewGroupName))
  {
    SetGroupName(strNewGroupName, true);
    UpdateChannelPaths();
  }
}

void CPVRChannelGroupInternal::UpdateChannelPaths(void)
{
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
  {
    PVRChannelGroupMember member = m_members.at(iChannelPtr);
    member.channel->UpdatePath(this, iChannelPtr);
  }
}

void CPVRChannelGroupInternal::UpdateFromClient(const CPVRChannel &channel, unsigned int iChannelNumber /* = 0 */)
{
  CSingleLock lock(m_critSection);
  CPVRChannelPtr realChannel = GetByClient(channel.UniqueID(), channel.ClientID());
  if (realChannel)
    realChannel->UpdateFromClient(channel);
  else
  {
    PVRChannelGroupMember newMember = { CPVRChannelPtr(new CPVRChannel(channel)), iChannelNumber > 0l ? iChannelNumber : (int)m_members.size() + 1 };
    m_members.push_back(newMember);
    m_bChanged = true;

    SortAndRenumber();
  }
}

bool CPVRChannelGroupInternal::InsertInGroup(CPVRChannel &channel, int iChannelNumber /* = 0 */)
{
  CSingleLock lock(m_critSection);
  return CPVRChannelGroup::AddToGroup(channel, iChannelNumber);
}

bool CPVRChannelGroupInternal::Update(void)
{
  CPVRChannelGroupInternal PVRChannels_tmp(m_bRadio);
  PVRChannels_tmp.SetPreventSortAndRenumber();
  return PVRChannels_tmp.LoadFromClients() && UpdateGroupEntries(PVRChannels_tmp);
}

bool CPVRChannelGroupInternal::AddToGroup(CPVRChannel &channel, int iChannelNumber /* = 0 */)
{
  CSingleLock lock(m_critSection);

  bool bReturn(false);

  /* get the actual channel since this is called from a fileitemlist copy */
  CPVRChannelPtr realChannel = GetByChannelID(channel.ChannelID());
  if (!realChannel)
    return bReturn;

  /* switch the hidden flag */
  if (realChannel->IsHidden())
  {
    realChannel->SetHidden(false);
    m_iHiddenChannels--;

    SortAndRenumber();
  }

  /* move this channel and persist */
  bReturn = (iChannelNumber > 0l) ?
    MoveChannel(realChannel->ChannelNumber(), iChannelNumber, true) :
    MoveChannel(realChannel->ChannelNumber(), m_members.size() - m_iHiddenChannels, true);

  if (m_bLoaded)
    realChannel->Persist();
  return bReturn;
}

bool CPVRChannelGroupInternal::RemoveFromGroup(const CPVRChannel &channel)
{
  CSingleLock lock(m_critSection);

  /* check if this channel is currently playing if we are hiding it */
  CPVRChannelPtr currentChannel;
  if (g_PVRManager.GetCurrentChannel(currentChannel) && *currentChannel == channel)
  {
    CGUIDialogOK::ShowAndGetInput(19098,19101,0,19102);
    return false;
  }

  /* get the actual channel since this is called from a fileitemlist copy */
  CPVRChannelPtr realChannel = GetByChannelID(channel.ChannelID());
  if (!realChannel)
    return false;

  /* switch the hidden flag */
  if (!realChannel->IsHidden())
  {
    realChannel->SetHidden(true);
    ++m_iHiddenChannels;
  }
  else
  {
    realChannel->SetHidden(false);
    --m_iHiddenChannels;
  }

  /* renumber this list */
  SortAndRenumber();

  /* and persist */
  return realChannel->Persist() &&
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

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
  {
    CPVRChannelPtr channel = m_members.at(iChannelPtr).channel;
    if (!channel)
      continue;

    if (bGroupMembers != channel->IsHidden())
    {
      CFileItemPtr pFileItem(new CFileItem(*channel));
      results.Add(pFileItem);
    }
  }

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

bool CPVRChannelGroupInternal::Renumber(void)
{
  CSingleLock lock(m_critSection);
  bool bReturn(CPVRChannelGroup::Renumber());

  m_iHiddenChannels = 0;
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size();  iChannelPtr++)
  {
    if (m_members.at(iChannelPtr).channel->IsHidden())
      m_iHiddenChannels++;
    else
      m_members.at(iChannelPtr).channel->UpdatePath(this, iChannelPtr);
  }

  return bReturn;
}

bool CPVRChannelGroupInternal::IsGroupMember(const CPVRChannel &channel) const
{
  return !channel.IsHidden();
}

bool CPVRChannelGroupInternal::UpdateChannel(const CPVRChannel &channel)
{
  CSingleLock lock(m_critSection);
  CPVRChannelPtr updateChannel = GetByUniqueID(channel.UniqueID());

  if (!updateChannel)
  {
    updateChannel = CPVRChannelPtr(new CPVRChannel(channel.IsRadio()));
    PVRChannelGroupMember newMember = { updateChannel, 0 };
    m_members.push_back(newMember);
    updateChannel->SetUniqueID(channel.UniqueID());
  }
  updateChannel->UpdateFromClient(channel);

  return updateChannel->Persist(!m_bLoaded);
}

bool CPVRChannelGroupInternal::AddAndUpdateChannels(const CPVRChannelGroup &channels, bool bUseBackendChannelNumbers)
{
  bool bReturn(false);
  SetPreventSortAndRenumber();

  CSingleLock lock(m_critSection);

  /* go through the channel list and check for updated or new channels */
  for (unsigned int iChannelPtr = 0; iChannelPtr < channels.m_members.size(); iChannelPtr++)
  {
    PVRChannelGroupMember member = channels.m_members.at(iChannelPtr);
    if (!member.channel)
      continue;

    /* check whether this channel is present in this container */
    CPVRChannelPtr existingChannel = GetByClient(member.channel->UniqueID(), member.channel->ClientID());
    if (existingChannel)
    {
      /* if it's present, update the current tag */
      if (existingChannel->UpdateFromClient(*member.channel))
      {
        bReturn = true;
        CLog::Log(LOGINFO,"PVRChannelGroupInternal - %s - updated %s channel '%s'", __FUNCTION__, m_bRadio ? "radio" : "TV", member.channel->ChannelName().c_str());
      }
    }
    else
    {
      /* new channel */
      UpdateFromClient(*member.channel, bUseBackendChannelNumbers ? member.channel->ClientChannelNumber() : 0);
      bReturn = true;
      CLog::Log(LOGINFO,"PVRChannelGroupInternal - %s - added %s channel '%s'", __FUNCTION__, m_bRadio ? "radio" : "TV", member.channel->ChannelName().c_str());
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
    for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
      CreateChannelEpg(m_members.at(iChannelPtr).channel);
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
