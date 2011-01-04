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

#include "PVRChannelGroups.h"
#include "PVRChannelGroup.h"
#include "TVDatabase.h"
#include "PVRManager.h"

using namespace XFILE;
using namespace MUSIC_INFO;

// --- CPVRChannelGroups ----------------------------------------------------------

CPVRChannelGroups PVRChannelGroupsTV;
CPVRChannelGroups PVRChannelGroupsRadio;

CPVRChannelGroups::CPVRChannelGroups(void)
{
}

bool CPVRChannelGroups::Load(bool radio)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  m_bRadio = radio;
  Clear();
  if (!m_bRadio)
    database->GetChannelGroupList(*this);
  else
    database->GetRadioChannelGroupList(*this);

  database->Close();
  return true;
}

void CPVRChannelGroups::Unload()
{
  Clear();
}

int CPVRChannelGroups::GetGroupList(CFileItemList* results)
{
  for (unsigned int i = 0; i < size(); i++)
  {
    CFileItemPtr group(new CFileItem(at(i).GroupName()));
    group->m_strTitle = at(i).GroupName();
    group->m_strPath.Format("%i", at(i).GroupID());
    results->Add(group);
  }
  return size();
}

int CPVRChannelGroups::GetFirstChannelForGroupID(int GroupId)
{
  if (GroupId == -1)
    return 1;

  CPVRChannels *channels;
  if (!m_bRadio)
    channels = &PVRChannelsTV;
  else
    channels = &PVRChannelsRadio;

  for (unsigned int i = 0; i < channels->size(); i++)
  {
    if (channels->at(i).GroupID() == GroupId)
      return i+1;
  }
  return 1;
}

int CPVRChannelGroups::GetPrevGroupID(int current_group_id)
{
  if (size() == 0)
    return -1;

  if ((current_group_id == -1) || (current_group_id == 0))
    return at(size()-1).GroupID();

  for (unsigned int i = 0; i < size(); i++)
  {
    if (current_group_id == at(i).GroupID())
    {
      if (i != 0)
        return at(i-1).GroupID();
      else
        return -1;
    }
  }
  return -1;
}

int CPVRChannelGroups::GetNextGroupID(int current_group_id)
{
  unsigned int i = 0;

  if (size() == 0)
    return -1;

  if ((current_group_id == 0) || (current_group_id == -1))
    return at(0).GroupID();

  if (size() == 0)
    return -1;

  for (; i < size(); i++)
  {
    if (current_group_id == at(i).GroupID())
      break;
  }

  if (i >= size()-1)
    return -1;
  else
    return at(i+1).GroupID();
}

void CPVRChannelGroups::AddGroup(const CStdString &name)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  Clear();
  if (!m_bRadio)
  {
    database->AddChannelGroup(name, -1);
    database->GetChannelGroupList(*this);
  }
  else
  {
    database->AddRadioChannelGroup(name, -1);
    database->GetRadioChannelGroupList(*this);
  }

  database->Close();
}

bool CPVRChannelGroups::RenameGroup(int GroupId, const CStdString &newname)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  Clear();
  if (!m_bRadio)
  {
    database->SetChannelGroupName(GroupId, newname);
    database->GetChannelGroupList(*this);
  }
  else
  {
    database->SetRadioChannelGroupName(GroupId, newname);
    database->GetRadioChannelGroupList(*this);
  }

  database->Close();
  return true;
}

bool CPVRChannelGroups::DeleteGroup(int GroupId)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  Clear();

  /* Set all channels with this group to undefined */
  if (!m_bRadio)
  {
    /* Delete the group inside Database */
    database->DeleteChannelGroup(GroupId);

    for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
    {
      if (PVRChannelsTV[i].GroupID() == GroupId)
      {
        PVRChannelsTV[i].SetGroupID(0);
        database->UpdateDBChannel(PVRChannelsTV[i]);
      }
    }

    /* Reload the group list */
    database->GetChannelGroupList(*this);
  }
  else
  {
    /* Delete the group inside Database */
    database->DeleteRadioChannelGroup(GroupId);

    for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
    {
      if (PVRChannelsRadio[i].GroupID() == GroupId)
      {
        PVRChannelsRadio[i].SetGroupID(0);
        database->UpdateDBChannel(PVRChannelsRadio[i]);
      }
    }

    /* Reload the group list */
    database->GetRadioChannelGroupList(*this);
  }

  database->Close();
  return true;
}

CStdString CPVRChannelGroups::GetGroupName(int GroupId)
{
  if (GroupId != -1)
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
  if (GroupName.IsEmpty() || GroupName == g_localizeStrings.Get(593) || GroupName == "all")
    return -1;

  for (unsigned int i = 0; i < size(); i++)
  {
    if (GroupName == at(i).GroupName())
      return at(i).GroupID();
  }
  return -1;
}

bool CPVRChannelGroups::ChannelToGroup(const CPVRChannel &channel, int GroupId)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  CPVRChannels *channels;
  if (!channel.IsRadio())
    channels = &PVRChannelsTV;
  else
    channels = &PVRChannelsRadio;

  channels->at(channel.ChannelNumber()-1).SetGroupID(GroupId);
  database->UpdateDBChannel(channels->at(channel.ChannelNumber()-1));

  database->Close();
  return true;
}

void CPVRChannelGroups::Clear()
{
  /* Clear all current present Channel groups inside list */
  erase(begin(), end());
  return;
}
