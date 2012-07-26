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
#include "utils/StringUtils.h"
#include "threads/SingleLock.h"

#include "PVRChannelGroupsContainer.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "epg/EpgContainer.h"

using namespace PVR;
using namespace EPG;

CPVRChannelGroup::CPVRChannelGroup(void) :
    m_bRadio(false),
    m_iGroupType(PVR_GROUP_TYPE_DEFAULT),
    m_iGroupId(-1),
    m_bLoaded(false),
    m_bChanged(false),
    m_bUsingBackendChannelOrder(false)
{
}

CPVRChannelGroup::CPVRChannelGroup(bool bRadio, unsigned int iGroupId, const CStdString &strGroupName) :
    m_bRadio(bRadio),
    m_iGroupType(PVR_GROUP_TYPE_DEFAULT),
    m_iGroupId(iGroupId),
    m_strGroupName(strGroupName),
    m_bLoaded(false),
    m_bChanged(false),
    m_bUsingBackendChannelOrder(false)
{
}

CPVRChannelGroup::CPVRChannelGroup(const PVR_CHANNEL_GROUP &group) :
    m_bRadio(group.bIsRadio),
    m_iGroupType(PVR_GROUP_TYPE_DEFAULT),
    m_iGroupId(-1),
    m_strGroupName(group.strGroupName),
    m_bLoaded(false),
    m_bChanged(false),
    m_bUsingBackendChannelOrder(false)
{
}

CPVRChannelGroup::~CPVRChannelGroup(void)
{
  Unload();
}

bool CPVRChannelGroup::operator==(const CPVRChannelGroup& right) const
{
  return (m_bRadio == right.m_bRadio &&
      m_iGroupType == right.m_iGroupType &&
      m_iGroupId == right.m_iGroupId &&
      m_strGroupName.Equals(right.m_strGroupName));
}

bool CPVRChannelGroup::operator!=(const CPVRChannelGroup &right) const
{
  return !(*this == right);
}

CPVRChannelGroup::CPVRChannelGroup(const CPVRChannelGroup &group)
{
  m_bRadio                      = group.m_bRadio;
  m_iGroupType                  = group.m_iGroupType;
  m_iGroupId                    = group.m_iGroupId;
  m_strGroupName                = group.m_strGroupName;
  m_bLoaded                     = group.m_bLoaded;
  m_bChanged                    = group.m_bChanged;
  m_bUsingBackendChannelOrder   = group.m_bUsingBackendChannelOrder;
  m_bUsingBackendChannelNumbers = group.m_bUsingBackendChannelNumbers;

  for (int iPtr = 0; iPtr < group.Size(); iPtr++)
    m_members.push_back(group.m_members.at(iPtr));
}

int CPVRChannelGroup::Load(void)
{
  /* make sure this container is empty before loading */
  Unload();

  m_bUsingBackendChannelOrder   = g_guiSettings.GetBool("pvrmanager.backendchannelorder");
  m_bUsingBackendChannelNumbers = g_guiSettings.GetBool("pvrmanager.usebackendchannelnumbers");

  int iChannelCount = m_iGroupId > 0 ? LoadFromDb() : 0;
  CLog::Log(LOGDEBUG, "PVRChannelGroup - %s - %d channels loaded from the database for group '%s'",
        __FUNCTION__, iChannelCount, m_strGroupName.c_str());

  Update();
  if (Size() - iChannelCount > 0)
  {
    CLog::Log(LOGDEBUG, "PVRChannelGroup - %s - %d channels added from clients to group '%s'",
        __FUNCTION__, Size() - iChannelCount, m_strGroupName.c_str());
  }

  SortByChannelNumber();
  Renumber();

  g_guiSettings.RegisterObserver(this);
  m_bLoaded = true;

  return Size();
}

void CPVRChannelGroup::Unload(void)
{
  g_guiSettings.UnregisterObserver(this);
  CSingleLock lock(m_critSection);
  m_members.clear();
}

bool CPVRChannelGroup::Update(void)
{
  if (GroupType() == PVR_GROUP_TYPE_USER_DEFINED ||
      !g_guiSettings.GetBool("pvrmanager.syncchannelgroups"))
    return false;

  CPVRChannelGroup PVRChannels_tmp(m_bRadio, m_iGroupId, m_strGroupName);
  PVRChannels_tmp.LoadFromClients();

  return UpdateGroupEntries(PVRChannels_tmp);
}

bool CPVRChannelGroup::SetChannelNumber(const CPVRChannel &channel, unsigned int iChannelNumber)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
  {
    if (*m_members.at(iChannelPtr).channel == channel)
    {
      if (m_members.at(iChannelPtr).iChannelNumber != iChannelNumber)
      {
        m_bChanged = true;
        bReturn = true;
        m_members.at(iChannelPtr).iChannelNumber = iChannelNumber;
      }
      break;
    }
  }

  return bReturn;
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
  if (iOldChannelNumber > m_members.size())
    return bReturn;

  /* new channel number out of range */
  if (iNewChannelNumber > m_members.size())
    iNewChannelNumber = m_members.size();

  /* move the channel in the list */
  PVRChannelGroupMember entry = m_members.at(iOldChannelNumber - 1);
  m_members.erase(m_members.begin() + iOldChannelNumber - 1);
  m_members.insert(m_members.begin() + iNewChannelNumber - 1, entry);

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
  if (g_guiSettings.GetString("pvrmenu.iconpath").IsEmpty())
    return;

  CPVRDatabase *database = GetPVRDatabase();
  if (!database)
    return;

  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < m_members.size(); ptr++)
  {
    PVRChannelGroupMember groupMember = m_members.at(ptr);

    /* skip if an icon is already set */
    if (!groupMember.channel->IconPath().IsEmpty())
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
  sort(m_members.begin(), m_members.end(), sortByClientChannelNumber());
}

void CPVRChannelGroup::SortByChannelNumber(void)
{
  CSingleLock lock(m_critSection);
  sort(m_members.begin(), m_members.end(), sortByChannelNumber());
}

/********** getters **********/

CPVRChannel *CPVRChannelGroup::GetByClient(int iUniqueChannelId, int iClientID) const
{
  CPVRChannel *channel = NULL;
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < m_members.size(); ptr++)
  {
    PVRChannelGroupMember groupMember = m_members.at(ptr);
    if (groupMember.channel->UniqueID() == iUniqueChannelId &&
        groupMember.channel->ClientID() == iClientID)
    {
      channel = groupMember.channel;
      break;
    }
  }

  return channel;
}

CPVRChannel *CPVRChannelGroup::GetByChannelID(int iChannelID) const
{
  CPVRChannel *channel = NULL;
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < m_members.size(); ptr++)
  {
    PVRChannelGroupMember groupMember = m_members.at(ptr);
    if (groupMember.channel->ChannelID() == iChannelID)
    {
      channel = groupMember.channel;
      break;
    }
  }

  return channel;
}

CPVRChannel *CPVRChannelGroup::GetByChannelEpgID(int iEpgID) const
{
  CPVRChannel *channel = NULL;
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < m_members.size(); ptr++)
  {
    PVRChannelGroupMember groupMember = m_members.at(ptr);
    if (groupMember.channel->EpgID() == iEpgID)
    {
      channel = groupMember.channel;
      break;
    }
  }

  return channel;
}

CPVRChannel *CPVRChannelGroup::GetByUniqueID(int iUniqueID) const
{
  CPVRChannel *channel = NULL;
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < m_members.size(); ptr++)
  {
    PVRChannelGroupMember groupMember = m_members.at(ptr);
    if (groupMember.channel->UniqueID() == iUniqueID)
    {
      channel = groupMember.channel;
      break;
    }
  }

  return channel;
}

CFileItemPtr CPVRChannelGroup::GetLastPlayedChannel(void) const
{
  CPVRChannel *channel = NULL;
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
  {
    PVRChannelGroupMember groupMember = m_members.at(iChannelPtr);

    /* check whether the client is loaded */
    if (!g_PVRClients->IsConnectedClient(groupMember.channel->ClientID()))
      continue;

    /* always get the first channel */
    if (channel == NULL)
    {
      channel = groupMember.channel;
      continue;
    }

    /* check whether this channel has a later LastWatched time */
    if (groupMember.channel->LastWatched() > channel->LastWatched())
      channel = groupMember.channel;
  }

  if (channel)
  {
    CFileItemPtr retVal = CFileItemPtr(new CFileItem(*channel));
    return retVal;
  }

  CFileItemPtr retVal = CFileItemPtr(new CFileItem);
  return retVal;
}


unsigned int CPVRChannelGroup::GetChannelNumber(const CPVRChannel &channel) const
{
  unsigned int iReturn = 0;
  CSingleLock lock(m_critSection);
  unsigned int iSize = m_members.size();

  for (unsigned int iChannelPtr = 0; iChannelPtr < iSize; iChannelPtr++)
  {
    PVRChannelGroupMember member = m_members.at(iChannelPtr);
    if (member.channel->ChannelID() == channel.ChannelID())
    {
      iReturn = member.iChannelNumber;
      break;
    }
  }

  return iReturn;
}

CFileItemPtr CPVRChannelGroup::GetByChannelNumber(unsigned int iChannelNumber) const
{
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < m_members.size(); ptr++)
  {
    PVRChannelGroupMember groupMember = m_members.at(ptr);
    if (groupMember.iChannelNumber == iChannelNumber)
    {
      CFileItemPtr retVal = CFileItemPtr(new CFileItem(*groupMember.channel));
      return retVal;
    }
  }

  CFileItemPtr retVal = CFileItemPtr(new CFileItem);
  return retVal;
}

CFileItemPtr CPVRChannelGroup::GetByChannelUpDown(const CFileItem &channel, bool bChannelUp) const
{
  if (channel.HasPVRChannelInfoTag())
  {
    CSingleLock lock(m_critSection);
    int iChannelIndex = GetIndex(*channel.GetPVRChannelInfoTag());

    bool bGotChannel(false);
    while (!bGotChannel)
    {
      if (bChannelUp)
        iChannelIndex++;
      else
        iChannelIndex--;

      if (iChannelIndex >= (int)m_members.size())
        iChannelIndex = 0;
      else if (iChannelIndex < 0)
        iChannelIndex = m_members.size() - 1;

      CFileItemPtr current = GetByIndex(iChannelIndex);
      if (!current || *current->GetPVRChannelInfoTag() == *channel.GetPVRChannelInfoTag())
        break;

      if (!current->GetPVRChannelInfoTag()->IsHidden())
        return current;
    }
  }

  CFileItemPtr retVal(new CFileItem);
  return retVal;
}

CFileItemPtr CPVRChannelGroup::GetByIndex(unsigned int iIndex) const
{
  CSingleLock lock(m_critSection);
  if (iIndex < m_members.size())
  {
    CFileItemPtr retVal = CFileItemPtr(new CFileItem(*m_members.at(iIndex).channel));
    return retVal;
  }

  CFileItemPtr retVal = CFileItemPtr(new CFileItem);
  return retVal;
}

int CPVRChannelGroup::GetIndex(const CPVRChannel &channel) const
{
  int iIndex(-1);
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
  {
    if (*m_members.at(iChannelPtr).channel == channel)
    {
      iIndex = iChannelPtr;
      break;
    }
  }

  return iIndex;
}

int CPVRChannelGroup::GetMembers(CFileItemList &results, bool bGroupMembers /* = true */) const
{
  int iOrigSize = results.Size();
  CSingleLock lock(m_critSection);

  const CPVRChannelGroup* channels = bGroupMembers ? this : g_PVRChannelGroups->GetGroupAll(m_bRadio).get();
  for (unsigned int iChannelPtr = 0; iChannelPtr < channels->m_members.size(); iChannelPtr++)
  {
    CPVRChannel *channel = channels->m_members.at(iChannelPtr).channel;
    if (!channel)
      continue;

    if (bGroupMembers || !IsGroupMember(*channel))
    {
      CFileItemPtr pFileItem(new CFileItem(*channel));
      results.Add(pFileItem);
    }
  }

  return results.Size() - iOrigSize;
}

CPVRChannelGroupPtr CPVRChannelGroup::GetNextGroup(void) const
{
  return g_PVRChannelGroups->Get(m_bRadio)->GetNextGroup(*this);
}

CPVRChannelGroupPtr CPVRChannelGroup::GetPreviousGroup(void) const
{
  return g_PVRChannelGroups->Get(m_bRadio)->GetPreviousGroup(*this);
}

/********** private methods **********/

int CPVRChannelGroup::LoadFromDb(bool bCompress /* = false */)
{
  CPVRDatabase *database = GetPVRDatabase();
  if (!database)
    return -1;

  int iChannelCount = Size();

  database->Get(*this);

  return Size() - iChannelCount;
}

int CPVRChannelGroup::LoadFromClients(void)
{
  int iCurSize = Size();

  /* get the channels from the backends */
  PVR_ERROR error;
  g_PVRClients->GetChannelGroupMembers(this, &error);
  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGWARNING, "PVRChannelGroup - %s - got bad error (%d) on call to GetChannelGroupMembers", __FUNCTION__, error);

  return Size() - iCurSize;
}

bool CPVRChannelGroup::AddAndUpdateChannels(const CPVRChannelGroup &channels, bool bUseBackendChannelNumbers)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  /* go through the channel list and check for new channels.
     channels will only by updated in CPVRChannelGroupInternal to prevent dupe updates */
  for (unsigned int iChannelPtr = 0; iChannelPtr < channels.m_members.size(); iChannelPtr++)
  {
    PVRChannelGroupMember member = channels.m_members.at(iChannelPtr);
    if (!member.channel)
      continue;

    /* check whether this channel is known in the internal group */
    CPVRChannel *existingChannel = (CPVRChannel *) g_PVRChannelGroups->GetGroupAll(m_bRadio)->GetByClient(member.channel->UniqueID(), member.channel->ClientID());
    if (!existingChannel)
      continue;

    /* if it's found, add the channel to this group */
    if (!IsGroupMember(*existingChannel))
    {
      int iChannelNumber = bUseBackendChannelNumbers ? member.channel->ClientChannelNumber() : 0;
      AddToGroup(*existingChannel, iChannelNumber, false);

      bReturn = true;
      CLog::Log(LOGINFO,"PVRChannelGroup - %s - added %s channel '%s' at position %d in group '%s'",
          __FUNCTION__, m_bRadio ? "radio" : "TV", existingChannel->ChannelName().c_str(), iChannelNumber, GroupName().c_str());
    }
  }

  return bReturn;
}

bool CPVRChannelGroup::RemoveDeletedChannels(const CPVRChannelGroup &channels)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  /* check for deleted channels */
  for (int iChannelPtr = m_members.size() - 1; iChannelPtr >= 0; iChannelPtr--)
  {
    CPVRChannel *channel = m_members.at(iChannelPtr).channel;
    if (!channel)
      continue;

    if (channels.GetByClient(channel->UniqueID(), channel->ClientID()) == NULL)
    {
      /* channel was not found */
      CLog::Log(LOGINFO,"PVRChannelGroup - %s - deleted %s channel '%s' from group '%s'",
          __FUNCTION__, m_bRadio ? "radio" : "TV", channel->ChannelName().c_str(), GroupName().c_str());

      /* remove this channel from all non-system groups if this is the internal group */
      if (IsInternalGroup())
      {
        g_PVRChannelGroups->Get(m_bRadio)->RemoveFromAllGroups(*channel);

        /* since it was not found in the internal group, it was deleted from the backend */
        channel->Delete();
      }

      m_members.erase(m_members.begin() + iChannelPtr);
      m_bChanged = true;
      bReturn = true;
    }
  }

  return bReturn;
}

bool CPVRChannelGroup::UpdateGroupEntries(const CPVRChannelGroup &channels)
{
  bool bReturn(false);
  bool bChanged(false);
  bool bRemoved(false);

  CSingleLock lock(m_critSection);
  /* sort by client channel number if this is the first time or if pvrmanager.backendchannelorder is true */
  bool bUseBackendChannelNumbers(m_members.size() == 0 || m_bUsingBackendChannelOrder);

  CPVRDatabase *database = GetPVRDatabase();
  if (!database)
    return bReturn;

  bRemoved = RemoveDeletedChannels(channels);
  bChanged = AddAndUpdateChannels(channels, bUseBackendChannelNumbers) || bRemoved;

  if (bChanged)
  {
    if (bUseBackendChannelNumbers)
      SortByClientChannelNumber();

    /* renumber to make sure all channels have a channel number.
       new channels were added at the back, so they'll get the highest numbers */
    bool bRenumbered = Renumber();

    SetChanged();
    lock.Leave();

    NotifyObservers(HasNewChannels() || bRemoved || bRenumbered ? "channelgroup-reset" : "channelgroup");

    bReturn = Persist();
  }
  else
  {
    bReturn = true;
  }

  return bReturn;
}

void CPVRChannelGroup::RemoveInvalidChannels(void)
{
  bool bDelete(false);
  CSingleLock lock(m_critSection);
  for (unsigned int ptr = 0; ptr < m_members.size(); ptr--)
  {
    bDelete = false;
    CPVRChannel *channel = m_members.at(ptr).channel;
    if (channel->IsVirtual())
      continue;

    if (m_members.at(ptr).channel->ClientChannelNumber() <= 0)
    {
      CLog::Log(LOGERROR, "PVRChannelGroup - %s - removing invalid channel '%s' from client '%i': no valid client channel number",
          __FUNCTION__, channel->ChannelName().c_str(), channel->ClientID());
      bDelete = true;
    }

    if (!bDelete && channel->UniqueID() <= 0)
    {
      CLog::Log(LOGERROR, "PVRChannelGroup - %s - removing invalid channel '%s' from client '%i': no valid unique ID",
          __FUNCTION__, channel->ChannelName().c_str(), channel->ClientID());
      bDelete = true;
    }

    /* remove this channel from all non-system groups if this is the internal group */
    if (bDelete)
    {
      if (IsInternalGroup())
      {
        g_PVRChannelGroups->Get(m_bRadio)->RemoveFromAllGroups(*channel);
        channel->Delete();
      }
      else
      {
          m_members.erase(m_members.begin() + ptr);
      }
      m_bChanged = true;
    }
  }
}

bool CPVRChannelGroup::RemoveFromGroup(const CPVRChannel &channel)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
  {
    if (channel == *m_members.at(iChannelPtr).channel)
    {
      // TODO notify observers
        m_members.erase(m_members.begin() + iChannelPtr);
      bReturn = true;
      m_bChanged = true;
      break;
    }
  }

  Renumber();

  return bReturn;
}

bool CPVRChannelGroup::AddToGroup(CPVRChannel &channel, int iChannelNumber /* = 0 */, bool bSortAndRenumber /* = true */)
{
  CSingleLock lock(m_critSection);

  bool bReturn(false);

  if (!CPVRChannelGroup::IsGroupMember(channel))
  {
    if (iChannelNumber <= 0 || iChannelNumber > (int) m_members.size() + 1)
      iChannelNumber = m_members.size() + 1;

    CPVRChannel *realChannel = (IsInternalGroup()) ?
        &channel :
        (CPVRChannel *) g_PVRChannelGroups->GetGroupAll(m_bRadio)->GetByClient(channel.UniqueID(), channel.ClientID());

    if (realChannel)
    {
      PVRChannelGroupMember newMember = { realChannel, iChannelNumber };
      m_members.push_back(newMember);
      m_bChanged = true;

      if (bSortAndRenumber)
      {
        if (m_bUsingBackendChannelOrder)
          SortByClientChannelNumber();
        else
          SortByChannelNumber();
        Renumber();
      }

      // TODO notify observers
      bReturn = true;
    }
  }

  return bReturn;
}

bool CPVRChannelGroup::IsGroupMember(const CPVRChannel &channel) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
  {
    if (channel == *m_members.at(iChannelPtr).channel)
    {
      bReturn = true;
      break;
    }
  }

  return bReturn;
}

bool CPVRChannelGroup::IsGroupMember(int iChannelId) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
  {
    if (iChannelId == m_members.at(iChannelPtr).channel->ChannelID())
    {
      bReturn = true;
      break;
    }
  }

  return bReturn;
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
  bool bReturn(true);
  CSingleLock lock(m_critSection);

  if (!HasChanges())
    return bReturn;

  if (CPVRDatabase *database = GetPVRDatabase())
  {
    CLog::Log(LOGDEBUG, "CPVRChannelGroup - %s - persisting channel group '%s' with %d channels",
        __FUNCTION__, GroupName().c_str(), (int) m_members.size());
    m_bChanged = false;
    lock.Leave();

    bReturn = database->Persist(*this);
  }
  else
  {
    bReturn = false;
  }

  return bReturn;
}

bool CPVRChannelGroup::Renumber(void)
{
  bool bReturn(false);
  unsigned int iChannelNumber(0);
  bool bUseBackendChannelNumbers(g_guiSettings.GetBool("pvrmanager.usebackendchannelnumbers") && g_PVRClients->EnabledClientAmount() == 1);
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size();  iChannelPtr++)
  {
    unsigned int iCurrentChannelNumber;
    if (m_members.at(iChannelPtr).channel->IsHidden())
      iCurrentChannelNumber = 0;
    else if (bUseBackendChannelNumbers)
      iCurrentChannelNumber = m_members.at(iChannelPtr).channel->ClientChannelNumber();
    else
      iCurrentChannelNumber = ++iChannelNumber;

    if (m_members.at(iChannelPtr).iChannelNumber != iCurrentChannelNumber)
    {
      bReturn = true;
      m_bChanged = true;
    }

    m_members.at(iChannelPtr).iChannelNumber = iCurrentChannelNumber;
  }

  SortByChannelNumber();
  ResetChannelNumberCache();

  return bReturn;
}

void CPVRChannelGroup::ResetChannelNumberCache(void)
{
  CPVRChannelGroupPtr playingGroup = g_PVRManager.GetPlayingGroup(m_bRadio);
  if (*this != *playingGroup)
    return;

  CSingleLock lock(m_critSection);

  /* reset the channel number cache */
  if (!IsInternalGroup())
    g_PVRChannelGroups->GetGroupAll(m_bRadio)->ResetChannelNumbers();

  /* set all channel numbers on members of this group */
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
    m_members.at(iChannelPtr).channel->SetCachedChannelNumber(m_members.at(iChannelPtr).iChannelNumber);
}

bool CPVRChannelGroup::HasChangedChannels(void) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
  {
    if (m_members.at(iChannelPtr).channel->IsChanged())
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

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
  {
    if (m_members.at(iChannelPtr).channel->ChannelID() <= 0)
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

void CPVRChannelGroup::ResetChannelNumbers(void)
{
  CSingleLock lock(m_critSection);
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
    m_members.at(iChannelPtr).channel->SetCachedChannelNumber(0);
}

void CPVRChannelGroup::Notify(const Observable &obs, const CStdString& msg)
{
  if (msg.Equals("settings"))
  {
    CSingleLock lock(m_critSection);
    bool bUsingBackendChannelOrder   = g_guiSettings.GetBool("pvrmanager.backendchannelorder");
    bool bUsingBackendChannelNumbers = g_guiSettings.GetBool("pvrmanager.usebackendchannelnumbers");
    bool bChannelNumbersChanged      = m_bUsingBackendChannelNumbers != bUsingBackendChannelNumbers;
    bool bChannelOrderChanged        = m_bUsingBackendChannelOrder != bUsingBackendChannelOrder;

    m_bUsingBackendChannelOrder   = bUsingBackendChannelOrder;
    m_bUsingBackendChannelNumbers = bUsingBackendChannelNumbers;

    /* check whether this channel group has to be renumbered */
    if (bChannelOrderChanged || bChannelNumbersChanged)
    {
      CLog::Log(LOGDEBUG, "CPVRChannelGroup - %s - renumbering group '%s' to use the backend channel order and/or numbers",
          __FUNCTION__, m_strGroupName.c_str());
      SortByClientChannelNumber();
      Renumber();
      Persist();
    }
  }
}

bool CPVRPersistGroupJob::DoWork(void)
{
  return m_group->Persist();
}

int CPVRChannelGroup::GetEPGSearch(CFileItemList &results, const EpgSearchFilter &filter)
{
  int iInitialSize = results.Size();

  /* get filtered results from all tables */
  g_EpgContainer.GetEPGSearch(results, filter);

  /* remove duplicate entries */
  if (filter.m_bPreventRepeats)
    EpgSearchFilter::RemoveDuplicates(results);

  /* filter recordings */
  if (filter.m_bIgnorePresentRecordings)
    EpgSearchFilter::FilterRecordings(results);

  /* filter timers */
  if (filter.m_bIgnorePresentTimers)
    EpgSearchFilter::FilterTimers(results);

  return results.Size() - iInitialSize;
}

int CPVRChannelGroup::GetEPGNow(CFileItemList &results)
{
  int iInitialSize = results.Size();
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
  {
    CPVRChannel *channel = m_members.at(iChannelPtr).channel;
    CEpg *epg = channel->GetEPG();
    if (!epg || !epg->HasValidEntries() || m_members.at(iChannelPtr).channel->IsHidden())
      continue;

    CEpgInfoTag epgNow;
    if (!epg->InfoTagNow(epgNow))
      continue;

    CFileItemPtr entry(new CFileItem(epgNow));
    entry->SetLabel2(epgNow.StartAsLocalTime().GetAsLocalizedTime(StringUtils::EmptyString, false));
    entry->SetPath(channel->ChannelName());
    entry->SetThumbnailImage(channel->IconPath());
    results.Add(entry);
  }

  return results.Size() - iInitialSize;
}

int CPVRChannelGroup::GetEPGNext(CFileItemList &results)
{
  int iInitialSize = results.Size();
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
  {
    CPVRChannel *channel = m_members.at(iChannelPtr).channel;
    CEpg *epg = channel->GetEPG();
    if (!epg || !epg->HasValidEntries() || m_members.at(iChannelPtr).channel->IsHidden())
      continue;

    CEpgInfoTag epgNow;
    if (!epg->InfoTagNext(epgNow))
      continue;

    CFileItemPtr entry(new CFileItem(epgNow));
    entry->SetLabel2(epgNow.StartAsLocalTime().GetAsLocalizedTime(StringUtils::EmptyString, false));
    entry->SetPath(channel->ChannelName());
    entry->SetThumbnailImage(channel->IconPath());
    results.Add(entry);
  }

  return results.Size() - iInitialSize;
}

int CPVRChannelGroup::GetEPGAll(CFileItemList &results)
{
  int iInitialSize = results.Size();
  CSingleLock lock(m_critSection);

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
  {
    if (!m_members.at(iChannelPtr).channel || m_members.at(iChannelPtr).channel->IsHidden())
      continue;

    m_members.at(iChannelPtr).channel->GetEPG(results);
  }

  return results.Size() - iInitialSize;
}

bool CPVRChannelGroup::IsValid(void) const
{
  CSingleLock lock(m_critSection);
  return m_iGroupId >= 0;
}

int CPVRChannelGroup::Size(void) const
{
  CSingleLock lock(m_critSection);
  return m_members.size();
}

int CPVRChannelGroup::GroupID(void) const
{
  CSingleLock lock(m_critSection);
  return m_iGroupId;
}

void CPVRChannelGroup::SetGroupID(int iGroupId)
{
  if (iGroupId >= 0)
  {
    CSingleLock lock(m_critSection);
    m_iGroupId = iGroupId;
  }
}

void CPVRChannelGroup::SetGroupType(int iGroupType)
{
  CSingleLock lock(m_critSection);
  m_iGroupType = iGroupType;
}

int CPVRChannelGroup::GroupType(void) const
{
  CSingleLock lock(m_critSection);
  return m_iGroupType;
}

CStdString CPVRChannelGroup::GroupName(void) const
{
  CSingleLock lock(m_critSection);
  CStdString strReturn(m_strGroupName);
  return strReturn;
}

bool CPVRChannelGroup::UpdateChannel(const CFileItem &item, bool bHidden, bool bVirtual, bool bEPGEnabled, bool bParentalLocked, int iEPGSource, int iChannelNumber, const CStdString &strChannelName, const CStdString &strIconPath, const CStdString &strStreamURL)
{
  if (!item.HasPVRChannelInfoTag())
    return false;

  CSingleLock lock(m_critSection);

  /* get the real channel from the group */
  CPVRChannel *channel = GetByUniqueID(item.GetPVRChannelInfoTag()->UniqueID());
  if (!channel)
    return false;

  channel->SetChannelName(strChannelName);
  channel->SetHidden(bHidden);
  channel->SetLocked(bParentalLocked);
  channel->SetIconPath(strIconPath);

  if (bVirtual)
    channel->SetStreamURL(strStreamURL);
  if (iEPGSource == 0)
    channel->SetEPGScraper("client");

  // TODO add other scrapers
  channel->SetEPGEnabled(bEPGEnabled);

  /* set new values in the channel tag */
  if (bHidden)
  {
    SortByChannelNumber(); // or previous changes will be overwritten
    RemoveFromGroup(*channel);
  }
  else
  {
    SetChannelNumber(*channel, iChannelNumber);
  }

  return true;
}

bool CPVRChannelGroup::ToggleChannelLocked(const CFileItem &item)
{
  if (!item.HasPVRChannelInfoTag())
    return false;

  CSingleLock lock(m_critSection);

  /* get the real channel from the group */
  CPVRChannel *channel = GetByUniqueID(item.GetPVRChannelInfoTag()->UniqueID());
  if (!channel)
    return false;

  channel->SetLocked(!channel->IsLocked());

  return true;
}
