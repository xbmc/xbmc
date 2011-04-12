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

/**
 * TODO:
 * - use Observable here, so we can use event driven operations later
 */

#include "settings/GUISettings.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/log.h"
#include "threads/SingleLock.h"

#include "PVRChannelGroupsContainer.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/epg/PVREpgContainer.h"

CPVRChannelGroup::CPVRChannelGroup(bool bRadio, unsigned int iGroupId, const CStdString &strGroupName, int iSortOrder)
{
  m_bRadio       = bRadio;
  m_iGroupId     = iGroupId;
  m_strGroupName = strGroupName;
  m_iSortOrder   = iSortOrder;
  m_bLoaded      = false;
  m_bChanged     = false;
  clear();
}

CPVRChannelGroup::CPVRChannelGroup(bool bRadio)
{
  m_bRadio       = bRadio;
  m_iGroupId     = -1;
  m_strGroupName.clear();
  m_iSortOrder   = -1;
  m_bLoaded      = false;
  m_bChanged     = false;
  clear();
}

CPVRChannelGroup::CPVRChannelGroup(const PVR_CHANNEL_GROUP &group)
{
  m_bRadio       = group.bIsRadio;
  m_iGroupId     = -1;
  m_strGroupName = group.strGroupName;
  m_iSortOrder   = -1;
  m_bLoaded      = false;
  m_bChanged     = false;
  clear();
}

CPVRChannelGroup::~CPVRChannelGroup(void)
{
  Unload();
}

bool CPVRChannelGroup::operator==(const CPVRChannelGroup& right) const
{
  if (this == &right) return true;

  return (m_bRadio == right.m_bRadio && m_iGroupId == right.m_iGroupId);
}

bool CPVRChannelGroup::operator!=(const CPVRChannelGroup &right) const
{
  return !(*this == right);
}

int CPVRChannelGroup::Load(void)
{
  /* make sure this container is empty before loading */
  Unload();

  int iChannelCount = LoadFromDb();
  CLog::Log(LOGDEBUG, "PVRChannelGroup - %s - %d channels loaded from the database for group '%s'",
        __FUNCTION__, iChannelCount, m_strGroupName.c_str());

  Update();
  if (size() - iChannelCount > 0)
  {
    CLog::Log(LOGDEBUG, "PVRChannelGroup - %s - %d channels added from clients to group '%s'",
        __FUNCTION__, (int) size() - iChannelCount, m_strGroupName.c_str());
  }

  m_bLoaded = true;

  return size();
}

void CPVRChannelGroup::Unload(void)
{
  clear();
}

bool CPVRChannelGroup::Update(void)
{
  CPVRChannelGroup PVRChannels_tmp(m_bRadio, m_iGroupId, m_strGroupName, m_iSortOrder);
  PVRChannels_tmp.LoadFromClients();

  return UpdateGroupEntries(PVRChannels_tmp);
}

bool CPVRChannelGroup::Update(const CPVRChannelGroup &group)
{
  CSingleLock lock(m_critSection);
  if (!m_strGroupName.Equals(group.GroupName()) || m_iSortOrder != group.SortOrder())
  {
    m_bChanged = true;
    m_strGroupName = group.GroupName();
    m_iSortOrder   = group.SortOrder();
  }

  return true;
}

bool CPVRChannelGroup::MoveChannel(unsigned int iOldChannelNumber, unsigned int iNewChannelNumber, bool bSaveInDb /* = true */)
{
  if (iOldChannelNumber == iNewChannelNumber)
    return true;

  bool bReturn(false);
  CSingleLock lock(m_critSection);

  /* make sure the list is sorted by channel number */
  SortByChannelNumber();

  /* old channel number out of range */
  if (iOldChannelNumber > size())
    return bReturn;

  /* new channel number out of range */
  if (iNewChannelNumber > size())
    iNewChannelNumber = size();

  /* move the channel in the list */
  PVRChannelGroupMember entry = at(iOldChannelNumber - 1);
  erase(begin() + iOldChannelNumber - 1);
  insert(begin() + iNewChannelNumber - 1, entry);

  /* renumber the list */
  Renumber();

  m_bChanged = true;

  if (bSaveInDb)
    bReturn = Persist();
  else
    bReturn = true;

  CLog::Log(LOGNOTICE, "CPVRChannelGroup - %s - %s channel '%s' moved to channel number '%d'",
      __FUNCTION__, (m_bRadio ? "radio" : "tv"), entry.channel->ChannelName().c_str(), iNewChannelNumber);

  return true;
}

void CPVRChannelGroup::SearchAndSetChannelIcons(bool bUpdateDb /* = false */)
{
  if (g_guiSettings.GetString("pvrmenu.iconpath") == "")
    return;

  CPVRDatabase *database = CPVRManager::Get()->GetTVDatabase();
  database->Open();
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    PVRChannelGroupMember groupMember = at(ptr);

    /* skip if an icon is already set */
    if (groupMember.channel->IconPath() != "")
      continue;

    CStdString strBasePath = g_guiSettings.GetString("pvrmenu.iconpath");
    CStdString strChannelName = groupMember.channel->ClientChannelName();

    CStdString strIconPath = strBasePath + groupMember.channel->ClientChannelName();
    CStdString strIconPathLower = strBasePath + strChannelName.ToLower();
    CStdString strIconPathUid;
    strIconPathUid.Format("%s/%08d", strBasePath, groupMember.channel->UniqueID());

    groupMember.channel->SetIconPath(strIconPath      + ".tbn", bUpdateDb) ||
    groupMember.channel->SetIconPath(strIconPath      + ".jpg", bUpdateDb) ||
    groupMember.channel->SetIconPath(strIconPath      + ".png", bUpdateDb) ||

    groupMember.channel->SetIconPath(strIconPathLower + ".tbn", bUpdateDb) ||
    groupMember.channel->SetIconPath(strIconPathLower + ".jpg", bUpdateDb) ||
    groupMember.channel->SetIconPath(strIconPathLower + ".png", bUpdateDb) ||

    groupMember.channel->SetIconPath(strIconPathUid   + ".tbn", bUpdateDb) ||
    groupMember.channel->SetIconPath(strIconPathUid   + ".jpg", bUpdateDb) ||
    groupMember.channel->SetIconPath(strIconPathUid   + ".png", bUpdateDb);

    /* TODO: start channel icon scraper here if nothing was found */
  }

  database->Close();
}

/********** sort methods **********/

struct sortByClientChannelNumber
{
  bool operator()(const PVRChannelGroupMember &channel1, const PVRChannelGroupMember &channel2)
  {
    return channel1.channel->ClientChannelNumber() < channel2.channel->ClientChannelNumber();
  }
};

struct sortByChannelNumber
{
  bool operator()(const PVRChannelGroupMember &channel1, const PVRChannelGroupMember &channel2)
  {
    return channel1.iChannelNumber < channel2.iChannelNumber;
  }
};

void CPVRChannelGroup::SortByClientChannelNumber(void)
{
  CSingleLock lock(m_critSection);
  sort(begin(), end(), sortByClientChannelNumber());
}

void CPVRChannelGroup::SortByChannelNumber(void)
{
  CSingleLock lock(m_critSection);
  sort(begin(), end(), sortByChannelNumber());
}

/********** getters **********/

const CPVRChannel *CPVRChannelGroup::GetByClient(int iUniqueChannelId, int iClientID) const
{
  CPVRChannel *channel = NULL;
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    PVRChannelGroupMember groupMember = at(ptr);
    if (groupMember.channel->UniqueID() == iUniqueChannelId &&
        groupMember.channel->ClientID() == iClientID)
    {
      channel = groupMember.channel;
      break;
    }
  }

  return channel;
}

const CPVRChannel *CPVRChannelGroup::GetByChannelID(int iChannelID) const
{
  CPVRChannel *channel = NULL;
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    PVRChannelGroupMember groupMember = at(ptr);
    if (groupMember.channel->ChannelID() == iChannelID)
    {
      channel = groupMember.channel;
      break;
    }
  }

  return channel;
}

const CPVRChannel *CPVRChannelGroup::GetByUniqueID(int iUniqueID) const
{
  CPVRChannel *channel = NULL;
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    PVRChannelGroupMember groupMember = at(ptr);
    if (groupMember.channel->UniqueID() == iUniqueID)
    {
      channel = groupMember.channel;
      break;
    }
  }

  return channel;
}


unsigned int CPVRChannelGroup::GetChannelNumber(const CPVRChannel &channel) const
{
  unsigned int iReturn = 0;
  CSingleLock lock(m_critSection);
  unsigned int iSize = size();

  for (unsigned int iChannelPtr = 0; iChannelPtr < iSize; iChannelPtr++)
  {
    PVRChannelGroupMember member = at(iChannelPtr);
    if (member.channel->ChannelID() == channel.ChannelID())
    {
      iReturn = member.iChannelNumber;
      break;
    }
  }

  return iReturn;
}

const CPVRChannel *CPVRChannelGroup::GetByChannelNumber(unsigned int iChannelNumber) const
{
  CPVRChannel *channel = NULL;
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    PVRChannelGroupMember groupMember = at(ptr);
    if (groupMember.iChannelNumber == iChannelNumber)
    {
      channel = groupMember.channel;
      break;
    }
  }

  return channel;
}

const CPVRChannel *CPVRChannelGroup::GetByChannelUp(const CPVRChannel &channel) const
{
  CSingleLock lock(m_critSection);
  unsigned int iChannelNumber = GetChannelNumber(channel) + 1;
  if (iChannelNumber > size())
    iChannelNumber = 1;

  return GetByChannelNumber(iChannelNumber);
}

const CPVRChannel *CPVRChannelGroup::GetByChannelDown(const CPVRChannel &channel) const
{
  CSingleLock lock(m_critSection);

  int iChannelNumber = GetChannelNumber(channel) - 1;
  if (iChannelNumber <= 0)
    iChannelNumber = size();

  return GetByChannelNumber(iChannelNumber);
}

const CPVRChannel *CPVRChannelGroup::GetByIndex(unsigned int iIndex) const
{
  CSingleLock lock(m_critSection);
  return iIndex < size() ?
    at(iIndex).channel :
    NULL;
}

int CPVRChannelGroup::GetMembers(CFileItemList *results, bool bGroupMembers /* = true */) const
{
  int iOrigSize = results->Size();
  CSingleLock lock(m_critSection);

  const CPVRChannelGroup *channels = bGroupMembers ? this : CPVRManager::GetChannelGroups()->GetGroupAll(m_bRadio);
  for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
  {
    CPVRChannel *channel = channels->at(iChannelPtr).channel;
    if (!channel)
      continue;

    if (bGroupMembers || !IsGroupMember(channel))
    {
      CFileItemPtr pFileItem(new CFileItem(*channel));
      results->Add(pFileItem);
    }
  }

  return results->Size() - iOrigSize;
}

/********** private methods **********/

int CPVRChannelGroup::LoadFromDb(bool bCompress /* = false */)
{
  CPVRDatabase *database = CPVRManager::Get()->GetTVDatabase();
  if (!database || !database->Open())
    return -1;

  int iChannelCount = size();

  database->GetChannelsInGroup(this);
  database->Close();

  return size() - iChannelCount;
}

int CPVRChannelGroup::LoadFromClients(void)
{
  int iCurSize = size();

  /* get the channels from the backends */
  PVR_ERROR error;
  CPVRManager::GetClients()->GetChannelGroupMembers(this, &error);
  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGWARNING, "PVRChannelGroup - %s - got bad error (%d) on call to GetChannelGroupMembers", __FUNCTION__, error);

  return size() - iCurSize;
}

bool CPVRChannelGroup::RemoveByUniqueID(int iUniqueID)
{
  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    if (at(ptr).channel->UniqueID() == iUniqueID)
    {
      erase(begin() + ptr);
      m_bChanged = true;
      return true;
    }
  }

  return false;
}

bool CPVRChannelGroup::UpdateGroupEntries(const CPVRChannelGroup &channels)
{
  bool bChanged(false);
  CSingleLock lock(m_critSection);
  int iCurSize = size();

  CPVRDatabase *database = CPVRManager::Get()->GetTVDatabase();
  if (!database || !database->Open())
    return false;

  /* go through the channel list and check for updated or new channels */
  for (unsigned int iChannelPtr = 0; iChannelPtr < channels.size(); iChannelPtr++)
  {
    CPVRChannel *channel = channels.at(iChannelPtr).channel;
    int iChannelNumber   = channels.at(iChannelPtr).iChannelNumber;
    if (!channel)
      continue;

    CPVRChannel *realChannel = (CPVRChannel *) CPVRManager::GetChannelGroups()->GetGroupAll(m_bRadio)->GetByClient(channel->UniqueID(), channel->ClientID());
    if (!realChannel)
      continue;

    if (!IsGroupMember(realChannel))
    {
      AddToGroup(realChannel, iChannelNumber, false);

      bChanged = true;
      m_bChanged = true;
      CLog::Log(LOGINFO,"PVRChannelGroup - %s - added %s channel '%s' at position %d in group '%s'",
          __FUNCTION__, m_bRadio ? "radio" : "TV", realChannel->ChannelName().c_str(), iChannelNumber, GroupName().c_str());
    }
  }

  /* check for deleted channels */
  unsigned int iSize = size();
  for (unsigned int iChannelPtr = 0; iChannelPtr < iSize; iChannelPtr++)
  {
    CPVRChannel *channel = (CPVRChannel *) GetByIndex(iChannelPtr);
    if (!channel)
      continue;
    if (channels.GetByClient(channel->UniqueID(), channel->ClientID()) == NULL)
    {
      /* channel was not found */
      CLog::Log(LOGINFO,"PVRChannelGroup - %s - deleted %s channel '%s' from group '%s'",
          __FUNCTION__, m_bRadio ? "radio" : "TV", channel->ChannelName().c_str(), GroupName().c_str());

      /* remove this channel from all non-system groups */
      RemoveFromGroup(channel);

      m_bChanged = true;
      bChanged = true;
      iChannelPtr--;
      iSize--;
    }
  }

  if (bChanged)
  {
    /* sort by client channel number if this is the first time */
    if (iCurSize == 0)
      SortByClientChannelNumber();

    /* renumber to make sure all channels have a channel number.
       new channels were added at the back, so they'll get the highest numbers */
    Renumber();

    lock.Leave();

    CPVRManager::Get()->UpdateWindow(m_bRadio ? PVR_WINDOW_CHANNELS_RADIO : PVR_WINDOW_CHANNELS_TV);

    return Persist();
  }

  return true;
}

void CPVRChannelGroup::RemoveInvalidChannels(void)
{
  for (unsigned int ptr = 0; ptr < size(); ptr--)
  {
    CPVRChannel *channel = at(ptr).channel;
    if (channel->IsVirtual())
      continue;

    if (at(ptr).channel->ClientChannelNumber() <= 0)
    {
      CLog::Log(LOGERROR, "PVRChannelGroup - %s - removing invalid channel '%s' from client '%i': no valid client channel number",
          __FUNCTION__, channel->ChannelName().c_str(), channel->ClientID());
      erase(begin() + ptr);
      ptr--;
      m_bChanged = true;
      continue;
    }

    if (channel->UniqueID() <= 0)
    {
      CLog::Log(LOGERROR, "PVRChannelGroup - %s - removing invalid channel '%s' from client '%i': no valid unique ID",
          __FUNCTION__, channel->ChannelName().c_str(), channel->ClientID());
      erase(begin() + ptr);
      ptr--;
      m_bChanged = true;
      continue;
    }
  }
}

bool CPVRChannelGroup::RemoveFromGroup(CPVRChannel *channel)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    if (*channel == *at(iChannelPtr).channel)
    {
      // TODO notify observers
      erase(begin() + iChannelPtr);
      bReturn = true;
      m_bChanged = true;
      break;
    }
  }

  Renumber();

  return bReturn;
}

bool CPVRChannelGroup::AddToGroup(CPVRChannel *channel, int iChannelNumber /* = 0 */, bool bSortAndRenumber /* = true */)
{
  CSingleLock lock(m_critSection);

  bool bReturn(false);
  if (!channel)
    return bReturn;

  if (!CPVRChannelGroup::IsGroupMember(channel))
  {
    if (iChannelNumber <= 0 || iChannelNumber > (int) size() + 1)
      iChannelNumber = size() + 1;

    CPVRChannel *realChannel = (IsInternalGroup()) ?
        channel :
        (CPVRChannel *) CPVRManager::GetChannelGroups()->GetGroupAll(m_bRadio)->GetByChannelID(channel->ChannelID());

    if (realChannel)
    {
      PVRChannelGroupMember newMember = { realChannel, iChannelNumber };
      push_back(newMember);
      m_bChanged = true;

      if (bSortAndRenumber)
      {
        SortByChannelNumber();
        Renumber();
      }

      // TODO notify observers
      bReturn = true;
    }
  }

  return bReturn;
}

bool CPVRChannelGroup::IsGroupMember(const CPVRChannel *channel) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    if (*channel == *at(iChannelPtr).channel)
    {
      bReturn = true;
      break;
    }
  }

  return bReturn;
}

const CPVRChannel *CPVRChannelGroup::GetFirstChannel(void) const
{
  CPVRChannel *channel = NULL;
  CSingleLock lock(m_critSection);

  if (size() > 0)
    channel = at(0).channel;

  return channel;
}

bool CPVRChannelGroup::SetGroupName(const CStdString &strGroupName, bool bSaveInDb /* = false */)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  if (m_strGroupName != strGroupName)
  {
    /* update the name */
    m_strGroupName = strGroupName;
    m_bChanged = true;
//    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannelGroup::Persist(void)
{
  CSingleLock lock(m_critSection);
  if (!HasChanges())
    return true;

  CPVRDatabase *database = CPVRManager::Get()->GetTVDatabase();
  if (database && database->Open())
  {
    CLog::Log(LOGDEBUG, "CPVRChannelGroup - %s - persisting channel group '%s' with %d channels",
        __FUNCTION__, GroupName().c_str(), (int) size());
    database->Persist(this);
    database->Close();

    m_bChanged = false;
    return true;
  }

  return false;
}

void CPVRChannelGroup::Renumber(void)
{
  unsigned int iChannelNumber = 0;
  for (unsigned int ptr = 0; ptr < size();  ptr++)
  {
    if (at(ptr).iChannelNumber != iChannelNumber + 1)
      m_bChanged = true;

    at(ptr).iChannelNumber = ++iChannelNumber;
  }
}

bool CPVRChannelGroup::HasChangedChannels(void) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    if (at(iChannelPtr).channel->IsChanged())
    {
      bReturn = true;
      break;
    }
  }

  return bReturn;
}

bool CPVRChannelGroup::HasNewChannels(void) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    if (at(iChannelPtr).channel->ChannelID() <= 0)
    {
      bReturn = true;
      break;
    }
  }

  return bReturn;
}

bool CPVRChannelGroup::HasChanges(void) const
{
  CSingleLock lock(m_critSection);
  return m_bChanged || HasNewChannels() || HasChangedChannels();
}

void CPVRChannelGroup::CacheIcons(void)
{
  CSingleLock lock(m_critSection);
  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    at(iChannelPtr).channel->CacheIcon();
  }
}

void CPVRChannelGroup::ResetChannelNumbers(void)
{
  CSingleLock lock(m_critSection);
  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
    at(iChannelPtr).channel->SetCachedChannelNumber(0);
}

void CPVRChannelGroup::SetSelectedGroup(void)
{
  CSingleLock lock(m_critSection);

  /* reset all channel numbers */
  ((CPVRChannelGroup *) CPVRManager::GetChannelGroups()->GetGroupAll(m_bRadio))->ResetChannelNumbers();

  /* set all channel numbers on members of this group */
  unsigned int iChannelNumber(1);
  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
    at(iChannelPtr).channel->SetCachedChannelNumber(iChannelNumber++);
}
