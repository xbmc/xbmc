/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupsContainer.h"

#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
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
  lock.Leave();

  CLog::LogFC(LOGDEBUG, LOGPVR, "Updating {}", bChannelsOnly ? "channels" : "channel groups");
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

std::shared_ptr<CPVRChannel> CPVRChannelGroupsContainer::GetChannelForEpgTag(const std::shared_ptr<CPVREpgInfoTag>& epgTag) const
{
  if (!epgTag)
    return {};

  return Get(epgTag->IsRadio())->GetGroupAll()->GetByUniqueID(epgTag->UniqueChannelID(), epgTag->ClientID());
}

std::shared_ptr<CPVRChannelGroupMember> CPVRChannelGroupsContainer::GetChannelGroupMemberByPath(
    const std::string& strPath) const
{
  const CPVRChannelsPath path(strPath);
  if (path.IsValid())
    return Get(path.IsRadio())->GetChannelGroupMemberByPath(path);

  return {};
}

std::shared_ptr<CPVRChannel> CPVRChannelGroupsContainer::GetByPath(const std::string& strPath) const
{
  const std::shared_ptr<CPVRChannelGroupMember> groupMember = GetChannelGroupMemberByPath(strPath);
  if (groupMember)
    return groupMember->Channel();

  return {};
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

std::shared_ptr<CPVRChannelGroupMember> CPVRChannelGroupsContainer::
    GetLastPlayedChannelGroupMember() const
{
  std::shared_ptr<CPVRChannelGroupMember> channelTV =
      m_groupsTV->GetGroupAll()->GetLastPlayedChannelGroupMember();
  std::shared_ptr<CPVRChannelGroupMember> channelRadio =
      m_groupsRadio->GetGroupAll()->GetLastPlayedChannelGroupMember();

  if (!channelTV || (channelRadio &&
                     channelRadio->Channel()->LastWatched() > channelTV->Channel()->LastWatched()))
    return channelRadio;

  return channelTV;
}

bool CPVRChannelGroupsContainer::CreateChannelEpgs()
{
  bool bReturn = m_groupsTV->CreateChannelEpgs();
  bReturn &= m_groupsRadio->CreateChannelEpgs();
  return bReturn;
}

int CPVRChannelGroupsContainer::CleanupCachedImages()
{
  return m_groupsTV->CleanupCachedImages() + m_groupsRadio->CleanupCachedImages();
}
