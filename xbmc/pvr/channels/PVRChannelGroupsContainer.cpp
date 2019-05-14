/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupsContainer.h"

#include "FileItem.h"
#include "utils/log.h"

#include "pvr/epg/EpgInfoTag.h"

using namespace PVR;

CPVRChannelGroupsContainer::CPVRChannelGroupsContainer(void) :
    m_groupsRadio(new CPVRChannelGroups(true)),
    m_groupsTV(new CPVRChannelGroups(false))
{
}

CPVRChannelGroupsContainer::~CPVRChannelGroupsContainer(void)
{
  delete m_groupsRadio;
  delete m_groupsTV;
}

bool CPVRChannelGroupsContainer::Update(bool bChannelsOnly /* = false */)
{
  CSingleLock lock(m_critSection);
  if (m_bIsUpdating)
    return false;
  m_bIsUpdating = true;
  m_bUpdateChannelsOnly = bChannelsOnly;
  lock.Leave();

  CLog::LogFC(LOGDEBUG, LOGPVR, "Updating %s", bChannelsOnly ? "channels" : "channel groups");
  bool bReturn = m_groupsTV->Update(bChannelsOnly) && m_groupsRadio->Update(bChannelsOnly);

  lock.Enter();
  m_bIsUpdating = false;
  lock.Leave();

  return bReturn;
}

bool CPVRChannelGroupsContainer::Load(void)
{
  Unload();
  m_bLoaded = m_groupsTV->Load() && m_groupsRadio->Load();
  return m_bLoaded;
}

bool CPVRChannelGroupsContainer::Loaded(void) const
{
  return m_bLoaded;
}

void CPVRChannelGroupsContainer::Unload(void)
{
  m_groupsRadio->Clear();
  m_groupsTV->Clear();
  m_bLoaded = false;
}

CPVRChannelGroups *CPVRChannelGroupsContainer::Get(bool bRadio) const
{
  return bRadio ? m_groupsRadio : m_groupsTV;
}

CPVRChannelGroupPtr CPVRChannelGroupsContainer::GetGroupAll(bool bRadio) const
{
  return Get(bRadio)->GetGroupAll();
}

CPVRChannelGroupPtr CPVRChannelGroupsContainer::GetByIdFromAll(int iGroupId) const
{
  CPVRChannelGroupPtr group = m_groupsTV->GetById(iGroupId);
  if (!group)
    group = m_groupsRadio->GetById(iGroupId);

  return group;
}

CPVRChannelPtr CPVRChannelGroupsContainer::GetChannelById(int iChannelId) const
{
  CPVRChannelPtr channel = m_groupsTV->GetGroupAll()->GetByChannelID(iChannelId);
  if (!channel)
    channel = m_groupsRadio->GetGroupAll()->GetByChannelID(iChannelId);

  return channel;
}

CPVRChannelPtr CPVRChannelGroupsContainer::GetChannelByEpgId(int iEpgId) const
{
  CPVRChannelPtr channel = m_groupsTV->GetGroupAll()->GetByChannelEpgID(iEpgId);
  if (!channel)
    channel = m_groupsRadio->GetGroupAll()->GetByChannelEpgID(iEpgId);

  return channel;
}

std::shared_ptr<CPVRChannel> CPVRChannelGroupsContainer::GetChannelForEpgTag(const std::shared_ptr<CPVREpgInfoTag>& epgTag) const
{
  if (!epgTag)
    return {};

  const CPVRChannelGroups* groups = epgTag->IsRadio() ? m_groupsRadio : m_groupsTV;
  return groups->GetGroupAll()->GetByUniqueID(epgTag->UniqueChannelID(), epgTag->ClientID());
}

CFileItemPtr CPVRChannelGroupsContainer::GetByPath(const std::string &strPath) const
{
  for (unsigned int bRadio = 0; bRadio <= 1; ++bRadio)
  {
    const CPVRChannelGroups *groups = Get(bRadio == 1);
    CFileItemPtr retVal = groups->GetByPath(strPath);
    if (retVal && retVal->HasPVRChannelInfoTag())
      return retVal;
  }

  CFileItemPtr retVal(new CFileItem);
  return retVal;
}

CPVRChannelGroupPtr CPVRChannelGroupsContainer::GetSelectedGroup(bool bRadio) const
{
  return Get(bRadio)->GetSelectedGroup();
}

CPVRChannelPtr CPVRChannelGroupsContainer::GetByUniqueID(int iUniqueChannelId, int iClientID) const
{
  CPVRChannelPtr channel;
  CPVRChannelGroupPtr channelgroup = GetGroupAllTV();
  if (channelgroup)
    channel = channelgroup->GetByUniqueID(iUniqueChannelId, iClientID);

  if (!channelgroup || !channel)
    channelgroup = GetGroupAllRadio();
  if (channelgroup)
    channel = channelgroup->GetByUniqueID(iUniqueChannelId, iClientID);

  return channel;
}

void CPVRChannelGroupsContainer::SearchMissingChannelIcons(void) const
{
  CLog::Log(LOGINFO, "Starting PVR channel icon search");

  CPVRChannelGroupPtr channelgrouptv  = GetGroupAllTV();
  CPVRChannelGroupPtr channelgroupradio = GetGroupAllRadio();

  if (channelgrouptv)
    channelgrouptv->SearchAndSetChannelIcons(true);
  if (channelgroupradio)
    channelgroupradio->SearchAndSetChannelIcons(true);
}

CFileItemPtr CPVRChannelGroupsContainer::GetLastPlayedChannel(void) const
{
  CFileItemPtr channelTV = m_groupsTV->GetGroupAll()->GetLastPlayedChannel();
  CFileItemPtr channelRadio = m_groupsRadio->GetGroupAll()->GetLastPlayedChannel();

  if (!channelTV ||
      (channelRadio && channelRadio->GetPVRChannelInfoTag()->LastWatched() > channelTV->GetPVRChannelInfoTag()->LastWatched()))
     return channelRadio;

  return channelTV;
}

CPVRChannelGroupPtr CPVRChannelGroupsContainer::GetLastPlayedGroup(int iChannelID /* = -1 */) const
{
  CPVRChannelGroupPtr groupTV = m_groupsTV->GetLastPlayedGroup(iChannelID);
  CPVRChannelGroupPtr groupRadio = m_groupsRadio->GetLastPlayedGroup(iChannelID);

  if (!groupTV || (groupRadio && groupTV->LastWatched() < groupRadio->LastWatched()))
    return groupRadio;

  return groupTV;
}

bool CPVRChannelGroupsContainer::CreateChannelEpgs(void)
{
  return m_groupsTV->CreateChannelEpgs() && m_groupsRadio->CreateChannelEpgs();
}

CPVRChannelGroupPtr CPVRChannelGroupsContainer::GetPreviousPlayedGroup(void)
{
  CSingleLock lock(m_critSection);
  return m_lastPlayedGroups[0];
}

void CPVRChannelGroupsContainer::SetLastPlayedGroup(const CPVRChannelGroupPtr &group)
{
  CSingleLock lock(m_critSection);
  m_lastPlayedGroups[0] = m_lastPlayedGroups[1];
  m_lastPlayedGroups[1] = group;
}
