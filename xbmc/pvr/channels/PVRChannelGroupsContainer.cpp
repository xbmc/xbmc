/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupsContainer.h"

#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/epg/EpgInfoTag.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <memory>

using namespace PVR;

CPVRChannelGroupsContainer::CPVRChannelGroupsContainer() :
    m_groupsRadio(new CPVRChannelGroups(true)),
    m_groupsTV(new CPVRChannelGroups(false))
{
}

CPVRChannelGroupsContainer::~CPVRChannelGroupsContainer()
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

bool CPVRChannelGroupsContainer::Load()
{
  Unload();
  m_bLoaded = m_groupsTV->Load() && m_groupsRadio->Load();
  return m_bLoaded;
}

bool CPVRChannelGroupsContainer::Loaded() const
{
  return m_bLoaded;
}

void CPVRChannelGroupsContainer::Unload()
{
  m_groupsRadio->Clear();
  m_groupsTV->Clear();
  m_bLoaded = false;
}

CPVRChannelGroups* CPVRChannelGroupsContainer::Get(bool bRadio) const
{
  return bRadio ? m_groupsRadio : m_groupsTV;
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroupsContainer::GetGroupAll(bool bRadio) const
{
  return Get(bRadio)->GetGroupAll();
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroupsContainer::GetByIdFromAll(int iGroupId) const
{
  std::shared_ptr<CPVRChannelGroup> group = m_groupsTV->GetById(iGroupId);
  if (!group)
    group = m_groupsRadio->GetById(iGroupId);

  return group;
}

std::shared_ptr<CPVRChannel> CPVRChannelGroupsContainer::GetChannelById(int iChannelId) const
{
  std::shared_ptr<CPVRChannel> channel = m_groupsTV->GetGroupAll()->GetByChannelID(iChannelId);
  if (!channel)
    channel = m_groupsRadio->GetGroupAll()->GetByChannelID(iChannelId);

  return channel;
}

std::shared_ptr<CPVRChannel> CPVRChannelGroupsContainer::GetChannelByEpgId(int iEpgId) const
{
  std::shared_ptr<CPVRChannel> channel = m_groupsTV->GetGroupAll()->GetByChannelEpgID(iEpgId);
  if (!channel)
    channel = m_groupsRadio->GetGroupAll()->GetByChannelEpgID(iEpgId);

  return channel;
}

std::shared_ptr<CPVRChannel> CPVRChannelGroupsContainer::GetChannelForEpgTag(const std::shared_ptr<CPVREpgInfoTag>& epgTag) const
{
  if (!epgTag)
    return {};

  return Get(epgTag->IsRadio())->GetGroupAll()->GetByUniqueID(epgTag->UniqueChannelID(), epgTag->ClientID());
}

std::shared_ptr<CPVRChannel> CPVRChannelGroupsContainer::GetByPath(const std::string& strPath) const
{
  const CPVRChannelsPath path(strPath);
  if (path.IsValid())
    return Get(path.IsRadio())->GetByPath(path);

  return {};
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroupsContainer::GetSelectedGroup(bool bRadio) const
{
  return Get(bRadio)->GetSelectedGroup();
}

std::shared_ptr<CPVRChannel> CPVRChannelGroupsContainer::GetByUniqueID(int iUniqueChannelId, int iClientID) const
{
  std::shared_ptr<CPVRChannel> channel;
  std::shared_ptr<CPVRChannelGroup> channelgroup = GetGroupAllTV();
  if (channelgroup)
    channel = channelgroup->GetByUniqueID(iUniqueChannelId, iClientID);

  if (!channelgroup || !channel)
    channelgroup = GetGroupAllRadio();
  if (channelgroup)
    channel = channelgroup->GetByUniqueID(iUniqueChannelId, iClientID);

  return channel;
}

std::shared_ptr<CPVRChannel> CPVRChannelGroupsContainer::GetLastPlayedChannel() const
{
  const std::shared_ptr<CPVRChannel> channelTV = m_groupsTV->GetGroupAll()->GetLastPlayedChannel();
  const std::shared_ptr<CPVRChannel> channelRadio = m_groupsRadio->GetGroupAll()->GetLastPlayedChannel();

  if (!channelTV ||
      (channelRadio && channelRadio->LastWatched() > channelTV->LastWatched()))
     return channelRadio;

  return channelTV;
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroupsContainer::GetLastPlayedGroup(int iChannelID /* = -1 */) const
{
  std::shared_ptr<CPVRChannelGroup> groupTV = m_groupsTV->GetLastPlayedGroup(iChannelID);
  std::shared_ptr<CPVRChannelGroup> groupRadio = m_groupsRadio->GetLastPlayedGroup(iChannelID);

  if (!groupTV || (groupRadio && groupTV->LastWatched() < groupRadio->LastWatched()))
    return groupRadio;

  return groupTV;
}

bool CPVRChannelGroupsContainer::CreateChannelEpgs()
{
  return m_groupsTV->CreateChannelEpgs() && m_groupsRadio->CreateChannelEpgs();
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroupsContainer::GetPreviousPlayedGroup()
{
  CSingleLock lock(m_critSection);
  return m_lastPlayedGroups[0];
}

void CPVRChannelGroupsContainer::SetLastPlayedGroup(const std::shared_ptr<CPVRChannelGroup>& group)
{
  CSingleLock lock(m_critSection);
  m_lastPlayedGroups[0] = m_lastPlayedGroups[1];
  m_lastPlayedGroups[1] = group;
}
