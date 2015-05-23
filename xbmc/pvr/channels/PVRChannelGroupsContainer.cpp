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

#include "PVRChannelGroupsContainer.h"
#include "URL.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace PVR;

CPVRChannelGroupsContainer::CPVRChannelGroupsContainer(void) :
    m_groupsRadio(new CPVRChannelGroups(true)),
    m_groupsTV(new CPVRChannelGroups(false)),
    m_bUpdateChannelsOnly(false),
    m_bIsUpdating(false)
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

  CLog::Log(LOGDEBUG, "CPVRChannelGroupsContainer - %s - updating %s", __FUNCTION__, bChannelsOnly ? "channels" : "channel groups");
  bool bReturn = m_groupsRadio->Update(bChannelsOnly) &&
       m_groupsTV->Update(bChannelsOnly);

  lock.Enter();
  m_bIsUpdating = false;
  lock.Leave();

  return bReturn;
}

bool CPVRChannelGroupsContainer::Load(void)
{
  Unload();

  return m_groupsRadio->Load() &&
         m_groupsTV->Load();
}

void CPVRChannelGroupsContainer::Unload(void)
{
  m_groupsRadio->Clear();
  m_groupsTV->Clear();
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

bool CPVRChannelGroupsContainer::GetGroupsDirectory(CFileItemList *results, bool bRadio) const
{
  const CPVRChannelGroups *channelGroups = Get(bRadio);
  if (channelGroups)
  {
    channelGroups->GetGroupList(results);
    return true;
  }
  return false;
}

CFileItemPtr CPVRChannelGroupsContainer::GetByPath(const std::string &strPath) const
{
  for (unsigned int bRadio = 0; bRadio <= 1; bRadio++)
  {
    const CPVRChannelGroups *groups = Get(bRadio == 1);
    CFileItemPtr retVal = groups->GetByPath(strPath);
    if (retVal && retVal->HasPVRChannelInfoTag())
      return retVal;
  }

  CFileItemPtr retVal(new CFileItem);
  return retVal;
}

bool CPVRChannelGroupsContainer::GetDirectory(const std::string& strPath, CFileItemList &results) const
{
  std::string strBase(strPath);
  URIUtils::RemoveSlashAtEnd(strBase);

  /* get the filename from curl */
  CURL url(strPath);
  std::string fileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);

  if (fileName == "channels")
  {
    CFileItemPtr item;

    /* all tv channels */
    item.reset(new CFileItem(strBase + "/tv/", true));
    item->SetLabel(g_localizeStrings.Get(19020));
    item->SetLabelPreformated(true);
    results.Add(item);

    /* all radio channels */
    item.reset(new CFileItem(strBase + "/radio/", true));
    item->SetLabel(g_localizeStrings.Get(19021));
    item->SetLabelPreformated(true);
    results.Add(item);

    return true;
  }
  else if (fileName == "channels/tv")
  {
    return GetGroupsDirectory(&results, false);
  }
  else if (fileName == "channels/radio")
  {
    return GetGroupsDirectory(&results, true);
  }
  else if (StringUtils::StartsWith(fileName, "channels/tv/"))
  {
    std::string strGroupName(fileName.substr(12));
    URIUtils::RemoveSlashAtEnd(strGroupName);
    CPVRChannelGroupPtr group = GetTV()->GetByName(strGroupName);
    if (!group)
      group = GetGroupAllTV();
    if (group)
      group->GetMembers(results, !StringUtils::EndsWithNoCase(fileName, ".hidden"));
    return true;
  }
  else if (StringUtils::StartsWith(fileName, "channels/radio/"))
  {
    std::string strGroupName(fileName.substr(15));
    URIUtils::RemoveSlashAtEnd(strGroupName);
    CPVRChannelGroupPtr group = GetRadio()->GetByName(strGroupName);
    if (!group)
      group = GetGroupAllRadio();
    if (group)
      group->GetMembers(results, !StringUtils::EndsWithNoCase(fileName, ".hidden"));
    return true;
  }

  return false;
}

int CPVRChannelGroupsContainer::GetNumChannelsFromAll() const
{
  return GetGroupAllTV()->Size() + GetGroupAllRadio()->Size();
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
  CLog::Log(LOGINFO, "PVRChannelGroupsContainer - %s - starting channel icon search", __FUNCTION__);

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
      !channelTV->HasPVRChannelInfoTag() ||
      (channelRadio && channelRadio->HasPVRChannelInfoTag() &&
       channelRadio->GetPVRChannelInfoTag()->LastWatched() > channelTV->GetPVRChannelInfoTag()->LastWatched()))
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
  return m_groupsRadio->CreateChannelEpgs() &&
         m_groupsTV->CreateChannelEpgs();
}

CPVRChannelGroupPtr CPVRChannelGroupsContainer::GetPreviousPlayedGroup(void)
{
  CSingleLock lock(m_critSection);
  return m_lastPlayedGroups[0];
}

void CPVRChannelGroupsContainer::SetLastPlayedGroup(CPVRChannelGroupPtr group)
{
  CSingleLock lock(m_critSection);
  m_lastPlayedGroups[0] = m_lastPlayedGroups[1];
  m_lastPlayedGroups[1] = group;
}
