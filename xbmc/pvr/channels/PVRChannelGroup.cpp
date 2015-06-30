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
    m_bUsingBackendChannelNumbers(false),
    m_bSelectedGroup(false),
    m_bPreventSortAndRenumber(false),
    m_iLastWatched(0),
    m_bHidden(false),
    m_iPosition(0)
{
  OnInit();
}

CPVRChannelGroup::CPVRChannelGroup(bool bRadio, unsigned int iGroupId, const std::string &strGroupName) :
    m_bRadio(bRadio),
    m_iGroupType(PVR_GROUP_TYPE_DEFAULT),
    m_iGroupId(iGroupId),
    m_strGroupName(strGroupName),
    m_bLoaded(false),
    m_bChanged(false),
    m_bUsingBackendChannelOrder(false),
    m_bUsingBackendChannelNumbers(false),
    m_bSelectedGroup(false),
    m_bPreventSortAndRenumber(false),
    m_iLastWatched(0),
    m_bHidden(false),
    m_iPosition(0)
{
  OnInit();
}

CPVRChannelGroup::CPVRChannelGroup(const PVR_CHANNEL_GROUP &group) :
    m_bRadio(group.bIsRadio),
    m_iGroupType(PVR_GROUP_TYPE_DEFAULT),
    m_iGroupId(-1),
    m_strGroupName(group.strGroupName),
    m_bLoaded(false),
    m_bChanged(false),
    m_bUsingBackendChannelOrder(false),
    m_bUsingBackendChannelNumbers(false),
    m_bSelectedGroup(false),
    m_bPreventSortAndRenumber(false),
    m_iLastWatched(0),
    m_bHidden(false),
    m_iPosition(group.iPosition)
{
  OnInit();
}

CPVRChannelGroup::CPVRChannelGroup(const CPVRChannelGroup &group) :
    m_strGroupName(group.m_strGroupName)
{
  m_bRadio                      = group.m_bRadio;
  m_iGroupType                  = group.m_iGroupType;
  m_iGroupId                    = group.m_iGroupId;
  m_bLoaded                     = group.m_bLoaded;
  m_bChanged                    = group.m_bChanged;
  m_bUsingBackendChannelOrder   = group.m_bUsingBackendChannelOrder;
  m_bUsingBackendChannelNumbers = group.m_bUsingBackendChannelNumbers;
  m_iLastWatched                = group.m_iLastWatched;
  m_bHidden                     = group.m_bHidden;
  m_bSelectedGroup              = group.m_bSelectedGroup;
  m_bPreventSortAndRenumber     = group.m_bPreventSortAndRenumber;
  m_members                     = group.m_members;
  m_sortedMembers               = group.m_sortedMembers;
  m_iPosition                   = group.m_iPosition;
  OnInit();
}

CPVRChannelGroup::~CPVRChannelGroup(void)
{
  CSettings::Get().UnregisterCallback(this);
  Unload();
}

bool CPVRChannelGroup::operator==(const CPVRChannelGroup& right) const
{
  return (m_bRadio == right.m_bRadio &&
      m_iGroupType == right.m_iGroupType &&
      m_iGroupId == right.m_iGroupId &&
      m_strGroupName == right.m_strGroupName &&
      m_iPosition == right.m_iPosition);
}

bool CPVRChannelGroup::operator!=(const CPVRChannelGroup &right) const
{
  return !(*this == right);
}

PVRChannelGroupMember CPVRChannelGroup::EmptyMember = { CPVRChannelPtr(), 0, 0 };

std::pair<int, int> CPVRChannelGroup::PathIdToStorageId(uint64_t storageId)
{
  return std::make_pair(storageId >> 32, storageId & 0xFFFFFFFF);
}

void CPVRChannelGroup::OnInit(void)
{
  CSettings::Get().RegisterCallback(this, {
    "pvrmanager.backendchannelorder",
    "pvrmanager.usebackendchannelnumbers"
  });
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
        __FUNCTION__, static_cast<int>(Size() - iChannelCount), m_strGroupName.c_str());
  }

  SortAndRenumber();

  m_bLoaded = true;

  return true;
}

void CPVRChannelGroup::Unload(void)
{
  CSingleLock lock(m_critSection);
  m_sortedMembers.clear();
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

bool CPVRChannelGroup::SetChannelNumber(const CPVRChannelPtr &channel, unsigned int iChannelNumber, unsigned int iSubChannelNumber /* = 0 */)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end(); ++it)
  {
    PVRChannelGroupMember& member(*it);
    if (*member.channel == *channel)
    {
      if (member.iChannelNumber    != iChannelNumber ||
          member.iSubChannelNumber != iSubChannelNumber)
      {
        m_bChanged = true;
        bReturn = true;
        member.iChannelNumber    = iChannelNumber;
        member.iSubChannelNumber = iSubChannelNumber;
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
  if (iOldChannelNumber > m_sortedMembers.size())
    return bReturn;

  /* new channel number out of range */
  if (iNewChannelNumber < 1)
    return bReturn;

  if (iNewChannelNumber > m_sortedMembers.size())
    iNewChannelNumber = m_sortedMembers.size();

  /* move the channel in the list */
  PVRChannelGroupMember entry = m_sortedMembers.at(iOldChannelNumber - 1);
  m_sortedMembers.erase(m_sortedMembers.begin() + iOldChannelNumber - 1);
  m_sortedMembers.insert(m_sortedMembers.begin() + iNewChannelNumber - 1, entry);

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
  CPVRChannelPtr channel;
  for(PVR_CHANNEL_GROUP_MEMBERS::const_iterator it = m_members.begin(); it != m_members.end(); ++it)
  {
    channel = it->second.channel;

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
  bool operator()(const PVRChannelGroupMember &channel1, const PVRChannelGroupMember &channel2) const
  {
    if (channel1.channel->ClientChannelNumber() == channel2.channel->ClientChannelNumber())
    {
      if (channel1.channel->ClientSubChannelNumber() > 0 || channel2.channel->ClientSubChannelNumber() > 0)
        return channel1.channel->ClientSubChannelNumber() < channel2.channel->ClientSubChannelNumber();
      else
        return channel1.channel->ChannelName() < channel2.channel->ChannelName();
    }
    return channel1.channel->ClientChannelNumber() < channel2.channel->ClientChannelNumber();
  }
};

struct sortByChannelNumber
{
  bool operator()(const PVRChannelGroupMember &channel1, const PVRChannelGroupMember &channel2) const
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
    sort(m_sortedMembers.begin(), m_sortedMembers.end(), sortByClientChannelNumber());
}

void CPVRChannelGroup::SortByChannelNumber(void)
{
  CSingleLock lock(m_critSection);
  if (!PreventSortAndRenumber())
    sort(m_sortedMembers.begin(), m_sortedMembers.end(), sortByChannelNumber());
}

/********** getters **********/
const PVRChannelGroupMember& CPVRChannelGroup::GetByUniqueID(const std::pair<int, int>& id) const
{
  CSingleLock lock(m_critSection);
  PVR_CHANNEL_GROUP_MEMBERS::const_iterator it = m_members.find(id);
  return it != m_members.end() ?
      it->second :
      CPVRChannelGroup::EmptyMember;
}

CPVRChannelPtr CPVRChannelGroup::GetByUniqueID(int iUniqueChannelId, int iClientID) const
{
  return GetByUniqueID(std::make_pair(iClientID, iUniqueChannelId)).channel;
}

CPVRChannelPtr CPVRChannelGroup::GetByChannelID(int iChannelID) const
{
  CPVRChannelPtr retval;
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_MEMBERS::const_iterator it = m_members.begin(); !retval && it != m_members.end(); ++it)
  {
    if (it->second.channel->ChannelID() == iChannelID)
      retval = it->second.channel;
  }

  return retval;
}

CPVRChannelPtr CPVRChannelGroup::GetByChannelEpgID(int iEpgID) const
{
  CPVRChannelPtr retval;
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_MEMBERS::const_iterator it = m_members.begin(); !retval && it != m_members.end(); ++it)
  {
    if (it->second.channel->EpgID() == iEpgID)
      retval = it->second.channel;
  }

  return retval;
}

CFileItemPtr CPVRChannelGroup::GetLastPlayedChannel(int iCurrentChannel /* = -1 */) const
{
  CSingleLock lock(m_critSection);

  CPVRChannelPtr returnChannel, channel;
  for (PVR_CHANNEL_GROUP_MEMBERS::const_iterator it = m_members.begin(); it != m_members.end(); ++it)
  {
    channel = it->second.channel;
    if (channel->ChannelID() != iCurrentChannel &&
        g_PVRClients->IsConnectedClient(channel->ClientID()) &&
        channel->LastWatched() > 0 &&
        (!returnChannel || channel->LastWatched() > returnChannel->LastWatched()))
    {
      returnChannel = channel;
    }
  }

  CFileItemPtr retVal = CFileItemPtr(returnChannel ? new CFileItem(returnChannel) : new CFileItem);
  return retVal;
}

unsigned int CPVRChannelGroup::GetSubChannelNumber(const CPVRChannelPtr &channel) const
{
  CSingleLock lock(m_critSection);
  const PVRChannelGroupMember& member(GetByUniqueID(channel->StorageId()));
  return member.iSubChannelNumber;
}

unsigned int CPVRChannelGroup::GetChannelNumber(const CPVRChannelPtr &channel) const
{
  CSingleLock lock(m_critSection);
  const PVRChannelGroupMember& member(GetByUniqueID(channel->StorageId()));
  return member.iChannelNumber;
}

CFileItemPtr CPVRChannelGroup::GetByChannelNumber(unsigned int iChannelNumber, unsigned int iSubChannelNumber /* = 0 */) const
{
  CFileItemPtr retval;
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_MEMBERS::const_iterator it = m_members.begin(); !retval && it != m_members.end(); ++it)
  {
    if (it->second.iChannelNumber == iChannelNumber &&
        (iSubChannelNumber == 0 || iSubChannelNumber == it->second.iSubChannelNumber))
      retval = CFileItemPtr(new CFileItem(it->second.channel));
  }

  if (!retval)
    retval = CFileItemPtr(new CFileItem);
  return retval;
}

CFileItemPtr CPVRChannelGroup::GetByChannelUp(const CFileItem &channel) const
{
  CFileItemPtr retval;

  if (channel.HasPVRChannelInfoTag())
  {
    CSingleLock lock(m_critSection);
    for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::const_iterator it = m_sortedMembers.begin(); !retval && it != m_sortedMembers.end(); ++it)
    {
      if ((*it).channel == channel.GetPVRChannelInfoTag())
      {
        do
        {
          if ((++it) == m_sortedMembers.end())
            it = m_sortedMembers.begin();
          if ((*it).channel && !(*it).channel->IsHidden())
            retval = CFileItemPtr(new CFileItem((*it).channel));
        } while (!retval && (*it).channel != channel.GetPVRChannelInfoTag());

        if (!retval)
          retval = CFileItemPtr(new CFileItem);
      }
    }
  }

  return retval;
}

CFileItemPtr CPVRChannelGroup::GetByChannelDown(const CFileItem &channel) const
{
  CFileItemPtr retval;

  if (channel.HasPVRChannelInfoTag())
  {
    CSingleLock lock(m_critSection);
    for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::const_reverse_iterator it = m_sortedMembers.rbegin(); !retval && it != m_sortedMembers.rend(); ++it)
    {
      if ((*it).channel == channel.GetPVRChannelInfoTag())
      {
        do
        {
          if ((++it) == m_sortedMembers.rend())
            it = m_sortedMembers.rbegin();
          if ((*it).channel && !(*it).channel->IsHidden())
            retval = CFileItemPtr(new CFileItem((*it).channel));
        } while (!retval && (*it).channel != channel.GetPVRChannelInfoTag());

        if (!retval)
          retval = CFileItemPtr(new CFileItem);
      }
    }
  }
  return retval;
}

PVR_CHANNEL_GROUP_SORTED_MEMBERS CPVRChannelGroup::GetMembers(void) const
{
  CSingleLock lock(m_critSection);
  return m_sortedMembers;
}

int CPVRChannelGroup::GetMembers(CFileItemList &results, bool bGroupMembers /* = true */) const
{
  int iOrigSize = results.Size();
  CSingleLock lock(m_critSection);

  const CPVRChannelGroup* channels = bGroupMembers ? this : g_PVRChannelGroups->GetGroupAll(m_bRadio).get();
  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::const_iterator it = channels->m_sortedMembers.begin(); it != channels->m_sortedMembers.end(); ++it)
  {
    if (bGroupMembers || !IsGroupMember((*it).channel))
    {
      CFileItemPtr pFileItem(new CFileItem((*it).channel));
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
  for (PVR_CHANNEL_GROUP_MEMBERS::const_iterator it = channels.m_members.begin(); it != channels.m_members.end(); ++it)
  {
    /* check whether this channel is known in the internal group */
    const PVRChannelGroupMember& existingChannel(g_PVRChannelGroups->GetGroupAll(m_bRadio)->GetByUniqueID(it->first));
    if (!existingChannel.channel)
      continue;

    /* if it's found, add the channel to this group */
    if (!IsGroupMember(existingChannel.channel))
    {
      int iChannelNumber = bUseBackendChannelNumbers ? it->second.channel->ClientChannelNumber() : 0;
      AddToGroup(existingChannel.channel, iChannelNumber);

      bReturn = true;
      CLog::Log(LOGINFO,"PVRChannelGroup - %s - added %s channel '%s' at position %d in group '%s'",
          __FUNCTION__, m_bRadio ? "radio" : "TV", existingChannel.channel->ChannelName().c_str(), iChannelNumber, GroupName().c_str());
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
  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end();)
  {
    if (channels.m_members.find((*it).channel->StorageId()) == channels.m_members.end())
    {
      /* channel was not found */
      CLog::Log(LOGINFO,"PVRChannelGroup - %s - deleted %s channel '%s' from group '%s'",
          __FUNCTION__, m_bRadio ? "radio" : "TV", (*it).channel->ChannelName().c_str(), GroupName().c_str());

      m_members.erase((*it).channel->StorageId());

      //we need a copy of our iterators data so that we can find it later on
      //if the vector has changed.
      auto group = *it;
      /* remove this channel from all non-system groups if this is the internal group */
      if (IsInternalGroup())
      {
        g_PVRChannelGroups->Get(m_bRadio)->RemoveFromAllGroups((*it).channel);

        /* since it was not found in the internal group, it was deleted from the backend */
        group.channel->Delete();
      }

      //our vector can have been modified during the call to RemoveFromAllGroups
      //make no assumption and search for the value to be removed
      auto possiblyRemovedGroup = std::find_if(m_sortedMembers.begin(), m_sortedMembers.end(), [&group](const PVRChannelGroupMember& it)
      {
        return  group.channel == it.channel &&
                group.iChannelNumber == it.iChannelNumber &&
                group.iSubChannelNumber == it.iSubChannelNumber;
      });

      if (possiblyRemovedGroup != m_sortedMembers.end())
        m_sortedMembers.erase(possiblyRemovedGroup);
      
      //We have to start over from the beginning, list can have been modified and
      //resorted, there's no safe way to continue where we left of
      it = m_sortedMembers.begin();
      m_bChanged = true;
      bReturn = true;
    }
    else
    {
      ++it;
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
  bool bUseBackendChannelNumbers(m_members.empty() || m_bUsingBackendChannelOrder);

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
  CPVRChannelPtr channel;
  CSingleLock lock(m_critSection);
  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end();)
  {
    bool bDelete = false;
    channel = (*it).channel;

    if (channel->ClientChannelNumber() <= 0)
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
        g_PVRChannelGroups->Get(m_bRadio)->RemoveFromAllGroups(channel);
        channel->Delete();
      }
      else
      {
        m_members.erase(channel->StorageId());
        it = m_sortedMembers.erase(it);
      }
      m_bChanged = true;
    }
    else
    {
      ++it;
    }
  }
}

bool CPVRChannelGroup::RemoveFromGroup(const CPVRChannelPtr &channel)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end();)
  {
    if (*channel == *((*it).channel))
    {
      // TODO notify observers
      m_members.erase((*it).channel->StorageId());
      it = m_sortedMembers.erase(it);
      bReturn = true;
      m_bChanged = true;
      break;
    }
  }

  Renumber();

  return bReturn;
}

bool CPVRChannelGroup::AddToGroup(const CPVRChannelPtr &channel, int iChannelNumber /* = 0 */)
{
  CSingleLock lock(m_critSection);

  bool bReturn(false);

  if (!CPVRChannelGroup::IsGroupMember(channel))
  {
    if (iChannelNumber <= 0 || iChannelNumber > (int) m_members.size() + 1)
      iChannelNumber = m_members.size() + 1;

    const PVRChannelGroupMember& realChannel(IsInternalGroup() ?
        GetByUniqueID(channel->StorageId()) :
        g_PVRChannelGroups->GetGroupAll(m_bRadio)->GetByUniqueID(channel->StorageId()));

    if (realChannel.channel)
    {
      PVRChannelGroupMember newMember(realChannel);
      newMember.iChannelNumber = (unsigned int)iChannelNumber;
      m_sortedMembers.push_back(newMember);
      m_members.insert(std::make_pair(realChannel.channel->StorageId(), newMember));
      m_bChanged = true;

      SortAndRenumber();

      // TODO notify observers
      bReturn = true;
    }
  }

  return bReturn;
}

bool CPVRChannelGroup::IsGroupMember(const CPVRChannelPtr &channel) const
{
  CSingleLock lock(m_critSection);
  return m_members.find(channel->StorageId()) != m_members.end();
}

bool CPVRChannelGroup::IsGroupMember(int iChannelId) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_MEMBERS::const_iterator it = m_members.begin(); !bReturn && it != m_members.end(); ++it)
    bReturn = (iChannelId == it->second.channel->ChannelID());

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

  /* don't persist until the group is fully loaded and has changes */
  if (!HasChanges() || !m_bLoaded)
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

  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end(); ++it)
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

  /* set all channel numbers on members of this group */
  CPVRChannelPtr channel;
  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end(); ++it)
  {
    channel = (*it).channel;
    channel->SetCachedChannelNumber((*it).iChannelNumber);
    channel->SetCachedSubChannelNumber((*it).iSubChannelNumber);
  }
}

bool CPVRChannelGroup::HasChangedChannels(void) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_MEMBERS::const_iterator it = m_members.begin(); !bReturn && it != m_members.end(); ++it)
    bReturn = it->second.channel->IsChanged();

  return bReturn;
}

bool CPVRChannelGroup::HasNewChannels(void) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_MEMBERS::const_iterator it = m_members.begin(); !bReturn && it != m_members.end(); ++it)
    bReturn = it->second.channel->ChannelID() <= 0;

  return bReturn;
}

bool CPVRChannelGroup::HasChanges(void) const
{
  CSingleLock lock(m_critSection);
  return m_bChanged || HasNewChannels() || HasChangedChannels();
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

int CPVRChannelGroup::GetEPGNowOrNext(CFileItemList &results, bool bGetNext) const
{
  int iInitialSize = results.Size();
  CEpg* epg;
  CEpgInfoTagPtr epgNext;
  CPVRChannelPtr channel;
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::const_iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end(); ++it)
  {
    channel = (*it).channel;
    epg = channel->GetEPG();
    if (epg && !channel->IsHidden())
    {
      epgNext = bGetNext ? epg->GetTagNext() : epg->GetTagNow();
      if (epgNext)
      {
        CFileItemPtr entry(new CFileItem(epgNext));
        entry->SetLabel2(epgNext->StartAsLocalTime().GetAsLocalizedTime("", false));
        entry->SetPath(channel->Path());
        entry->SetArt("thumb", channel->IconPath());
        results.Add(entry);
      }
    }
  }

  return results.Size() - iInitialSize;
}

int CPVRChannelGroup::GetEPGAll(CFileItemList &results) const
{
  int iInitialSize = results.Size();
  CEpg* epg;
  CPVRChannelPtr channel;
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::const_iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end(); ++it)
  {
    channel = (*it).channel;
    if (!channel->IsHidden() && (epg = channel->GetEPG()) != NULL)
    {
      // XXX channel pointers aren't set in some occasions. this works around the issue, but is not very nice
      epg->SetChannel(channel);
      epg->Get(results);
    }
  }

  return results.Size() - iInitialSize;
}

CDateTime CPVRChannelGroup::GetEPGDate(EpgDateType epgDateType) const
{
  CDateTime date;
  CEpg* epg;
  CPVRChannelPtr channel;
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_MEMBERS::const_iterator it = m_members.begin(); it != m_members.end(); ++it)
  {
    channel = it->second.channel;
    if (!channel->IsHidden() && (epg = channel->GetEPG()) != NULL)
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

CDateTime CPVRChannelGroup::GetFirstEPGDate(void) const
{
  return GetEPGDate(EPG_FIRST_DATE);
}

CDateTime CPVRChannelGroup::GetLastEPGDate(void) const
{
  return GetEPGDate(EPG_LAST_DATE);
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
    m_iLastWatched = iLastWatched;
    lock.Leave();

    /* update the database immediately */
    if (CPVRDatabase *database = GetPVRDatabase())
      return database->UpdateLastWatched(*this);
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

bool CPVRChannelGroup::UpdateChannel(const CFileItem &item, bool bHidden, bool bEPGEnabled, bool bParentalLocked, int iEPGSource, int iChannelNumber, const std::string &strChannelName, const std::string &strIconPath, const std::string &strStreamURL, bool bUserSetIcon)
{
  if (!item.HasPVRChannelInfoTag())
    return false;

  CSingleLock lock(m_critSection);

  /* get the real channel from the group */
  const PVRChannelGroupMember& member(GetByUniqueID(item.GetPVRChannelInfoTag()->StorageId()));
  if (!member.channel)
    return false;

  member.channel->SetChannelName(strChannelName, true);
  member.channel->SetHidden(bHidden);
  member.channel->SetLocked(bParentalLocked);
  member.channel->SetIconPath(strIconPath, bUserSetIcon);

  if (iEPGSource == 0)
    member.channel->SetEPGScraper("client");

  // TODO add other scrapers
  member.channel->SetEPGEnabled(bEPGEnabled);

  /* set new values in the channel tag */
  if (bHidden)
  {
    SortByChannelNumber(); // or previous changes will be overwritten
    RemoveFromGroup(member.channel);
  }
  else
  {
    SetChannelNumber(member.channel, iChannelNumber);
  }

  return true;
}

size_t CPVRChannelGroup::Size(void) const
{
  CSingleLock lock(m_critSection);
  return m_members.size();
}

bool CPVRChannelGroup::HasChannels() const
{
  CSingleLock lock(m_critSection);
  return !m_members.empty();
}

bool CPVRChannelGroup::ToggleChannelLocked(const CFileItem &item)
{
  if (!item.HasPVRChannelInfoTag())
    return false;

  CSingleLock lock(m_critSection);

  /* get the real channel from the group */
  const PVRChannelGroupMember& member(GetByUniqueID(item.GetPVRChannelInfoTag()->StorageId()));
  if (member.channel)
  {
    member.channel->SetLocked(!member.channel->IsLocked());
    return true;
  }

  return false;
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

void CPVRChannelGroup::SetHidden(bool bHidden)
{
  CSingleLock lock(m_critSection);

  if (m_bHidden != bHidden)
  {
    m_bHidden = bHidden;
    m_bChanged = true;
  }
}

bool CPVRChannelGroup::IsHidden(void) const
{
  CSingleLock lock(m_critSection);
  return m_bHidden;
}

int CPVRChannelGroup::GetPosition(void) const
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
