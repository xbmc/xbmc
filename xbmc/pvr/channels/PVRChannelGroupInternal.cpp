/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "settings/GUISettings.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
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
  CPVRChannelGroup(bRadio)
{
  m_iHiddenChannels = 0;
  m_iGroupId        = bRadio ? XBMC_INTERNAL_GROUP_RADIO : XBMC_INTERNAL_GROUP_TV;
  m_strGroupName    = g_localizeStrings.Get(bRadio ? 19216 : 19217);
}

CPVRChannelGroupInternal::CPVRChannelGroupInternal(const CPVRChannelGroup &group) :
    CPVRChannelGroup(group)
{
  m_iHiddenChannels = group.GetNumHiddenChannels();
}

CPVRChannelGroupInternal::~CPVRChannelGroupInternal(void)
{
  Unload();
}

int CPVRChannelGroupInternal::Load(void)
{
  int iChannelCount = CPVRChannelGroup::Load();
  UpdateChannelPaths();
  CreateChannelEpgs();

  return iChannelCount;
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
  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    PVRChannelGroupMember member = at(iChannelPtr);
    member.channel->UpdatePath(iChannelPtr);
  }
}

void CPVRChannelGroupInternal::Unload()
{
  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    delete at(iChannelPtr).channel;
  }

  CPVRChannelGroup::Unload();
}

bool CPVRChannelGroupInternal::UpdateFromClient(const CPVRChannel &channel)
{
  CSingleLock lock(m_critSection);
  CPVRChannel *realChannel = (CPVRChannel *) GetByClient(channel.UniqueID(), channel.ClientID());
  if (realChannel != NULL)
    realChannel->UpdateFromClient(channel);
  else
    realChannel = new CPVRChannel(channel);

  return CPVRChannelGroup::AddToGroup(*realChannel, 0, false);
}

bool CPVRChannelGroupInternal::InsertInGroup(CPVRChannel &channel, int iChannelNumber /* = 0 */, bool bSortAndRenumber /* = true */)
{
  CSingleLock lock(m_critSection);
  return CPVRChannelGroup::AddToGroup(channel, iChannelNumber, bSortAndRenumber);
}

bool CPVRChannelGroupInternal::Update(void)
{
  CPVRChannelGroupInternal PVRChannels_tmp(m_bRadio);
  PVRChannels_tmp.LoadFromClients();

  return UpdateGroupEntries(PVRChannels_tmp);
}

bool CPVRChannelGroupInternal::UpdateTimers(void)
{
  CSingleLock lock(m_critSection);

  /* update the timers with the new channel numbers */
  vector<CPVRTimerInfoTag *> timers;
  g_PVRTimers->GetActiveTimers(&timers);

  for (unsigned int ptr = 0; ptr < timers.size(); ptr++)
  {
    CPVRTimerInfoTag *timer = timers.at(ptr);
    const CPVRChannel *tag = GetByClient(timer->m_iClientChannelUid, timer->m_iClientId);
    if (tag)
      timer->m_channel = tag;
  }

  return true;
}

bool CPVRChannelGroupInternal::AddToGroup(CPVRChannel &channel, int iChannelNumber /* = 0 */, bool bSortAndRenumber /* = true */)
{
  CSingleLock lock(m_critSection);

  bool bReturn(false);

  /* get the actual channel since this is called from a fileitemlist copy */
  CPVRChannel *realChannel = (CPVRChannel *) GetByChannelID(channel.ChannelID());
  if (!realChannel)
    return bReturn;

  /* switch the hidden flag */
  if (realChannel->IsHidden())
  {
    realChannel->SetHidden(false, true);
    m_iHiddenChannels--;

    if (bSortAndRenumber)
      Renumber();
  }

  /* move this channel and persist */
  bReturn = (iChannelNumber > 0) ?
    MoveChannel(realChannel->ChannelNumber(), iChannelNumber, true) :
    MoveChannel(realChannel->ChannelNumber(), size() - m_iHiddenChannels, true);

  return bReturn;
}

bool CPVRChannelGroupInternal::RemoveFromGroup(const CPVRChannel &channel)
{
  CSingleLock lock(m_critSection);

  /* check if this channel is currently playing if we are hiding it */
  CPVRChannel currentChannel;
  if (g_PVRManager.GetCurrentChannel(currentChannel) && currentChannel == channel)
  {
    CGUIDialogOK::ShowAndGetInput(19098,19101,0,19102);
    return false;
  }

  /* get the actual channel since this is called from a fileitemlist copy */
  CPVRChannel *realChannel = (CPVRChannel *) GetByChannelID(channel.ChannelID());
  if (!realChannel)
    return false;

  /* switch the hidden flag */
  if (!realChannel->IsHidden())
  {
    realChannel->SetHidden(true, true);
    ++m_iHiddenChannels;
  }
  else
  {
    realChannel->SetHidden(false, true);
    --m_iHiddenChannels;
  }

  /* renumber this list */
  Renumber();

  /* and persist */
  return Persist();
}

bool CPVRChannelGroupInternal::MoveChannel(unsigned int iOldChannelNumber, unsigned int iNewChannelNumber, bool bSaveInDb /* = true */)
{
  CSingleLock lock(m_critSection);
  /* new channel number out of range */
  if (iNewChannelNumber > size() - m_iHiddenChannels)
    iNewChannelNumber = size() - m_iHiddenChannels;

  return CPVRChannelGroup::MoveChannel(iOldChannelNumber, iNewChannelNumber, bSaveInDb);
}

int CPVRChannelGroupInternal::GetMembers(CFileItemList &results, bool bGroupMembers /* = true */) const
{
  int iOrigSize = results.Size();
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    CPVRChannel *channel = at(iChannelPtr).channel;
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

  int iChannelCount = size();

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

  return size() - iChannelCount;
}

int CPVRChannelGroupInternal::LoadFromClients(void)
{
  int iCurSize = size();

  /* get the channels from the backends */
  PVR_ERROR error;
  g_PVRClients->GetChannels(this, &error);
  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGWARNING, "CPVRChannelGroupInternal - %s - got bad error (%d) on call to GetChannels", __FUNCTION__, error);

  return size() - iCurSize;
}

bool CPVRChannelGroupInternal::Renumber(void)
{
  CSingleLock lock(m_critSection);
  bool bReturn(CPVRChannelGroup::Renumber());

  m_iHiddenChannels = 0;
  for (unsigned int iChannelPtr = 0; iChannelPtr < size();  iChannelPtr++)
  {
    if (at(iChannelPtr).channel->IsHidden())
      m_iHiddenChannels++;
    else
      at(iChannelPtr).channel->UpdatePath(iChannelPtr);
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
  CPVRChannel *updateChannel = (CPVRChannel *) GetByUniqueID(channel.UniqueID());

  if (!updateChannel)
  {
    updateChannel = new CPVRChannel(channel.IsRadio());
    PVRChannelGroupMember newMember = { updateChannel, 0 };
    push_back(newMember);
    updateChannel->SetUniqueID(channel.UniqueID());
  }
  updateChannel->UpdateFromClient(channel);

  return updateChannel->Persist(!m_bLoaded);
}

bool CPVRChannelGroupInternal::AddAndUpdateChannels(const CPVRChannelGroup &channels, bool bUseBackendChannelNumbers)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  /* go through the channel list and check for updated or new channels */
  for (unsigned int iChannelPtr = 0; iChannelPtr < channels.size(); iChannelPtr++)
  {
    PVRChannelGroupMember member = channels.at(iChannelPtr);
    if (!member.channel)
      continue;

    /* check whether this channel is present in this container */
    CPVRChannel *existingChannel = (CPVRChannel *) GetByClient(member.channel->UniqueID(), member.channel->ClientID());
    if (existingChannel)
    {
      /* if it's present, update the current tag */
      if (existingChannel->UpdateFromClient(*member.channel))
      {
        existingChannel->Persist(!m_bLoaded);

        bReturn = true;
        CLog::Log(LOGINFO,"PVRChannelGroupInternal - %s - updated %s channel '%s'",
            __FUNCTION__, m_bRadio ? "radio" : "TV", member.channel->ChannelName().c_str());
      }
    }
    else
    {
      /* new channel */
      CPVRChannel *newChannel = new CPVRChannel(*member.channel);

      /* insert the new channel in this group */
      int iChannelNumber = bUseBackendChannelNumbers ? member.channel->ClientChannelNumber() : 0;
      InsertInGroup(*newChannel, iChannelNumber, false);

      bReturn = true;
      CLog::Log(LOGINFO,"PVRChannelGroupInternal - %s - added %s channel '%s' at position %d",
          __FUNCTION__, m_bRadio ? "radio" : "TV", member.channel->ChannelName().c_str(), iChannelNumber);
    }
  }

  return bReturn;
}

bool CPVRChannelGroupInternal::UpdateGroupEntries(const CPVRChannelGroup &channels)
{
  bool bReturn(false);

  if (CPVRChannelGroup::UpdateGroupEntries(channels))
  {
    /* try to find channel icons */
    SearchAndSetChannelIcons();
    Persist();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannelGroupInternal::Persist(void)
{
  bool bReturn(true);
  CSingleLock lock(m_critSection);

  bool bHasNewChannels = HasNewChannels();
  bool bHasChangedChannels = HasChangedChannels();

  /* open the database */
  CPVRDatabase *database = GetPVRDatabase();
  if (!database)
    return false;

  if (bHasNewChannels || bHasChangedChannels)
  CLog::Log(LOGDEBUG, "CPVRChannelGroupInternal - %s - persisting %d channels",
      __FUNCTION__, (int) size());

  if (bHasNewChannels)
  {
    CLog::Log(LOGDEBUG, "CPVRChannelGroupInternal - %s - group '%s' has new channels. writing changes directly",
        __FUNCTION__, GroupName().c_str());
    /* write directly to get channel ids */
    for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
    {
      CPVRChannel *channel = at(iChannelPtr).channel;
      if (!channel->Persist())
      {
        CLog::Log(LOGERROR, "CPVRChannelGroupInternal - %s - failed to persist channel '%s'",
            __FUNCTION__, channel->ChannelName().c_str());
        bReturn = false;
      }
    }

    lock.Leave();
  }
  else if (bHasChangedChannels)
  {
    /* queue queries */
    for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
      at(iChannelPtr).channel->Persist(true);

    lock.Leave();

    /* and commit them */
    bReturn = database->CommitInsertQueries();
    if (!bReturn)
      CLog::Log(LOGERROR, "CPVRChannelGroupInternal - %s - failed to persist channels", __FUNCTION__);
  }

  if (bReturn)
    bReturn = CPVRChannelGroup::Persist();

  return bReturn;
}

bool CPVRChannelGroupInternal::CreateChannelEpgs(bool bForce /* = false */)
{
  {
    CSingleLock lock(m_critSection);
    for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
    {
      CPVRChannel *channel = at(iChannelPtr).channel;
      if (!channel)
        continue;

      channel->CreateEPG(bForce);
    }
  }

  if (HasChangedChannels())
  {
    g_EpgContainer.PersistAll();
    return Persist();
  }

  return true;
}
