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

#include "PVRChannelGroupsContainer.h"
#include "URL.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "pvr/PVRManager.h"

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

CPVRChannelGroupInternal *CPVRChannelGroupsContainer::GetGroupAll(bool bRadio) const
{
  CPVRChannelGroupInternal *group = NULL;
  const CPVRChannelGroups *groups = Get(bRadio);
  if (groups)
    group = groups->GetGroupAll();

  return group;
}

CPVRChannelGroup *CPVRChannelGroupsContainer::GetById(bool bRadio, int iGroupId) const
{
  CPVRChannelGroup *group = NULL;
  const CPVRChannelGroups *groups = Get(bRadio);
  if (groups)
    group = groups->GetById(iGroupId);

  return group;
}

CPVRChannelGroup *CPVRChannelGroupsContainer::GetByIdFromAll(int iGroupId) const
{
  CPVRChannelGroup *group = m_groupsRadio->GetById(iGroupId);
  if (!group)
    group = m_groupsTV->GetById(iGroupId);

  return group;
}

CPVRChannel *CPVRChannelGroupsContainer::GetChannelById(int iChannelId) const
{
  CPVRChannel *channel = m_groupsTV->GetGroupAll()->GetByChannelID(iChannelId);
  if (!channel)
    channel = m_groupsRadio->GetGroupAll()->GetByChannelID(iChannelId);

  return channel;
}

CPVRChannel *CPVRChannelGroupsContainer::GetChannelByEpgId(int iEpgId) const
{
  CPVRChannel *channel = m_groupsTV->GetGroupAll()->GetByChannelEpgID(iEpgId);
  if (!channel)
    channel = m_groupsRadio->GetGroupAll()->GetByChannelEpgID(iEpgId);

  return channel;
}

bool CPVRChannelGroupsContainer::GetGroupsDirectory(const CStdString &strBase, CFileItemList *results, bool bRadio)
{
  const CPVRChannelGroups *channelGroups = Get(bRadio);
  CFileItemPtr item;

  /* add all groups */
  for (unsigned int ptr = 0; ptr < channelGroups->size(); ptr++)
  {
    const CPVRChannelGroup *group = channelGroups->at(ptr);
    CStdString strGroup = strBase + "/" + group->GroupName() + "/";
    item.reset(new CFileItem(strGroup, true));
    item->SetLabel(group->GroupName());
    item->SetLabelPreformated(true);
    results->Add(item);
  }

  return true;
}

CPVRChannel *CPVRChannelGroupsContainer::GetByPath(const CStdString &strPath)
{
  const CPVRChannelGroup *channels = NULL;
  int iChannelIndex(-1);

  /* get the filename from curl */
  CURL url(strPath);
  CStdString strFileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(strFileName);

  CStdString strCheckPath;
  for (unsigned int bRadio = 0; bRadio <= 1; bRadio++)
  {
    const CPVRChannelGroups *groups = Get(bRadio == 1);
    for (unsigned int iGroupPtr = 0; iGroupPtr < groups->size(); iGroupPtr++)
    {
      const CPVRChannelGroup *group = groups->at(iGroupPtr);
      strCheckPath.Format("channels/%s/%s/", group->IsRadio() ? "radio" : "tv", group->GroupName().c_str());

      if (strFileName.Left(strCheckPath.length()) == strCheckPath)
      {
        strFileName.erase(0, strCheckPath.length());
        channels = group;
        iChannelIndex = atoi(strFileName.c_str());
        break;
      }
    }
  }

  return channels ? channels->GetByIndex(iChannelIndex) : NULL;
}

bool CPVRChannelGroupsContainer::GetDirectory(const CStdString& strPath, CFileItemList &results)
{
  CStdString strBase(strPath);

  /* get the filename from curl */
  CURL url(strPath);
  CStdString fileName = url.GetFileName();
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
    return GetGroupsDirectory(strBase, &results, false);
  }
  else if (fileName == "channels/radio")
  {
    return GetGroupsDirectory(strBase, &results, true);
  }
  else if (fileName.Left(12) == "channels/tv/")
  {
    CStdString strGroupName(fileName.substr(12));
    URIUtils::RemoveSlashAtEnd(strGroupName);
    const CPVRChannelGroup *group = GetTV()->GetByName(strGroupName);
    if (!group)
      group = GetGroupAllTV();
    if (group)
      group->GetMembers(results, !fileName.Right(7).Equals(".hidden"));
    return true;
  }
  else if (fileName.Left(15) == "channels/radio/")
  {
    CStdString strGroupName(fileName.substr(15));
    URIUtils::RemoveSlashAtEnd(strGroupName);
    const CPVRChannelGroup *group = GetRadio()->GetByName(strGroupName);
    if (!group)
      group = GetGroupAllRadio();
    if (group)
      group->GetMembers(results, !fileName.Right(7).Equals(".hidden"));
    return true;
  }

  return false;
}

int CPVRChannelGroupsContainer::GetNumChannelsFromAll()
{
  return GetGroupAllTV()->Size() + GetGroupAllRadio()->Size();
}

CPVRChannelGroup *CPVRChannelGroupsContainer::GetSelectedGroup(bool bRadio) const
{
  return Get(bRadio)->GetSelectedGroup();
}

CPVRChannel *CPVRChannelGroupsContainer::GetByUniqueID(int iClientChannelNumber, int iClientID)
{
  CPVRChannel *channel = NULL;
  const CPVRChannelGroup* channelgroup = GetGroupAllTV();

  if (channelgroup == NULL)
    channelgroup = GetGroupAllRadio();

  if (channelgroup != NULL)
    channel = channelgroup->GetByClient(iClientChannelNumber, iClientID);

  return channel;
}

CPVRChannel *CPVRChannelGroupsContainer::GetByChannelIDFromAll(int iChannelID)
{
  CPVRChannel *channel = NULL;
  const CPVRChannelGroup* channelgroup = GetGroupAllTV();
  if (channelgroup)
    channel = channelgroup->GetByChannelID(iChannelID);

  if (!channel)
  {
    channelgroup = GetGroupAllRadio();
    if (channelgroup)
      channel = channelgroup->GetByChannelID(iChannelID);
  }

  return channel;
}

CPVRChannel *CPVRChannelGroupsContainer::GetByClientFromAll(unsigned int iClientId, unsigned int iChannelUid)
{
  CPVRChannel *channel = NULL;

  channel = GetGroupAllTV()->GetByClient(iChannelUid, iClientId);

  if (channel == NULL)
    channel = GetGroupAllRadio()->GetByClient(iChannelUid, iClientId);

  return channel;
}

CPVRChannel *CPVRChannelGroupsContainer::GetByUniqueIDFromAll(int iUniqueID)
{
  CPVRChannel *channel;
  const CPVRChannelGroup* channelgroup = GetGroupAllTV();

  if (channelgroup == NULL)
    channelgroup = GetGroupAllRadio();

  if (channelgroup != NULL)
    channel = channelgroup->GetByUniqueID(iUniqueID);

  return NULL;
}

void CPVRChannelGroupsContainer::SearchMissingChannelIcons(void)
{
  CLog::Log(LOGINFO, "PVRChannelGroupsContainer - %s - starting channel icon search", __FUNCTION__);

  // TODO: Add Process dialog here
  CPVRChannelGroup* channelgrouptv  = (CPVRChannelGroup *) GetGroupAllTV();
  CPVRChannelGroup* channelgroupradio  =(CPVRChannelGroup *) GetGroupAllRadio();

  if (channelgrouptv != NULL)
    channelgrouptv->SearchAndSetChannelIcons(true);
  if (channelgroupradio != NULL)
    channelgroupradio->SearchAndSetChannelIcons(true);

  CGUIDialogOK::ShowAndGetInput(19103,0,20177,0);
}

CPVRChannel *CPVRChannelGroupsContainer::GetLastPlayedChannel(void) const
{
  CPVRChannel *lastChannel = GetGroupAllTV()->GetLastPlayedChannel();

  CPVRChannel *lastRadioChannel = GetGroupAllRadio()->GetLastPlayedChannel();
  if (!lastChannel || (lastRadioChannel && lastChannel->LastWatched() < lastRadioChannel->LastWatched()))
    lastChannel = lastRadioChannel;

  return lastChannel;
}
