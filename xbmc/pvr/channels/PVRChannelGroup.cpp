/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

//! @todo use Observable here, so we can use event driven operations later

#include "PVRChannelGroup.h"

#include <algorithm>

#include "ServiceBroker.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include "pvr/PVRDatabase.h"
#include "pvr/PVRGUIProgressHandler.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/epg/EpgChannelData.h"
#include "pvr/epg/EpgContainer.h"

using namespace PVR;

CPVRChannelGroup::CPVRChannelGroup()
{
  OnInit();
}

CPVRChannelGroup::CPVRChannelGroup(bool bRadio,
                                   int iGroupId,
                                   const std::string& strGroupName,
                                   const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup) :
    m_bRadio(bRadio),
    m_iGroupId(iGroupId),
    m_strGroupName(strGroupName),
    m_allChannelsGroup(allChannelsGroup)
{
  OnInit();
}

CPVRChannelGroup::CPVRChannelGroup(const PVR_CHANNEL_GROUP& group,
                                   const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup) :
    m_bRadio(group.bIsRadio),
    m_strGroupName(group.strGroupName),
    m_iPosition(group.iPosition),
    m_allChannelsGroup(allChannelsGroup)
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
  m_bPreventSortAndRenumber     = group.m_bPreventSortAndRenumber;
  m_members                     = group.m_members;
  m_sortedMembers               = group.m_sortedMembers;
  m_iPosition                   = group.m_iPosition;
  m_failedClientsForChannels    = group.m_failedClientsForChannels;
  m_failedClientsForChannelGroupMembers = group.m_failedClientsForChannelGroupMembers;
  m_allChannelsGroup = group.m_allChannelsGroup;

  OnInit();
}

CPVRChannelGroup::~CPVRChannelGroup(void)
{
  CServiceBroker::GetSettingsComponent()->GetSettings()->UnregisterCallback(this);
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

PVRChannelGroupMember CPVRChannelGroup::EmptyMember;

void CPVRChannelGroup::OnInit(void)
{
  CServiceBroker::GetSettingsComponent()->GetSettings()->RegisterCallback(this, {
    CSettings::SETTING_PVRMANAGER_BACKENDCHANNELORDER,
    CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS
  });
}

bool CPVRChannelGroup::Load(std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove)
{
  /* make sure this container is empty before loading */
  Unload();

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_bUsingBackendChannelOrder   = settings->GetBool(CSettings::SETTING_PVRMANAGER_BACKENDCHANNELORDER);
  m_bUsingBackendChannelNumbers = settings->GetBool(CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS);

  int iChannelCount = m_iGroupId > 0 ? LoadFromDb() : 0;
  CLog::LogFC(LOGDEBUG, LOGPVR, "%d channels loaded from the database for group '%s'", iChannelCount, m_strGroupName.c_str());

  if (!Update(channelsToRemove))
  {
    CLog::LogF(LOGERROR, "Failed to update channels for group '%s', m_strGroupName.c_str()");
    return false;
  }

  if (Size() - iChannelCount > 0)
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "%d channels added from clients to group '%s'",
                static_cast<int>(Size() - iChannelCount), m_strGroupName.c_str());
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
  m_failedClientsForChannels.clear();
  m_failedClientsForChannelGroupMembers.clear();
}

bool CPVRChannelGroup::Update(std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove)
{
  if (GroupType() == PVR_GROUP_TYPE_USER_DEFINED ||
      !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRMANAGER_SYNCCHANNELGROUPS))
    return true;

  CPVRChannelGroup PVRChannels_tmp(m_bRadio, m_iGroupId, m_strGroupName, m_allChannelsGroup);
  PVRChannels_tmp.SetPreventSortAndRenumber();
  PVRChannels_tmp.LoadFromClients();
  m_failedClientsForChannelGroupMembers = PVRChannels_tmp.m_failedClientsForChannelGroupMembers;
  return UpdateGroupEntries(PVRChannels_tmp, channelsToRemove);
}

std::string CPVRChannelGroup::GetPath() const
{
  return StringUtils::Format("pvr://channels/%s/%s/", m_bRadio ? "radio" : "tv", GroupName().c_str());
}

bool CPVRChannelGroup::SetChannelNumber(const CPVRChannelPtr &channel, const CPVRChannelNumber &channelNumber)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end(); ++it)
  {
    PVRChannelGroupMember& member(*it);
    if (*member.channel == *channel)
    {
      if (member.channelNumber  != channelNumber)
      {
        m_bChanged = true;
        bReturn = true;
        member.channelNumber = channelNumber;
      }
      break;
    }
  }

  return bReturn;
}

void CPVRChannelGroup::SearchAndSetChannelIcons(bool bUpdateDb /* = false */)
{
  std::string iconPath = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_PVRMENU_ICONPATH);
  if (iconPath.empty())
    return;

  /* fetch files in icon path for fast lookup */
  CFileItemList fileItemList;
  XFILE::CDirectory::GetDirectory(iconPath, fileItemList, ".jpg|.png|.tbn", XFILE::DIR_FLAG_DEFAULTS);

  if (fileItemList.IsEmpty())
    return;

  CSingleLock lock(m_critSection);

  /* create a map for fast lookup of normalized file base name */
  std::map<std::string, std::string> fileItemMap;
  for (const auto& item : fileItemList)
  {
    std::string baseName = URIUtils::GetFileName(item->GetPath());
    URIUtils::RemoveExtension(baseName);
    StringUtils::ToLower(baseName);
    fileItemMap.insert(std::make_pair(baseName, item->GetPath()));
  }

  CPVRGUIProgressHandler* progressHandler = new CPVRGUIProgressHandler(g_localizeStrings.Get(19286)); // Searching for channel icons

  int channelIndex = 0;
  CPVRChannelPtr channel;
  for(PVR_CHANNEL_GROUP_MEMBERS::const_iterator it = m_members.begin(); it != m_members.end(); ++it)
  {
    channel = it->second.channel;

    /* update progress dialog */
    progressHandler->UpdateProgress(channel->ChannelName(), channelIndex++, m_members.size());

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
      channel->SetIconPath(itItem->second, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bPVRAutoScanIconsUserSet);
    }

    if (bUpdateDb)
      channel->Persist();

    //! @todo start channel icon scraper here if nothing was found
  }

  progressHandler->DestroyProgress();
}

/********** sort methods **********/

struct sortByClientChannelNumber
{
  bool operator()(const PVRChannelGroupMember &channel1, const PVRChannelGroupMember &channel2) const
  {
    if (channel1.iClientPriority == channel2.iClientPriority)
    {
      if (channel1.channel->ClientChannelNumber() == channel2.channel->ClientChannelNumber())
        return channel1.channel->ChannelName() < channel2.channel->ChannelName();

      return channel1.channel->ClientChannelNumber() < channel2.channel->ClientChannelNumber();
    }
    return channel1.iClientPriority > channel2.iClientPriority;
  }
};

struct sortByChannelNumber
{
  bool operator()(const PVRChannelGroupMember &channel1, const PVRChannelGroupMember &channel2) const
  {
    return channel1.channelNumber < channel2.channelNumber;
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

bool CPVRChannelGroup::UpdateClientPriorities()
{
  const CPVRClientsPtr clients = CServiceBroker::GetPVRManager().Clients();
  bool bChanged = false;

  CSingleLock lock(m_critSection);

  for (auto& member : m_sortedMembers)
  {
    int iNewPriority = 0;

    if (m_bUsingBackendChannelOrder)
    {
      CPVRClientPtr client;
      if (!clients->GetCreatedClient(member.channel->ClientID(), client))
        continue;

      iNewPriority = client->GetPriority();
    }
    else
    {
      iNewPriority = 0;
    }

    bChanged |= (member.iClientPriority != iNewPriority);
    member.iClientPriority = iNewPriority;
  }

  return bChanged;
}

/********** getters **********/
PVRChannelGroupMember& CPVRChannelGroup::GetByUniqueID(const std::pair<int, int>& id)
{
  CSingleLock lock(m_critSection);
  const auto it = m_members.find(id);
  return it != m_members.end() ? it->second : CPVRChannelGroup::EmptyMember;
}

const PVRChannelGroupMember& CPVRChannelGroup::GetByUniqueID(const std::pair<int, int>& id) const
{
  CSingleLock lock(m_critSection);
  const auto it = m_members.find(id);
  return it != m_members.end() ? it->second : CPVRChannelGroup::EmptyMember;
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
        CServiceBroker::GetPVRManager().Clients()->IsCreatedClient(channel->ClientID()) &&
        channel->LastWatched() > 0 &&
        (!returnChannel || channel->LastWatched() > returnChannel->LastWatched()))
    {
      returnChannel = channel;
    }
  }

  return CFileItemPtr(returnChannel ? new CFileItem(returnChannel) : nullptr);
}

CPVRChannelNumber CPVRChannelGroup::GetChannelNumber(const CPVRChannelPtr &channel) const
{
  CSingleLock lock(m_critSection);
  const PVRChannelGroupMember& member(GetByUniqueID(channel->StorageId()));
  return member.channelNumber;
}

CFileItemPtr CPVRChannelGroup::GetByChannelNumber(const CPVRChannelNumber &channelNumber) const
{
  CFileItemPtr retval;
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::const_iterator it = m_sortedMembers.begin(); !retval && it != m_sortedMembers.end(); ++it)
  {
    if ((*it).channelNumber == channelNumber)
      retval = CFileItemPtr(new CFileItem((*it).channel));
  }

  return retval;
}

CFileItemPtr CPVRChannelGroup::GetNextChannel(const CPVRChannelPtr &channel) const
{
  CFileItemPtr retval;

  if (channel)
  {
    CSingleLock lock(m_critSection);
    for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::const_iterator it = m_sortedMembers.begin(); !retval && it != m_sortedMembers.end(); ++it)
    {
      if ((*it).channel == channel)
      {
        do
        {
          if ((++it) == m_sortedMembers.end())
            it = m_sortedMembers.begin();
          if ((*it).channel && !(*it).channel->IsHidden())
            retval = std::make_shared<CFileItem>((*it).channel);
        } while (!retval && (*it).channel != channel);

        if (!retval)
          retval = std::make_shared<CFileItem>();
      }
    }
  }

  return retval;
}

CFileItemPtr CPVRChannelGroup::GetPreviousChannel(const CPVRChannelPtr &channel) const
{
  CFileItemPtr retval;

  if (channel)
  {
    CSingleLock lock(m_critSection);
    for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::const_reverse_iterator it = m_sortedMembers.rbegin(); !retval && it != m_sortedMembers.rend(); ++it)
    {
      if ((*it).channel == channel)
      {
        do
        {
          if ((++it) == m_sortedMembers.rend())
            it = m_sortedMembers.rbegin();
          if ((*it).channel && !(*it).channel->IsHidden())
            retval = std::make_shared<CFileItem>((*it).channel);
        } while (!retval && (*it).channel != channel);

        if (!retval)
          retval = std::make_shared<CFileItem>();
      }
    }
  }
  return retval;
}

std::vector<PVRChannelGroupMember> CPVRChannelGroup::GetMembers(Include eFilter /* = Include::ALL */) const
{
  CSingleLock lock(m_critSection);
  if (eFilter == Include::ALL)
    return m_sortedMembers;

  std::vector<PVRChannelGroupMember> members;
  for (const auto& member : m_sortedMembers)
  {
    switch (eFilter)
    {
      case Include::ONLY_HIDDEN:
        if (!member.channel->IsHidden())
          continue;
        break;
      case Include::ONLY_VISIBLE:
        if (member.channel->IsHidden())
          continue;
       break;
      default:
        break;
    }

    members.emplace_back(member);
  }

  return members;
}

void CPVRChannelGroup::GetChannelNumbers(std::vector<std::string>& channelNumbers) const
{
  CSingleLock lock(m_critSection);
  for (const auto& member : m_sortedMembers)
    channelNumbers.emplace_back(member.channelNumber.FormattedChannelNumber());
}

int CPVRChannelGroup::LoadFromDb(bool bCompress /* = false */)
{
  const CPVRDatabasePtr database(CServiceBroker::GetPVRManager().GetTVDatabase());
  if (!database)
    return -1;

  int iChannelCount = Size();

  database->Get(*this, *m_allChannelsGroup);

  return Size() - iChannelCount;
}

bool CPVRChannelGroup::LoadFromClients(void)
{
  /* get the channels from the backends */
  return CServiceBroker::GetPVRManager().Clients()->GetChannelGroupMembers(this, m_failedClientsForChannelGroupMembers) == PVR_ERROR_NO_ERROR;
}

bool CPVRChannelGroup::AddAndUpdateChannels(const CPVRChannelGroup &channels, bool bUseBackendChannelNumbers)
{
  bool bReturn(false);

  /* go through the channel list and check for new channels.
     channels will only by updated in CPVRChannelGroupInternal to prevent dupe updates */
  for (PVR_CHANNEL_GROUP_MEMBERS::const_iterator it = channels.m_members.begin(); it != channels.m_members.end(); ++it)
  {
    /* check whether this channel is known in the internal group */
    const PVRChannelGroupMember& existingChannel(m_allChannelsGroup->GetByUniqueID(it->first));
    if (!existingChannel.channel)
      continue;

    /* if it's found, add the channel to this group */
    if (!IsGroupMember(existingChannel.channel))
    {
      AddToGroup(existingChannel.channel,
                 bUseBackendChannelNumbers ? it->second.channel->ClientChannelNumber() : CPVRChannelNumber(),
                 bUseBackendChannelNumbers);

      bReturn = true;
      CLog::Log(LOGINFO,"Added %s channel '%s' to group '%s'",
                m_bRadio ? "radio" : "TV", existingChannel.channel->ChannelName().c_str(), GroupName().c_str());
    }
  }

  SortAndRenumber();

  return bReturn;
}

bool CPVRChannelGroup::IsMissingChannelsFromClient(int iClientId) const
{
  return std::find(m_failedClientsForChannels.begin(),
                   m_failedClientsForChannels.end(),
                   iClientId) != m_failedClientsForChannels.end();
}

bool CPVRChannelGroup::IsMissingChannelGroupMembersFromClient(int iClientId) const
{
  return std::find(m_failedClientsForChannelGroupMembers.begin(),
                   m_failedClientsForChannelGroupMembers.end(),
                   iClientId) != m_failedClientsForChannelGroupMembers.end();
}

std::vector<CPVRChannelPtr> CPVRChannelGroup::RemoveDeletedChannels(const CPVRChannelGroup &channels)
{
  std::vector<CPVRChannelPtr> removedChannels;
  CSingleLock lock(m_critSection);

  /* check for deleted channels */
  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end();)
  {
    const CPVRChannelPtr channel = (*it).channel;
    if (channels.m_members.find(channel->StorageId()) == channels.m_members.end())
    {
      /* channel was not found */
      CLog::Log(LOGINFO,"Deleted %s channel '%s' from group '%s'",
                m_bRadio ? "radio" : "TV", channel->ChannelName().c_str(), GroupName().c_str());

      removedChannels.emplace_back(channel);

      m_members.erase(channel->StorageId());
      it = m_sortedMembers.erase(it);
      m_bChanged = true;
    }
    else
    {
      ++it;
    }
  }

  return removedChannels;
}

bool CPVRChannelGroup::UpdateGroupEntries(const CPVRChannelGroup& channels, std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove)
{
  bool bReturn(false);
  bool bChanged(false);
  bool bRemoved(false);

  CSingleLock lock(m_critSection);
  /* sort by client channel number if this is the first time or if SETTING_PVRMANAGER_BACKENDCHANNELORDER is true */
  bool bUseBackendChannelNumbers(m_members.empty() || m_bUsingBackendChannelOrder);

  SetPreventSortAndRenumber(true);
  channelsToRemove = RemoveDeletedChannels(channels);
  bRemoved = !channelsToRemove.empty();
  bChanged = AddAndUpdateChannels(channels, bUseBackendChannelNumbers) || bRemoved;
  SetPreventSortAndRenumber(false);

  bChanged |= UpdateClientPriorities();

  if (bChanged)
  {
    /* renumber to make sure all channels have a channel number.
       new channels were added at the back, so they'll get the highest numbers */
    bool bRenumbered = SortAndRenumber();

    bReturn = Persist();

    SetChanged();

    lock.Leave();
    NotifyObservers(HasNewChannels() || bRemoved || bRenumbered ? ObservableMessageChannelGroupReset : ObservableMessageChannelGroup);
  }
  else
  {
    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannelGroup::RemoveFromGroup(const CPVRChannelPtr &channel)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end();)
  {
    if (*channel == *((*it).channel))
    {
      //! @todo notify observers
      m_members.erase((*it).channel->StorageId());
      it = m_sortedMembers.erase(it);
      bReturn = true;
      m_bChanged = true;
      break;
    }
    else
    {
      ++it;
    }
  }

  // no need to renumber if nothing was removed
  if (bReturn)
    Renumber();

  return bReturn;
}

bool CPVRChannelGroup::AddToGroup(const CPVRChannelPtr &channel, const CPVRChannelNumber &channelNumber, bool bUseBackendChannelNumbers)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  if (!CPVRChannelGroup::IsGroupMember(channel))
  {
    const PVRChannelGroupMember& realChannel(IsInternalGroup() ?
        GetByUniqueID(channel->StorageId()) :
        m_allChannelsGroup->GetByUniqueID(channel->StorageId()));

    if (realChannel.channel)
    {
      unsigned int iChannelNumber = channelNumber.GetChannelNumber();
      if (!channelNumber.IsValid() ||
          (!bUseBackendChannelNumbers && (iChannelNumber > m_members.size() + 1)))
        iChannelNumber = m_members.size() + 1;

      PVRChannelGroupMember newMember(realChannel);
      newMember.channelNumber = CPVRChannelNumber(iChannelNumber, channelNumber.GetSubChannelNumber());
      m_sortedMembers.push_back(newMember);
      m_members.insert(std::make_pair(realChannel.channel->StorageId(), newMember));
      m_bChanged = true;

      SortAndRenumber();

      //! @todo notify observers
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
  const CPVRDatabasePtr database(CServiceBroker::GetPVRManager().GetTVDatabase());

  CSingleLock lock(m_critSection);

  /* only persist if the group has changes and is fully loaded or never has been saved before */
  if (!HasChanges() || (!m_bLoaded && m_iGroupId != INVALID_GROUP_ID))
    return bReturn;

  // Mark newly created groups as loaded so future updates will also be persisted...
  if (m_iGroupId == INVALID_GROUP_ID)
    m_bLoaded = true;

  if (database)
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Persisting channel group '%s' with %d channels",
                GroupName().c_str(), (int) m_members.size());
    m_bChanged = false;

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
  if (PreventSortAndRenumber())
    return true;

  bool bReturn(false);
  unsigned int iChannelNumber(0);
  bool bUseBackendChannelNumbers(CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS) &&
                                 CServiceBroker::GetPVRManager().Clients()->EnabledClientAmount() == 1);

  CSingleLock lock(m_critSection);

  CPVRChannelNumber currentChannelNumber;
  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end(); ++it)
  {
    if ((*it).channel->IsHidden())
    {
      currentChannelNumber = CPVRChannelNumber(0, 0);
    }
    else if (bUseBackendChannelNumbers)
    {
      currentChannelNumber = (*it).channel->ClientChannelNumber();
    }
    else
    {
      if (IsInternalGroup())
        currentChannelNumber = CPVRChannelNumber(++iChannelNumber, 0);
      else
        currentChannelNumber = m_allChannelsGroup->GetChannelNumber((*it).channel);
    }

    if ((*it).channelNumber != currentChannelNumber)
    {
      bReturn = true;
      m_bChanged = true;
      (*it).channelNumber = currentChannelNumber;
    }

    //! @todo This is a quick fix for v18. Whole channel number handling should be reworked - code is imo unmaintainable.
    if (IsInternalGroup())
      (*it).channel->SetChannelNumber((*it).channelNumber);
  }

  SortByChannelNumber();
  return bReturn;
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

void CPVRChannelGroup::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  //! @todo while pvr manager is starting up do accept setting changes.
  if(!CServiceBroker::GetPVRManager().IsStarted())
  {
    CLog::Log(LOGWARNING, "Channel group setting change ignored while PVR Manager is starting\n");
    return;
  }

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_PVRMANAGER_BACKENDCHANNELORDER || settingId == CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS)
  {
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    bool bUsingBackendChannelOrder   = settings->GetBool(CSettings::SETTING_PVRMANAGER_BACKENDCHANNELORDER);
    bool bUsingBackendChannelNumbers = settings->GetBool(CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS);

    CSingleLock lock(m_critSection);

    bool bChannelNumbersChanged = m_bUsingBackendChannelNumbers != bUsingBackendChannelNumbers;
    bool bChannelOrderChanged = m_bUsingBackendChannelOrder != bUsingBackendChannelOrder;

    m_bUsingBackendChannelOrder = bUsingBackendChannelOrder;
    m_bUsingBackendChannelNumbers = bUsingBackendChannelNumbers;

    /* check whether this channel group has to be renumbered */
    if (bChannelOrderChanged || bChannelNumbersChanged)
    {
      CLog::LogFC(LOGDEBUG, LOGPVR, "Renumbering channel group '%s' to use the backend channel order and/or numbers",
                  m_strGroupName.c_str());

      if (bChannelOrderChanged)
        UpdateClientPriorities();

      SortAndRenumber();
      Persist();
    }
  }
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVRChannelGroup::GetEPGAll(bool bIncludeChannelsWithoutEPG /* = false */) const
{
  std::vector<std::shared_ptr<CPVREpgInfoTag>> tags;

  CPVREpgInfoTagPtr epgTag;
  CPVRChannelPtr channel;
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_SORTED_MEMBERS::const_iterator it = m_sortedMembers.begin(); it != m_sortedMembers.end(); ++it)
  {
    channel = (*it).channel;
    if (!channel->IsHidden())
    {
      bool bEmpty = true;

      CPVREpgPtr epg = channel->GetEPG();
      if (epg)
      {
        const std::vector<std::shared_ptr<CPVREpgInfoTag>> epgTags = epg->GetTags();
        bEmpty = epgTags.empty();
        if (!bEmpty)
          tags.insert(tags.end(), epgTags.begin(), epgTags.end());
      }

      if (bIncludeChannelsWithoutEPG && bEmpty)
      {
        // Add dummy EPG tag associated with this channel
        if (epg)
          epgTag = std::make_shared<CPVREpgInfoTag>(epg->GetChannelData(), epg->EpgID());
        else
          epgTag = std::make_shared<CPVREpgInfoTag>(std::make_shared<CPVREpgChannelData>(*channel), -1);

        tags.emplace_back(epgTag);
      }
    }
  }

  return tags;
}

CDateTime CPVRChannelGroup::GetEPGDate(EpgDateType epgDateType) const
{
  CDateTime date;
  CPVREpgPtr epg;
  CPVRChannelPtr channel;
  CSingleLock lock(m_critSection);

  for (PVR_CHANNEL_GROUP_MEMBERS::const_iterator it = m_members.begin(); it != m_members.end(); ++it)
  {
    channel = it->second.channel;
    if (!channel->IsHidden() && (epg = channel->GetEPG()))
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
  const CPVRDatabasePtr database(CServiceBroker::GetPVRManager().GetTVDatabase());

  CSingleLock lock(m_critSection);

  if (m_iLastWatched != iLastWatched)
  {
    m_iLastWatched = iLastWatched;

    /* update the database immediately */
    if (database)
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

bool CPVRChannelGroup::UpdateChannel(const CFileItem &item, bool bHidden, bool bEPGEnabled, bool bParentalLocked, int iEPGSource, int iChannelNumber, const std::string &strChannelName, const std::string &strIconPath, bool bUserSetIcon)
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

  //! @todo add other scrapers
  member.channel->SetEPGEnabled(bEPGEnabled);

  /* set new values in the channel tag */
  if (bHidden)
  {
    SortByChannelNumber(); // or previous changes will be overwritten
    RemoveFromGroup(member.channel);
  }
  else
  {
    SetChannelNumber(member.channel, CPVRChannelNumber(iChannelNumber, 0));
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
