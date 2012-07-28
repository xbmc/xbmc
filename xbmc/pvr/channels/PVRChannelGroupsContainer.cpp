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
  if (!channel->IsValid())
    channel = m_groupsRadio->GetGroupAll()->GetByChannelID(iChannelId);

  return channel;
}

CPVRChannelPtr CPVRChannelGroupsContainer::GetChannelByEpgId(int iEpgId) const
{
  CPVRChannelPtr channel = m_groupsTV->GetGroupAll()->GetByChannelEpgID(iEpgId);
  if (!channel->IsValid())
    channel = m_groupsRadio->GetGroupAll()->GetByChannelEpgID(iEpgId);

  return channel;
}

bool CPVRChannelGroupsContainer::GetGroupsDirectory(CFileItemList *results, bool bRadio)
{
  const CPVRChannelGroups *channelGroups = Get(bRadio);
  if (channelGroups)
  {
    channelGroups->GetGroupList(results);
    return true;
  }
  return false;
}

CFileItemPtr CPVRChannelGroupsContainer::GetByPath(const CStdString &strPath) const
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
    return GetGroupsDirectory(&results, false);
  }
  else if (fileName == "channels/radio")
  {
    return GetGroupsDirectory(&results, true);
  }
  else if (fileName.Left(12) == "channels/tv/")
  {
    CStdString strGroupName(fileName.substr(12));
    URIUtils::RemoveSlashAtEnd(strGroupName);
    CPVRChannelGroupPtr group = GetTV()->GetByName(strGroupName);
    if (!group->IsValid())
      group = GetGroupAllTV();
    if (group->IsValid())
      group->GetMembers(results, !fileName.Right(7).Equals(".hidden"));
    return true;
  }
  else if (fileName.Left(15) == "channels/radio/")
  {
    CStdString strGroupName(fileName.substr(15));
    URIUtils::RemoveSlashAtEnd(strGroupName);
    CPVRChannelGroupPtr group = GetRadio()->GetByName(strGroupName);
    if (!group->IsValid())
      group = GetGroupAllRadio();
    if (group->IsValid())
      group->GetMembers(results, !fileName.Right(7).Equals(".hidden"));
    return true;
  }

  return false;
}

int CPVRChannelGroupsContainer::GetNumChannelsFromAll()
{
  return GetGroupAllTV()->Size() + GetGroupAllRadio()->Size();
}

CPVRChannelGroupPtr CPVRChannelGroupsContainer::GetSelectedGroup(bool bRadio) const
{
  return Get(bRadio)->GetSelectedGroup();
}

CPVRChannelPtr CPVRChannelGroupsContainer::GetByUniqueID(int iClientChannelNumber, int iClientID)
{
  CPVRChannelPtr channel = CPVRChannelPtrEmpty;
  CPVRChannelGroupPtr channelgroup = GetGroupAllTV();
  if (channelgroup->IsValid())
    channel = channelgroup->GetByClient(iClientChannelNumber, iClientID);

  if (!channelgroup->IsValid() || !channel->IsValid())
    channelgroup = GetGroupAllRadio();
  if (channelgroup->IsValid())
    channel = channelgroup->GetByClient(iClientChannelNumber, iClientID);

  return channel;
}

CFileItemPtr CPVRChannelGroupsContainer::GetByChannelIDFromAll(int iChannelID)
{
  CPVRChannelPtr channel = CPVRChannelPtrEmpty;
  CPVRChannelGroupPtr channelgroup = GetGroupAllTV();
  if (channelgroup->IsValid())
    channel = channelgroup->GetByChannelID(iChannelID);

  if (!channel->IsValid())
  {
    channelgroup = GetGroupAllRadio();
    if (channelgroup->IsValid())
      channel = channelgroup->GetByChannelID(iChannelID);
  }

  if (channel->IsValid())
  {
    CFileItemPtr retVal = CFileItemPtr(new CFileItem(*channel));
    return retVal;
  }

  CFileItemPtr retVal = CFileItemPtr(new CFileItem);
  return retVal;
}

void CPVRChannelGroupsContainer::SearchMissingChannelIcons(void)
{
  CLog::Log(LOGINFO, "PVRChannelGroupsContainer - %s - starting channel icon search", __FUNCTION__);

  // TODO: Add Process dialog here
  CPVRChannelGroupPtr channelgrouptv  = GetGroupAllTV();
  CPVRChannelGroupPtr channelgroupradio = GetGroupAllRadio();

  if (channelgrouptv->IsValid())
    channelgrouptv->SearchAndSetChannelIcons(true);
  if (channelgroupradio->IsValid())
    channelgroupradio->SearchAndSetChannelIcons(true);

  CGUIDialogOK::ShowAndGetInput(19103,0,20177,0);
}

CFileItemPtr CPVRChannelGroupsContainer::GetLastPlayedChannel(void) const
{
  CFileItemPtr lastChannel = GetGroupAllTV()->GetLastPlayedChannel();
  bool bHasTVChannel(lastChannel && lastChannel->HasPVRChannelInfoTag());

  CFileItemPtr lastRadioChannel = GetGroupAllRadio()->GetLastPlayedChannel();
  bool bHasRadioChannel(lastRadioChannel && lastRadioChannel->HasPVRChannelInfoTag());

  if (!bHasTVChannel || (bHasRadioChannel && lastChannel->GetPVRChannelInfoTag()->LastWatched() < lastRadioChannel->GetPVRChannelInfoTag()->LastWatched()))
    return lastRadioChannel;

  return lastChannel;
}
