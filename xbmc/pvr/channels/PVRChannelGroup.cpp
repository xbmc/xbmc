/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "PVRChannelGroupsContainer.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/epg/PVREpgInfoTag.h"

using namespace MUSIC_INFO;

CPVRChannelGroup::CPVRChannelGroup(bool bRadio, unsigned int iGroupId, const CStdString &strGroupName, int iSortOrder)
{
  m_bRadio          = bRadio;
  m_iGroupId        = iGroupId;
  m_strGroupName    = strGroupName;
  m_iSortOrder      = iSortOrder;
  m_bInhibitSorting = false;
  clear();
}

CPVRChannelGroup::CPVRChannelGroup(bool bRadio)
{
  m_bRadio          = bRadio;
  m_iGroupId        = -1;
  m_strGroupName.clear();
  m_iSortOrder      = -1;
  m_bInhibitSorting = false;
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

  int iReturn = -1;

  CPVRDatabase *database = CPVRManager::Get()->GetTVDatabase();
  if (database->Open())
  {
    m_bInhibitSorting = true;
    iReturn = database->GetChannelsInGroup(this);
    m_bInhibitSorting = false;

    database->Close();
  }

  return iReturn;
}

void CPVRChannelGroup::Unload()
{
  clear();
}

bool CPVRChannelGroup::Update()
{
  return (Load() >= 0);
}

bool CPVRChannelGroup::Update(const CPVRChannelGroup &group)
{
  m_strGroupName = group.GroupName();
  m_iSortOrder   = group.SortOrder();

  return true;
}

bool CPVRChannelGroup::MoveChannel(unsigned int iOldChannelNumber, unsigned int iNewChannelNumber, bool bSaveInDb /* = true */)
{
  if (iOldChannelNumber == iNewChannelNumber)
    return true;

  bool bReturn = false;

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
  bool operator()(PVRChannelGroupMember channel1, PVRChannelGroupMember channel2)
  {
    return channel1.channel->ClientChannelNumber() > 0 && channel1.channel->ClientChannelNumber() < channel2.channel->ClientChannelNumber();
  }
};

struct sortByChannelNumber
{
  bool operator()(PVRChannelGroupMember channel1, PVRChannelGroupMember channel2)
  {
    return channel1.iChannelNumber > 0 && channel1.iChannelNumber < channel2.iChannelNumber;
  }
};

void CPVRChannelGroup::SortByClientChannelNumber(void)
{
  if (m_bInhibitSorting)
    return;

  sort(begin(), end(), sortByClientChannelNumber());
}

void CPVRChannelGroup::SortByChannelNumber(void)
{
  if (m_bInhibitSorting)
    return;

  sort(begin(), end(), sortByChannelNumber());
}

/********** getters **********/

const CPVRChannel *CPVRChannelGroup::GetByClient(int iClientChannelNumber, int iClientID) const
{
  CPVRChannel *channel = NULL;

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    PVRChannelGroupMember groupMember = at(ptr);
    if (groupMember.channel->ClientChannelNumber() == iClientChannelNumber &&
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


unsigned int CPVRChannelGroup::GetChannelNumber(const CPVRChannel *channel) const
{
  unsigned int iReturn = 0;
  unsigned int iSize = size();

  for (unsigned int iChannelPtr = 0; iChannelPtr < iSize; iChannelPtr++)
  {
    PVRChannelGroupMember member = at(iChannelPtr);
    if (*member.channel == *channel)
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

const CPVRChannel *CPVRChannelGroup::GetByChannelUp(const CPVRChannel *channel) const
{
  unsigned int iChannelNumber = GetChannelNumber(channel) + 1;
  if (iChannelNumber > size())
    iChannelNumber = 1;

  return GetByChannelNumber(iChannelNumber);
}

const CPVRChannel *CPVRChannelGroup::GetByChannelDown(const CPVRChannel *channel) const
{
  int iChannelNumber = GetChannelNumber(channel) - 1;
  if (iChannelNumber <= 0)
    iChannelNumber = size();

  return GetByChannelNumber(iChannelNumber);
}

const CPVRChannel *CPVRChannelGroup::GetByIndex(unsigned int iIndex) const
{
  return iIndex < size() ?
    at(iIndex).channel :
    NULL;
}

int CPVRChannelGroup::GetMembers(CFileItemList *results, bool bGroupMembers /* = true */) const
{
  int iOrigSize = results->Size();

  const CPVRChannelGroup *channels = bGroupMembers ? this : CPVRManager::GetChannelGroups()->GetGroupAll(m_bRadio);
  for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
  {
    CPVRChannel *channel = channels->at(iChannelPtr).channel;
    if (bGroupMembers || !IsGroupMember(channel))
    {
      CFileItemPtr pFileItem(new CFileItem(*channel));
      results->Add(pFileItem);
    }
  }

  return results->Size() - iOrigSize;
}

int CPVRChannelGroup::GetHiddenChannels(CFileItemList* results) const
{
  return GetMembers(results, false);
}

/********** private methods **********/

int CPVRChannelGroup::LoadFromDb(bool bCompress /* = false */)
{
  // TODO load group members
  return -1;
}

int CPVRChannelGroup::LoadFromClients(bool bAddToDb /* = true */)
{
  // TODO add support to load channel groups from clients
  return -1;
}

int CPVRChannelGroup::GetFromClients(void)
{
  return -1;
}

bool CPVRChannelGroup::RemoveByUniqueID(int iUniqueID)
{
  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    if (at(ptr).channel->UniqueID() == iUniqueID)
    {
      erase(begin() + ptr);
      return true;
    }
  }

  return false;
}

bool CPVRChannelGroup::UpdateGroupEntries(CPVRChannelGroup *channels)
{
  // TODO
  return false;
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
      continue;
    }

    if (channel->UniqueID() <= 0)
    {
      CLog::Log(LOGERROR, "PVRChannelGroup - %s - removing invalid channel '%s' from client '%i': no valid unique ID",
          __FUNCTION__, channel->ChannelName().c_str(), channel->ClientID());
      erase(begin() + ptr);
      ptr--;
      continue;
    }
  }
}

bool CPVRChannelGroup::RemoveFromGroup(CPVRChannel *channel)
{
  bool bReturn = false;

  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    if (*channel == *at(iChannelPtr).channel)
    {
      // TODO notify observers
      erase(begin() + iChannelPtr);
      bReturn = true;
      break;
    }
  }

  return bReturn;
}

bool CPVRChannelGroup::AddToGroup(CPVRChannel *channel, int iChannelNumber /* = 0 */)
{
  bool bReturn = false;

  if (!IsGroupMember(channel))
  {
    if (iChannelNumber <= 0)
      iChannelNumber = size() + 1;

    PVRChannelGroupMember newMember = { channel, iChannelNumber };
    // TODO notify observers
    push_back(newMember);
    SortByChannelNumber();
    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannelGroup::IsGroupMember(const CPVRChannel *channel) const
{
  bool bReturn = false;

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

  if (size() > 0)
    channel = at(0).channel;

  return channel;
}

bool CPVRChannelGroup::SetGroupName(const CStdString &strGroupName, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_strGroupName != strGroupName)
  {
    /* update the name */
    m_strGroupName = strGroupName;
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
  CPVRDatabase *database = CPVRManager::Get()->GetTVDatabase();
  if (database)
  {
    database->Open();
    database->Persist(this);
    database->Close();

    return true;
  }

  return false;
}

void CPVRChannelGroup::Renumber(void)
{
  int iChannelNumber = 0;
  for (unsigned int ptr = 0; ptr < size();  ptr++)
  {
    at(ptr).iChannelNumber = ++iChannelNumber;
  }
}
