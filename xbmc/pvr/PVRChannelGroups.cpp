/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "FileItem.h"
#include "GUISettings.h"
#include "GUIWindowManager.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "LocalizeStrings.h"
#include "utils/log.h"
#include "Util.h"
#include "URL.h"
#include "FileSystem/File.h"
#include "MusicInfoTag.h"

#include "PVRChannelGroupInternal.h"
#include "PVRChannelGroupsContainer.h"
#include "PVRDatabase.h"
#include "PVRManager.h"

using namespace XFILE;
using namespace MUSIC_INFO;

CPVRChannelGroups::CPVRChannelGroups(bool bRadio)
{
  m_bRadio = bRadio;
}

CPVRChannelGroups::~CPVRChannelGroups(void)
{
}

int CPVRChannelGroups::GetIndexForGroupID(int iGroupId)
{
  int iReturn = -1;

  for (unsigned int iGroupPtr = 0; iGroupPtr < size(); iGroupPtr++)
  {
    if (at(iGroupPtr).GroupID() == iGroupId)
    {
      iReturn = iGroupPtr;
      break;
    }
  }

  return iReturn;
}

bool CPVRChannelGroups::Load(void)
{
  Unload();

  /* create internal channel group */
  CPVRChannelGroup *internalChannels = new CPVRChannelGroupInternal(m_bRadio);
  push_back(internalChannels);
  internalChannels->Load();

  /* load the other groups froom the database */
  CPVRDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  database->GetChannelGroupList(*this, m_bRadio);
  database->Close();
  return true;
}

void CPVRChannelGroups::Unload()
{
  for (unsigned int iGroupPtr = 0; iGroupPtr < size(); iGroupPtr++)
    delete &at(iGroupPtr);

  clear();
}

CPVRChannelGroup *CPVRChannelGroups::GetGroupAll(void)
{
  if (size() > 0)
    return &at(0);
  else
    return NULL;
}

int CPVRChannelGroups::GetGroupList(CFileItemList* results)
{
  int iReturn = 0;

  for (unsigned int iGroupPtr = 0; iGroupPtr < size(); iGroupPtr++)
  {
    CFileItemPtr group(new CFileItem(at(iGroupPtr).GroupName()));
    group->m_strTitle = at(iGroupPtr).GroupName();
    group->m_strPath.Format("%i", at(iGroupPtr).GroupID());
    results->Add(group);
    ++iReturn;
  }

  return iReturn;
}

CPVRChannelGroup *CPVRChannelGroups::GetGroupById(int iGroupId)
{
  CPVRChannelGroup *group = NULL;

  if (iGroupId == XBMC_INTERNAL_GROUPID)
  {
    group = g_PVRChannelGroups.GetGroupAll(m_bRadio);
  }
  else if (iGroupId > -1)
  {
    int iGroupIndex = GetIndexForGroupID(iGroupId);
    if (iGroupIndex != -1)
      group = &at(iGroupIndex);
  }

  return group;
}

int CPVRChannelGroups::GetFirstChannelForGroupID(int iGroupId)
{
  int iReturn = 1;

  CPVRChannelGroup *group;

  if (iGroupId == -1 || iGroupId == XBMC_INTERNAL_GROUPID)
    group = GetGroupAll();
  else
    group = GetGroupById(iGroupId);

  if (group)
    iReturn = group->GetFirstChannel()->ChannelID();

  return iReturn;
}

int CPVRChannelGroups::GetPreviousGroupID(int iGroupId)
{
  int iReturn = XBMC_INTERNAL_GROUPID;

  int iCurrentGroupIndex = GetIndexForGroupID(iGroupId);
  if (iCurrentGroupIndex != -1)
  {
    int iGroupIndex = iCurrentGroupIndex - 1;
    if (iGroupIndex < 0) iGroupIndex = size() - 1;

    iReturn = at(iGroupIndex).GroupID();
  }

  return iReturn;
}

int CPVRChannelGroups::GetNextGroupID(int iGroupId)
{
  int iReturn = XBMC_INTERNAL_GROUPID;

  int iCurrentGroupIndex = GetIndexForGroupID(iGroupId);
  if (iCurrentGroupIndex != -1)
  {
    int iGroupIndex = iCurrentGroupIndex + 1;
    if (iGroupIndex == size()) iGroupIndex = 0;

    iReturn = at(iGroupIndex).GroupID();
  }

  return iReturn;
}

void CPVRChannelGroups::AddGroup(const CStdString &name)
{
  CPVRDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  Unload();
  database->AddChannelGroup(name, -1, m_bRadio);
  database->GetChannelGroupList(*this, m_bRadio);

  database->Close();
}

bool CPVRChannelGroups::RenameGroup(int GroupId, const CStdString &newname)
{
  CPVRDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  Unload();
  database->SetChannelGroupName(GroupId, newname, m_bRadio);
  database->GetChannelGroupList(*this, m_bRadio);  database->Close();
  return true;
}

bool CPVRChannelGroups::DeleteGroup(int GroupId)
{
  CPVRDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  Unload();

  const CPVRChannelGroup *channels = g_PVRChannelGroups.GetGroupAll(m_bRadio);

  /* Delete the group inside Database */
  database->DeleteChannelGroup(GroupId, m_bRadio);

  /* Set all channels with this group to undefined */
  for (unsigned int i = 0; i < channels->size(); i++)
  {
    if (channels->at(i)->GroupID() == GroupId)
    {
      channels->at(i)->SetGroupID(0, true);
    }
  }

  /* Reload the group list */
  database->GetChannelGroupList(*this, m_bRadio);

  database->Close();
  return true;
}

CStdString CPVRChannelGroups::GetGroupName(int GroupId)
{
  if (GroupId != XBMC_INTERNAL_GROUPID)
  {
    for (unsigned int i = 0; i < size(); i++)
    {
      if (GroupId == at(i).GroupID())
        return at(i).GroupName();
    }
  }

  return g_localizeStrings.Get(593);
}

int CPVRChannelGroups::GetGroupId(CStdString GroupName)
{
  if (GroupName.IsEmpty() || GroupName == g_localizeStrings.Get(593) || GroupName == "All")
    return XBMC_INTERNAL_GROUPID;

  for (unsigned int i = 0; i < size(); i++)
  {
    if (GroupName == at(i).GroupName())
      return at(i).GroupID();
  }
  return -1;
}

bool CPVRChannelGroups::ChannelToGroup(const CPVRChannel &channel, int GroupId)
{
  const CPVRChannelGroup *channels = g_PVRChannelGroups.GetGroupAll(channel.IsRadio());
  channels->at(channel.ChannelNumber()-1)->SetGroupID(GroupId);
  return channels->at(channel.ChannelNumber()-1)->Persist();
}
