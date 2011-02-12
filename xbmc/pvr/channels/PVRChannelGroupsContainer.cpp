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
#include "dialogs/GUIDialogOK.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

CPVRChannelGroupsContainer::CPVRChannelGroupsContainer(void)
{

  m_groupsRadio = new CPVRChannelGroups(true);
  m_groupsTV    = new CPVRChannelGroups(false);
}

CPVRChannelGroupsContainer::~CPVRChannelGroupsContainer(void)
{
  delete m_groupsRadio;
  delete m_groupsTV;
}

bool CPVRChannelGroupsContainer::Update(void)
{
  return m_groupsRadio->Update() &&
         m_groupsTV->Update();
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

const CPVRChannelGroups *CPVRChannelGroupsContainer::Get(bool bRadio) const
{
  return bRadio ? m_groupsRadio : m_groupsTV;
}

const CPVRChannelGroup *CPVRChannelGroupsContainer::GetGroupAll(bool bRadio) const
{
  const CPVRChannelGroup *group = NULL;
  const CPVRChannelGroups *groups = Get(bRadio);
  if (groups)
    group = groups->GetGroupAll();

  return group;
}

const CPVRChannelGroup *CPVRChannelGroupsContainer::GetById(bool bRadio, int iGroupId) const
{
  const CPVRChannelGroup *group = NULL;
  const CPVRChannelGroups *groups = Get(bRadio);
  if (groups)
    group = (iGroupId == XBMC_INTERNAL_GROUPID) ?
        groups->GetGroupAll() :
        groups->GetById(iGroupId);

  return group;
}

const CPVRChannel *CPVRChannelGroupsContainer::GetChannelById(int iChannelId) const
{
  const CPVRChannel *channel = m_groupsTV->GetGroupAll()->GetByChannelID(iChannelId);

  if (!channel)
    channel = m_groupsRadio->GetGroupAll()->GetByChannelID(iChannelId);

  return channel;
}

bool CPVRChannelGroupsContainer::GetGroupsDirectory(const CStdString &strBase, CFileItemList *results, bool bRadio)
{
  const CPVRChannelGroup * channels = GetGroupAll(bRadio);
  const CPVRChannelGroups *channelGroups = Get(bRadio);
  CFileItemPtr item;

  item.reset(new CFileItem(strBase + "/all/", true));
  item->SetLabel(g_localizeStrings.Get(593));
  item->SetLabelPreformated(true);
  results->Add(item);

  /* container has hidden channels */
  if (channels->GetNumHiddenChannels() > 0)
  {
    item.reset(new CFileItem(strBase + "/.hidden/", true));
    item->SetLabel(g_localizeStrings.Get(19022));
    item->SetLabelPreformated(true);
    results->Add(item);
  }

  /* add all groups */
  for (unsigned int ptr = 0; ptr < channelGroups->size(); ptr++)
  {
    const CPVRChannelGroup group = channelGroups->at(ptr);
    CStdString strGroup = strBase + "/" + group.GroupName() + "/";
    item.reset(new CFileItem(strGroup, true));
    item->SetLabel(group.GroupName());
    item->SetLabelPreformated(true);
    results->Add(item);
  }

  return true;
}

const CPVRChannel *CPVRChannelGroupsContainer::GetByPath(const CStdString &strPath)
{
  const CPVRChannelGroup *channels = NULL;
  int iChannelNumber = -1;

  /* get the filename from curl */
  CURL url(strPath);
  CStdString strFileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(strFileName);

  if (strFileName.Left(16) == "channels/tv/all/")
  {
    strFileName.erase(0,16);
    iChannelNumber = atoi(strFileName.c_str());
    channels = GetGroupAllTV();
  }
  else if (strFileName.Left(19) == "channels/radio/all/")
  {
    strFileName.erase(0,19);
    iChannelNumber = atoi(strFileName.c_str());
    channels = GetGroupAllRadio();
  }

  return channels ? channels->GetByChannelNumber(iChannelNumber) : NULL;
}

bool CPVRChannelGroupsContainer::GetDirectory(const CStdString& strPath, CFileItemList &results)
{
  CStdString strBase(strPath);
  URIUtils::RemoveSlashAtEnd(strBase);

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
    GetGroupAllTV()->GetChannels(&results, (fileName.substr(12) == ".hidden") ? -1 : GetTV()->GetGroupId(fileName.substr(12)), false);
    return true;
  }
  else if (fileName.Left(15) == "channels/radio/")
  {
    GetGroupAllRadio()->GetChannels(&results, (fileName.substr(15) == ".hidden") ? -1 : GetRadio()->GetGroupId(fileName.substr(15)), false);
    return true;
  }

  return false;
}

int CPVRChannelGroupsContainer::GetNumChannelsFromAll()
{
  return GetGroupAllTV()->GetNumChannels() + GetGroupAllRadio()->GetNumChannels();
}

const CPVRChannel *CPVRChannelGroupsContainer::GetByClientFromAll(int iClientChannelNumber, int iClientID)
{
  const CPVRChannel *channel = NULL;

  channel = GetGroupAllTV()->GetByClient(iClientChannelNumber, iClientID);

  if (channel == NULL)
    channel = GetGroupAllRadio()->GetByClient(iClientChannelNumber, iClientID);

  return channel;
}

const CPVRChannel *CPVRChannelGroupsContainer::GetByChannelIDFromAll(int iChannelID)
{
  const CPVRChannel *channel = NULL;

  channel = GetGroupAllTV()->GetByChannelID(iChannelID);

  if (channel == NULL)
    channel = GetGroupAllRadio()->GetByChannelID(iChannelID);

  return channel;
}

const CPVRChannel *CPVRChannelGroupsContainer::GetByUniqueIDFromAll(int iUniqueID)
{
  const CPVRChannel *channel;

  channel = GetGroupAllTV()->GetByUniqueID(iUniqueID);

  if (channel == NULL)
    channel = GetGroupAllRadio()->GetByUniqueID(iUniqueID);

  return NULL;
}

void CPVRChannelGroupsContainer::SearchMissingChannelIcons(void)
{
  CLog::Log(LOGINFO, "PVRChannelGroupsContainer - %s - starting channel icon search", __FUNCTION__);

  // TODO: Add Process dialog here
  ((CPVRChannelGroup *) GetGroupAllTV())->SearchAndSetChannelIcons(true);
  ((CPVRChannelGroup *) GetGroupAllRadio())->SearchAndSetChannelIcons(true);

  CGUIDialogOK::ShowAndGetInput(19103,0,20177,0);
}
