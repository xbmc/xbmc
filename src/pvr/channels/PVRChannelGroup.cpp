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

/**
 * TODO:
 * - use Observable here, so we can use event driven operations later
 */

#include "settings/AdvancedSettings.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/log.h"
#include "Util.h"
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
    m_bUsingBackendChannelOrder(false),
    m_bSelectedGroup(false),
    m_bPreventSortAndRenumber(false),
    m_iLastWatched(0)
{
}

CPVRChannelGroup::CPVRChannelGroup(bool bRadio, unsigned int iGroupId, const std::string &strGroupName) :
    m_bRadio(bRadio),
    m_iGroupType(PVR_GROUP_TYPE_DEFAULT),
    m_iGroupId(iGroupId),
    m_strGroupName(strGroupName),
    m_bLoaded(false),
    m_bChanged(false),
    m_bUsingBackendChannelOrder(false),
    m_bSelectedGroup(false),
    m_bPreventSortAndRenumber(false),
    m_iLastWatched(0)
{
}

CPVRChannelGroup::CPVRChannelGroup(const PVR_CHANNEL_GROUP &group) :
    m_bRadio(group.bIsRadio),
    m_iGroupType(PVR_GROUP_TYPE_DEFAULT),
    m_iGroupId(-1),
    m_strGroupName(group.strGroupName),
    m_bLoaded(false),
    m_bChanged(false),
    m_bUsingBackendChannelOrder(false),
    m_bSelectedGroup(false),
    m_bPreventSortAndRenumber(false),
    m_iLastWatched(0)
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
      m_strGroupName == right.m_strGroupName);
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
  m_iLastWatched                = group.m_iLastWatched;

  for (int iPtr = 0; iPtr < group.Size(); iPtr++)
    m_members.push_back(group.m_members.at(iPtr));
}

bool CPVRChannelGroup::Load(void)
{
  /* make sure this container is empty before loading */
  Unload();

  m_bUsingBackendChannelOrder   = CSettings::Get().GetBool("pvrmanager.backendchannelorder");
  m_bUsingBackendChannelNumbers = CSettings::Get().GetBool("pvrmanager.usebackendchannelnumbers");

  int iChannelCount = m_iGroupId > 0 ? LoadFromDb() : 0;
  CLog::Log(LOGDEBUG, "PVRChannelGroup - %s - %d channels loaded from the database for group '%s'",
        __FUNCTION__, iChannelCount, m_strGroupName.c_str());

  if (!Update())
  {
    CLog::Log(LOGERROR, "PVRChannelGroup - %s - failed to update channels", __FUNCTION__);
    return false;
  }

  if (Size() - iChannelCount > 0)
  {
    CLog::Log(LOGDEBUG, "PVRChannelGroup - %s - %d channels added from clients to group '%s'",
        __FUNCTION__, Size() - iChannelCount, m_strGroupName.c_str());
  }

  SortAndRenumber();

  m_bLoaded = true;

  return true;
}

void CPVRChannelGroup::Unload(void)
{
  CSingleLock lock(m_critSection);
  m_members.clear();
}

bool CPVRChannelGroup::Update(void)
{
  if (GroupType() == PVR_GROUP_TYPE_USER_DEFINED ||
      !CSettings::Get().GetBool("pvrmanager.syncchannelgroups"))
    return true;

  CPVRChannelGroup PVRChannels_tmp(m_bRadio, m_iGroupId, m_strGroupName);
  PVRChannels_tmp.SetPreventSortAndRenumber();
  PVRChannels_tmp.LoadFromClients();

  return UpdateGroupEntries(PVRChannels_tmp);
}

bool CPVRChannelGroup::SetChannelNumber(const CPVRChannel &channel, unsigned int iChannelNumber, unsigned int iSubChannelNumber /* = 0 */)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (std::vector<PVRChannelGroupMember>::iterator it = m_members.begin(); it != m_members.end(); ++it)
  {
    if (*((*it).channel) == channel)
    {
      if ((*it).iChannelNumber    != iChannelNumber ||
          (*it).iSubChannelNumber != iSubChannelNumber)
      {
        m_bChanged = true;
        bReturn = true;
        (*it).iChannelNumber    = iChannelNumber;
        (*it).iSubChannelNumber = iSubChannelNumber;
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
  if (iNewChannelNumber < 1)
    return bReturn;

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

  return bReturn;
}

void CPVRChannelGroup::SearchAndSetChannelIcons(bool bUpdateDb /* = false */)
{
  std::string iconPath = CSettings::Get().GetString("pvrmenu.iconpath");
  if (iconPath.empty())
    return;

  CPVRDatabase *database = GetPVRDatabase();
  if (!database)
    return;

  /* fetch files in icon path for fast lookup */
  CFileItemList fileItemList;
  XFILE::CDirectory::GetDirectory(iconPath, fileItemList, ".jpg|.png|.tbn");

  if (fileItemList.IsEmpty())
    return;

  CGUIDialogExtendedProgressBar* dlgProgress = (CGUIDialogExtendedProgressBar*)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);
  CGUIDialogProgressBarHandle* dlgProgressHandle = dlgProgress ? dlgProgress->GetHandle(g_localizeStrings.Get(19287)) : NULL;

  CSingleLock lock(m_critSection);

  /* create a map for fast lookup of normalized file base name */
  std::map<std::string, std::string> fileItemMap;
  const VECFILEITEMS &items = fileItemList.GetList();
  for(VECFILEITEMS::const_iterator it = items.begin(); it != items.end(); ++it)
  {
    std::string baseName = URIUtils::GetFileName((*it)->GetPath());
    URIUtils::RemoveExtension(baseName);
    StringUtils::ToLower(baseName);
    fileItemMap.insert(std::make_pair(baseName, (*it)->GetPath()));
  }

  int channelIndex = 0;
  for(std::vector<PVRChannelGroupMember>::const_iterator it = m_members.begin(); it != m_members.end(); ++it)
  {
    CPVRChannelPtr channel = (*it).channel;

    /* update progress dialog */
    if (dlgProgressHandle)
    {
      dlgProgressHandle->SetProgress(channelIndex++, m_members.size());
      dlgProgressHandle->SetText(channel->ChannelName());
    }

    /* skip if an icon is already set and exists */
    if (channel->IsIconExists())
      continue;

    /* reset icon before searching for a new one */
    channel->SetIconPath("");

    std::string strChannelUid = StringUtils::Format("%08d", channel->UniqueID());
    std::string strLegalClientChannelName = CUtil::MakeLegalFileName(channel->ClientChannelName());
    StringUtils::ToLower(strLegalClientChannelName);
    std::string strLegalChannelName = CUtil::MakeLegalFileName(channel->ChannelName());
    StringUtils::ToLower(strLegalChannelName);

    std::map<std::string, std::string>::iterator itItem;
    if ((itItem = fileItemMap.find(strLegalClientChannelName)) != fileItemMap.end() ||
        (itItem = fileItemMap.find(strLegalChannelName)) != fileItemMap.end() ||
        (itItem = fileItemMap.find(strChannelUid)) != fileItemMap.end())
    {
      channel->SetIconPath(itItem->second, g_advancedSettings.m_bPVRAutoScanIconsUserSet);
    }

    if (bUpdateDb)
      channel->Persist();

    /* TODO: start channel icon scraper here if nothing was found */
  }

  if (dlgProgressHandle)
    dlgProgressHandle->MarkFinished();
}

/********** sort methods **********/

struct sortByClientChannelNumber
{
  bool operator()(const PVRChannelGroupMember &channel1, const PVRChannelGroupMember &channel2)
  {
    if (channel1.channel->ClientChannelNumber() == channel2.channel->ClientChannelNumber())
      return channel1.channel->ClientSubChannelNumber() < channel2.channel->ClientSubChannelNumber();
    return channel1.channel->ClientChannelNumber() < channel2.channel->ClientChannelNumber();
  }
};

struct sortByChannelNumber
{
  bool operator()(const PVRChannelGroupMember &channel1, const PVRChannelGroupMember &channel2)
  {
    if (channel1.iChannelNumber == channel2.iChannelNumber)
      return channel1.iSubChannelNumber < channel2.iSubChannelNumber;
    return channel1.iChannelNumber < channel2.iChannelNumber;
  }
};

bool CPVRChannelGroup::SortAndRenumber(void)
{
  if (PreventSortAndRenumber())
    return true;

  CSingleLock lock(m_critSection);
  if (m_bUsingBackendChannelOrder)
    SortByClientChannelNumber();
  else
    SortByChannelNumber();

  bool bReturn = Renumber();
  ResetChannelNumberCache();
  return bReturn;
}

void CPVRChannelGroup::SortByClientChannelNumber(void)
{
  CSingleLock lock(m_critSection);
  if (!PreventSortAndRenumber())
    sort(m_members.begin(), m_members.end(), sortByClientChannelNumber());
}

void CPVRChannelGroup::SortByChannelNumber(void)
{
  CSingleLock lock(m_critSection);
  if (!PreventSortAndRenumber())
    sort(m_members.begin(), m_members.end(), sortByChannelNumber());
}

/********** getters **********/

CPVRChannelPtr CPVRChannelGroup::GetByClient(int iUniqueChannelId, int iClientID) const
{
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < m_members.size(); ptr++)
  {
    PVRChannelGroupMember groupMember = m_members.at(ptr);
    if (groupMember.channel->UniqueID() == iUniqueChannelId &&
        groupMember.channel->ClientID() == iClientID)
      return groupMember.channel;
  }

  CPVRChannelPtr empty;
  return empty;
}

CPVRChannelPtr CPVRChannelGroup::GetByChannelID(int iChannelID) const
{
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < m_members.size(); ptr++)
  {
    PVRChannelGroupMember groupMember = m_members.at(ptr);
    if (groupMember.channel->ChannelID() == iChannelID)
      return groupMember.channel;
  }

  CPVRChannelPtr empty;
  return empty;
}

CPVRChannelPtr CPVRChannelGroup::GetByChannelEpgID(int iEpgID) const
{
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < m_members.size(); ptr++)
  {
    PVRChannelGroupMember groupMember = m_members.at(ptr);
    if (groupMember.channel->EpgID() == iEpgID)
      return groupMember.channel;
  }

  CPVRChannelPtr empty;
  return empty;
}

CPVRChannelPtr CPVRChannelGroup::GetByUniqueID(int iUniqueID) const
{
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < m_members.size(); ptr++)
  {
    PVRChannelGroupMember groupMember = m_members.at(ptr);
    if (groupMember.channel->UniqueID() == iUniqueID)
      return groupMember.channel;
  }

  CPVRChannelPtr empty;
  return empty;
}

CFileItemPtr CPVRChannelGroup::GetLastPlayedChannel(int iCurrentChannel /* = -1 */) const
{
  CSingleLock lock(m_critSection);

  CPVRChannelPtr returnChannel;
  for (std::vector<PVRChannelGroupMember>::const_iterator it = m_members.begin(); it != m_members.end(); ++it)
  {
    CPVRChannelPtr channel = (*it).channel;
    if (channel->ChannelID() != iCurrentChannel &&
        g_PVRClients->IsConnectedClient(channel->ClientID()) &&
        channel->LastWatched() > 0 &&
        (!returnChannel || channel->LastWatched() > returnChannel->LastWatched()))
    {
      returnChannel = channel;
    }
  }

  if (returnChannel)
  {
    CFileItemPtr retVal = CFileItemPtr(new CFileItem(*returnChannel));
    return retVal;
  }

  CFileItemPtr retVal = CFileItemPtr(new CFileItem);
  return retVal;
}

unsigned int CPVRChannelGroup::GetSubChannelNumber(const CPVRChannel &channel) const
{
  unsigned int iReturn = 0;
  CSingleLock lock(m_critSection);

  for (std::vector<PVRChannelGroupMember>::const_iterator it = m_members.begin(); it != m_members.end(); ++it)
  {
    if ((*it).channel->ChannelID() == channel.ChannelID())
    {
      iReturn = (*it).iSubChannelNumber;
      break;
    }
  }

  return iReturn;
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

CFileItemPtr CPVRChannelGroup::GetByChannelNumber(unsigned int iChannelNumber, unsigned int iSubChannelNumber /* = 0 */) const
{
  CSingleLock lock(m_critSection);

  for (unsigned int ptr = 0; ptr < m_members.size(); ptr++)
  {
    PVRChannelGroupMember groupMember = m_members.at(ptr);
    if (groupMember.iChannelNumber == iChannelNumber && (iSubChannelNumber == 0 || iSubChannelNumber == groupMember.iSubChannelNumber))
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
    CPVRChannelPtr channel = channels->m_members.at(iChannelPtr).channel;
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

bool CPVRChannelGroup::LoadFromClients(void)
{
  /* get the channels from the backends */
  return g_PVRClients->GetChannelGroupMembers(this) == PVR_ERROR_NO_ERROR;
}

bool CPVRChannelGroup::AddAndUpdateChannels(const CPVRChannelGroup &channels, bool bUseBackendChannelNumbers)
{
  bool bReturn(false);
  bool bPreventSortAndRenumber(PreventSortAndRenumber());
  CSingleLock lock(m_critSection);

  SetPreventSortAndRenumber();

  /* go through the channel list and check for new channels.
     channels will only by updated in CPVRChannelGroupInternal to prevent dupe updates */
  for (unsigned int iChannelPtr = 0; iChannelPtr < channels.m_members.size(); iChannelPtr++)
  {
    PVRChannelGroupMember member = channels.m_members.at(iChannelPtr);
    if (!member.channel)
      continue;

    /* check whether this channel is known in the internal group */
    CPVRChannelPtr existingChannel = g_PVRChannelGroups->GetGroupAll(m_bRadio)->GetByClient(member.channel->UniqueID(), member.channel->ClientID());
    if (!existingChannel)
      continue;

    /* if it's found, add the channel to this group */
    if (!IsGroupMember(*existingChannel))
    {
      int iChannelNumber = bUseBackendChannelNumbers ? member.channel->ClientChannelNumber() : 0;
      AddToGroup(*existingChannel, iChannelNumber);

      bReturn = true;
      CLog::Log(LOGINFO,"PVRChannelGroup - %s - added %s channel '%s' at position %d in group '%s'",
          __FUNCTION__, m_bRadio ? "radio" : "TV", existingChannel->ChannelName().c_str(), iChannelNumber, GroupName().c_str());
    }
  }

  SetPreventSortAndRenumber(bPreventSortAndRenumber);
  SortAndRenumber();

  return bReturn;
}

bool CPVRChannelGroup::RemoveDeletedChannels(const CPVRChannelGroup &channels)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  /* check for deleted channels */
  for (int iChannelPtr = m_members.size() - 1; iChannelPtr >= 0; iChannelPtr--)
  {
    CPVRChannelPtr channel = m_members.at(iChannelPtr).channel;
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
    /* renumber to make sure all channels have a channel number.
       new channels were added at the back, so they'll get the highest numbers */
    bool bRenumbered = SortAndRenumber();

    SetChanged();
    lock.Leave();

    NotifyObservers(HasNewChannels() || bRemoved || bRenumbered ? ObservableMessageChannelGroupReset : ObservableMessageChannelGroup);

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
    CPVRChannelPtr channel = m_members.at(ptr).channel;
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

bool CPVRChannelGroup::AddToGroup(CPVRChannel &channel, int iChannelNumber /* = 0 */)
{
  CSingleLock lock(m_critSection);

  bool bReturn(false);

  if (!CPVRChannelGroup::IsGroupMember(channel))
  {
    if (iChannelNumber <= 0 || iChannelNumber > (int) m_members.size() + 1)
      iChannelNumber = m_members.size() + 1;

    CPVRChannelPtr realChannel = (IsInternalGroup()) ?
        GetByClient(channel.UniqueID(), channel.ClientID()) :
        g_PVRChannelGroups->GetGroupAll(m_bRadio)->GetByClient(channel.UniqueID(), channel.ClientID());

    if (realChannel)
    {
      PVRChannelGroupMember newMember = { realChannel, (unsigned int)iChannelNumber };
      m_members.push_back(newMember);
      m_bChanged = true;

      SortAndRenumber();

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

bool CPVRChannelGroup::SetGroupName(const std::string &strGroupName, bool bSaveInDb /* = false */)
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
  bool bUseBackendChannelNumbers(CSettings::Get().GetBool("pvrmanager.usebackendchannelnumbers") && g_PVRClients->EnabledClientAmount() == 1);

  if (PreventSortAndRenumber())
    return true;

  CSingleLock lock(m_critSection);

  for (std::vector<PVRChannelGroupMember>::iterator it = m_members.begin(); it != m_members.end(); ++it)
  {
    unsigned int iCurrentChannelNumber, iSubChannelNumber;
    if ((*it).channel->IsHidden())
    {
      iCurrentChannelNumber = 0;
      iSubChannelNumber     = 0;
    }
    else if (bUseBackendChannelNumbers)
    {
      iCurrentChannelNumber = (*it).channel->ClientChannelNumber();
      iSubChannelNumber     = (*it).channel->ClientSubChannelNumber();
    }
    else
    {
      iCurrentChannelNumber = ++iChannelNumber;
      iSubChannelNumber     = 0;
    }

    if ((*it).iChannelNumber    != iCurrentChannelNumber ||
        (*it).iSubChannelNumber != iSubChannelNumber)
    {
      bReturn = true;
      m_bChanged = true;
    }

    (*it).iChannelNumber    = iCurrentChannelNumber;
    (*it).iSubChannelNumber = iSubChannelNumber;
  }

  SortByChannelNumber();
  ResetChannelNumberCache();

  return bReturn;
}

void CPVRChannelGroup::ResetChannelNumberCache(void)
{
  CSingleLock lock(m_critSection);
  if (!m_bSelectedGroup)
    return;

  /* reset the channel number cache */
  if (!IsInternalGroup())
    g_PVRChannelGroups->GetGroupAll(m_bRadio)->ResetChannelNumbers();

  /* set all channel numbers on members of this group */
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_members.size(); iChannelPtr++)
  {
    m_members.at(iChannelPtr).channel->SetCachedChannelNumber(m_members.at(iChannelPtr).iChannelNumber);
    m_members.at(iChannelPtr).channel->SetCachedSubChannelNumber(m_members.at(iChannelPtr).iSubChannelNumber);
  }
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

void CPVRChannelGroup::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  /* TODO: while pvr manager is starting up do accept setting changes. */
  if(!g_PVRManager.IsStarted())
  {
    CLog::Log(LOGWARNING, "CPVRChannelGroup setting change ignored while PVRManager is starting\n");
    return;
  }

  const std::string &settingId = setting->GetId();
  if (settingId == "pvrmanager.backendchannelorder" || settingId == "pvrmanager.usebackendchannelnumbers")
  {
    CSingleLock lock(m_critSection);
    bool bUsingBackendChannelOrder   = CSettings::Get().GetBool("pvrmanager.backendchannelorder");
    bool bUsingBackendChannelNumbers = CSettings::Get().GetBool("pvrmanager.usebackendchannelnumbers");
    bool bChannelNumbersChanged      = m_bUsingBackendChannelNumbers != bUsingBackendChannelNumbers;
    bool bChannelOrderChanged        = m_bUsingBackendChannelOrder != bUsingBackendChannelOrder;

    m_bUsingBackendChannelOrder   = bUsingBackendChannelOrder;
    m_bUsingBackendChannelNumbers = bUsingBackendChannelNumbers;
    lock.Leave();

    /* check whether this channel group has to be renumbered */
    if (bChannelOrderChanged || bChannelNumbersChanged)
    {
      CLog::Log(LOGDEBUG, "CPVRChannelGroup - %s - renumbering group '%s' to use the backend channel order and/or numbers",
          __FUNCTION__, m_strGroupName.c_str());
      SortAndRenumber();
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
    CPVRChannelPtr channel = m_members.at(iChannelPtr).channel;
    CEpg *epg = channel->GetEPG();
    if (!epg || !epg->HasValidEntries() || m_members.at(iChannelPtr).channel->IsHidden())
      continue;

    CEpgInfoTag epgNow;
    if (!epg->InfoTagNow(epgNow))
      continue;

    CFileItemPtr entry(new CFileItem(epgNow));
    entry->SetLabel2(epgNow.StartAsLocalTime().GetAsLocalizedTime("", false));
    entry->SetPath(channel->Path());
    entry->SetArt("thumb", channel->IconPath());
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
    CPVRChannelPtr channel = m_members.at(iChannelPtr).channel;
    CEpg *epg = channel->GetEPG();
    if (!epg || !epg->HasValidEntries() || m_members.at(iChannelPtr).channel->IsHidden())
      continue;

    CEpgInfoTag epgNow;
    if (!epg->InfoTagNext(epgNow))
      continue;

    CFileItemPtr entry(new CFileItem(epgNow));
    entry->SetLabel2(epgNow.StartAsLocalTime().GetAsLocalizedTime("", false));
    entry->SetPath(channel->Path());
    entry->SetArt("thumb", channel->IconPath());
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
    if (m_members.at(iChannelPtr).channel && !m_members.at(iChannelPtr).channel->IsHidden())
    {
      CEpg* epg = m_members.at(iChannelPtr).channel->GetEPG();
      if (epg)
      {
        // XXX channel pointers aren't set in some occasions. this works around the issue, but is not very nice
        epg->SetChannel(m_members.at(iChannelPtr).channel);
        epg->Get(results);
      }
    }
  }

  return results.Size() - iInitialSize;
}

CDateTime CPVRChannelGroup::GetEPGDate(EpgDateType epgDateType) const
{
  CDateTime date;
  CSingleLock lock(m_critSection);
  
  for (std::vector<PVRChannelGroupMember>::const_iterator it = m_members.begin(); it != m_members.end(); it++)
  {
    if (it->channel && !it->channel->IsHidden())
    {
      CEpg* epg = it->channel->GetEPG();
      if (epg)
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
  }
  
  return date;
}

CDateTime CPVRChannelGroup::GetFirstEPGDate(void) const
{
  return GetEPGDate(EPG_FIRST_DATE);
}

CDateTime CPVRChannelGroup::GetLastEPGDate(void) const
{
  return GetEPGDate(EPG_LAST_DATE);
}

int CPVRChannelGroup::Size(void) const
{
  return m_members.size();
}

int CPVRChannelGroup::GroupID(void) const
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

int CPVRChannelGroup::GroupType(void) const
{
  return m_iGroupType;
}

std::string CPVRChannelGroup::GroupName(void) const
{
  CSingleLock lock(m_critSection);
  std::string strReturn(m_strGroupName);
  return strReturn;
}

time_t CPVRChannelGroup::LastWatched(void) const
{
  CSingleLock lock(m_critSection);
  return m_iLastWatched;
}

bool CPVRChannelGroup::SetLastWatched(time_t iLastWatched)
{
  CSingleLock lock(m_critSection);

  if (m_iLastWatched != iLastWatched)
  {
    /* update last watched  */
    m_iLastWatched = iLastWatched;
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

bool CPVRChannelGroup::PreventSortAndRenumber(void) const
{
  CSingleLock lock(m_critSection);
  return m_bPreventSortAndRenumber;
}

void CPVRChannelGroup::SetPreventSortAndRenumber(bool bPreventSortAndRenumber /* = true */)
{
  CSingleLock lock(m_critSection);
  m_bPreventSortAndRenumber = bPreventSortAndRenumber;
}

bool CPVRChannelGroup::UpdateChannel(const CFileItem &item, bool bHidden, bool bVirtual, bool bEPGEnabled, bool bParentalLocked, int iEPGSource, int iChannelNumber, const std::string &strChannelName, const std::string &strIconPath, const std::string &strStreamURL, bool bUserSetIcon)
{
  if (!item.HasPVRChannelInfoTag())
    return false;

  CSingleLock lock(m_critSection);

  /* get the real channel from the group */
  CPVRChannelPtr channel = GetByUniqueID(item.GetPVRChannelInfoTag()->UniqueID());
  if (!channel)
    return false;

  channel->SetChannelName(strChannelName, true);
  channel->SetHidden(bHidden);
  channel->SetLocked(bParentalLocked);
  channel->SetIconPath(strIconPath, bUserSetIcon);

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

int CPVRChannelGroup::GetNumChannels() const
{
  return m_members.size();
}

bool CPVRChannelGroup::HasChannels() const
{
  return !m_members.empty();
}

bool CPVRChannelGroup::ToggleChannelLocked(const CFileItem &item)
{
  if (!item.HasPVRChannelInfoTag())
    return false;

  CSingleLock lock(m_critSection);

  /* get the real channel from the group */
  CPVRChannelPtr channel = GetByUniqueID(item.GetPVRChannelInfoTag()->UniqueID());
  if (!channel)
    return false;

  channel->SetLocked(!channel->IsLocked());

  return true;
}

void CPVRChannelGroup::SetSelectedGroup(bool bSetTo)
{
  CSingleLock lock(m_critSection);
  m_bSelectedGroup = bSetTo;
}

bool CPVRChannelGroup::IsSelectedGroup(void) const
{
  CSingleLock lock(m_critSection);
  return m_bSelectedGroup;
}

bool CPVRChannelGroup::CreateChannelEpgs(bool bForce /* = false */)
{
  /* used only by internal channel groups */
  return true;
}
