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
#include "settings/GUISettings.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "Util.h"
#include "URL.h"
#include "filesystem/File.h"
#include "music/tags/MusicInfoTag.h"

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

void CPVRChannelGroups::Unload(void)
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
    if (iGroupIndex == (int) size()) iGroupIndex = 0;

    iReturn = at(iGroupIndex).GroupID();
  }

  return iReturn;
}

void CPVRChannelGroups::AddGroup(const CStdString &strName)
{
  CPVRDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  Unload();
  database->AddChannelGroup(strName, -1, m_bRadio);
  database->GetChannelGroupList(*this, m_bRadio);

  database->Close();
}

bool CPVRChannelGroups::RenameGroup(int iGroupId, const CStdString &strNewName)
{
  CPVRDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  Unload();
  database->SetChannelGroupName(iGroupId, strNewName, m_bRadio);
  database->GetChannelGroupList(*this, m_bRadio);  database->Close();
  return true;
}

bool CPVRChannelGroups::DeleteGroup(int iGroupId)
{
  CPVRDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  Unload();

  const CPVRChannelGroup *channels = g_PVRChannelGroups.GetGroupAll(m_bRadio);

  /* Delete the group inside Database */
  database->DeleteChannelGroup(iGroupId, m_bRadio);

  /* Set all channels with this group to undefined */
  for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
  {
    if (channels->at(iChannelPtr)->GroupID() == iGroupId)
    {
      channels->at(iChannelPtr)->SetGroupID(0, true);
    }
  }

  /* Reload the group list */
  database->GetChannelGroupList(*this, m_bRadio);

  database->Close();
  return true;
}

CStdString CPVRChannelGroups::GetGroupName(int iGroupId)
{
  if (iGroupId != XBMC_INTERNAL_GROUPID)
  {
    for (unsigned int iGroupPtr = 0; iGroupPtr < size(); iGroupPtr++)
    {
      if (iGroupId == at(iGroupPtr).GroupID())
        return at(iGroupPtr).GroupName();
    }
  }

  return g_localizeStrings.Get(593);
}

int CPVRChannelGroups::GetGroupId(CStdString strGroupName)
{
  if (strGroupName.IsEmpty() || strGroupName == g_localizeStrings.Get(593) || strGroupName == "All")
    return XBMC_INTERNAL_GROUPID;

  for (unsigned int iGroupPtr = 0; iGroupPtr < size(); iGroupPtr++)
  {
    if (strGroupName == at(iGroupPtr).GroupName())
      return at(iGroupPtr).GroupID();
  }
  return -1;
}

bool CPVRChannelGroups::AddChannelToGroup(const CPVRChannel &channel, int iGroupId)
{
  const CPVRChannelGroup *channels = g_PVRChannelGroups.GetGroupAll(channel.IsRadio());
  channels->at(channel.ChannelNumber()-1)->SetGroupID(iGroupId);
  return channels->at(channel.ChannelNumber()-1)->Persist();
}
