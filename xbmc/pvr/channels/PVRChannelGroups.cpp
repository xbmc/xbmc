/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PVRChannelGroups.h"

#include "FileItem.h"
#include "settings/GUISettings.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"
#include "URL.h"
#include "filesystem/File.h"

#include "PVRChannelGroupInternal.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"

using namespace PVR;

CPVRChannelGroups::CPVRChannelGroups(bool bRadio) :
    m_bRadio(bRadio)
{
}

CPVRChannelGroups::~CPVRChannelGroups(void)
{
  Clear();
}

void CPVRChannelGroups::Clear(void)
{
  CSingleLock lock(m_critSection);
  m_groups.clear();
}

bool CPVRChannelGroups::GetGroupsFromClients(void)
{
  if (! g_guiSettings.GetBool("pvrmanager.syncchannelgroups"))
    return true;

  return g_PVRClients->GetChannelGroups(this) == PVR_ERROR_NO_ERROR;
}

bool CPVRChannelGroups::Update(const CPVRChannelGroup &group, bool bSaveInDb)
{
  if (group.GroupName().IsEmpty() && group.GroupID() <= 0)
    return true;

  CPVRChannelGroupPtr updateGroup;
  {
    CSingleLock lock(m_critSection);
    // try to find the group by id
    if (group.GroupID() > 0)
      updateGroup = GetById(group.GroupID());

    // try to find the group by name if we didn't find it yet
    if (!updateGroup)
      updateGroup = GetByName(group.GroupName());

    if (!updateGroup)
    {
      // create a new group if none was found
      updateGroup = CPVRChannelGroupPtr(new CPVRChannelGroup(m_bRadio, group.GroupID(), group.GroupName()));
      updateGroup->SetGroupType(group.GroupType());
      m_groups.push_back(updateGroup);
    }
    else
    {
      // update existing group
      updateGroup->SetGroupID(group.GroupID());
      updateGroup->SetGroupName(group.GroupName());
      updateGroup->SetGroupType(group.GroupType());
    }
  }

  // persist changes
  if (bSaveInDb && updateGroup)
    return updateGroup->Persist();

  return true;
}

CFileItemPtr CPVRChannelGroups::GetByPath(const CStdString &strPath) const
{
  // get the filename from curl
  CURL url(strPath);
  CStdString strFileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(strFileName);

  CStdString strCheckPath;
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); it++)
  {
    // check if the path matches
    strCheckPath.Format("channels/%s/%s/", (*it)->IsRadio() ? "radio" : "tv", (*it)->GroupName().c_str());
    if (strFileName.Left(strCheckPath.length()) == strCheckPath)
    {
      strFileName.erase(0, strCheckPath.length());
      return (*it)->GetByIndex(atoi(strFileName.c_str()));
    }
  }

  // no match
  CFileItemPtr retVal(new CFileItem);
  return retVal;
}

CPVRChannelGroupPtr CPVRChannelGroups::GetById(int iGroupId) const
{
  CSingleLock lock(m_critSection);
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); it++)
  {
    if ((*it)->GroupID() == iGroupId)
      return *it;
  }

  CPVRChannelGroupPtr empty;
  return empty;
}

CPVRChannelGroupPtr CPVRChannelGroups::GetByName(const CStdString &strName) const
{
  CSingleLock lock(m_critSection);
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); it++)
  {
    if ((*it)->GroupName().Equals(strName))
      return *it;
  }

  CPVRChannelGroupPtr empty;
  return empty;
}

void CPVRChannelGroups::RemoveFromAllGroups(const CPVRChannel &channel)
{
  CSingleLock lock(m_critSection);
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); it++)
  {
    // only delete the channel from non-system groups
    if (!(*it)->IsInternalGroup())
      (*it)->RemoveFromGroup(channel);
  }
}

bool CPVRChannelGroups::Update(bool bChannelsOnly /* = false */)
{
  bool bUpdateAllGroups = !bChannelsOnly && g_guiSettings.GetBool("pvrmanager.syncchannelgroups");
  bool bReturn(true);

  // sync groups
  if (bUpdateAllGroups)
    GetGroupsFromClients();

  // sync channels in groups
  {
    CSingleLock lock(m_critSection);
    for (std::vector<CPVRChannelGroupPtr>::iterator it = m_groups.begin(); it != m_groups.end(); it++)
    {
      if (bUpdateAllGroups || (*it)->IsInternalGroup())
        bReturn = (*it)->Update() && bReturn;
    }
  }

  // persist changes
  return PersistAll() && bReturn;
}

bool CPVRChannelGroups::UpdateGroupsEntries(const CPVRChannelGroups &groups)
{
  CSingleLock lock(m_critSection);

  // go through groups list and check for deleted groups
  for (int iGroupPtr = m_groups.size() - 1; iGroupPtr > 0; iGroupPtr--)
  {
    CPVRChannelGroup existingGroup(*m_groups.at(iGroupPtr));
    CPVRChannelGroupPtr group = groups.GetByName(existingGroup.GroupName());
    // user defined group wasn't found
    if (existingGroup.GroupType() == PVR_GROUP_TYPE_DEFAULT && !group)
    {
      CLog::Log(LOGDEBUG, "PVR - %s - user defined group %s with id '%u' does not exist on the client anymore; deleting it", __FUNCTION__, existingGroup.GroupName().c_str(), existingGroup.GroupID());
      DeleteGroup(*m_groups.at(iGroupPtr));
    }
  }

  // go through the groups list and check for new groups
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = groups.m_groups.begin(); it != groups.m_groups.end(); it++)
  {
    // check if this group is present in this container
    CPVRChannelGroupPtr existingGroup = GetByName((*it)->GroupName());

    // add it if not
    if (!existingGroup)
      m_groups.push_back(CPVRChannelGroupPtr(new CPVRChannelGroup(m_bRadio, -1, (*it)->GroupName())));
  }

  return true;
}

bool CPVRChannelGroups::LoadUserDefinedChannelGroups(void)
{
  CPVRDatabase *database = GetPVRDatabase();
  if (!database)
    return false;

  bool bSyncWithBackends = g_guiSettings.GetBool("pvrmanager.syncchannelgroups");

  CSingleLock lock(m_critSection);

  // load the other groups from the database
  int iSize = m_groups.size();
  database->Get(*this);
  CLog::Log(LOGDEBUG, "PVR - %s - %d user defined %s channel groups fetched from the database", __FUNCTION__, (int) (m_groups.size() - iSize), m_bRadio ? "radio" : "TV");

  // load groups from the backends if the option is enabled
  iSize = m_groups.size();
  if (bSyncWithBackends)
  {
    GetGroupsFromClients();
    CLog::Log(LOGDEBUG, "PVR - %s - %d new user defined %s channel groups fetched from clients", __FUNCTION__, (int) (m_groups.size() - iSize), m_bRadio ? "radio" : "TV");
  }
  else
    CLog::Log(LOGDEBUG, "PVR - %s - 'synchannelgroups' is disabled; skipping groups from clients", __FUNCTION__);

  std::vector<CPVRChannelGroupPtr> emptyGroups;

  // load group members
  for (std::vector<CPVRChannelGroupPtr>::iterator it = m_groups.begin(); it != m_groups.end(); it++)
  {
    (*it)->Load();

    // remove empty groups when sync with backend is enabled
    if (bSyncWithBackends && !(*it)->IsInternalGroup() && (*it)->Size() == 0)
      emptyGroups.push_back(*it);
  }

  for (std::vector<CPVRChannelGroupPtr>::iterator it = emptyGroups.begin(); it != emptyGroups.end(); it++)
  {
    CLog::Log(LOGDEBUG, "PVR - %s - deleting empty group '%s'", __FUNCTION__, (*it)->GroupName().c_str());
    DeleteGroup(*(*it));
  }

  // persist changes if we fetched groups from the backends
  return bSyncWithBackends ? PersistAll() : true;
}

bool CPVRChannelGroups::Load(void)
{
  CSingleLock lock(m_critSection);

  // remove previous contents
  Clear();

  CLog::Log(LOGDEBUG, "PVR - %s - loading all %s channel groups", __FUNCTION__, m_bRadio ? "radio" : "TV");

  // create and load the internal channel group
  CPVRChannelGroupPtr internalChannels = CPVRChannelGroupPtr(new CPVRChannelGroupInternal(m_bRadio));
  m_groups.push_back(internalChannels);
  internalChannels->Load();

  // load the other groups from the database
  LoadUserDefinedChannelGroups();

  // set the internal group as selected at startup
  internalChannels->SetSelectedGroup(true);
  internalChannels->Renumber();
  m_selectedGroup = internalChannels;

  CLog::Log(LOGDEBUG, "PVR - %s - %d %s channel groups loaded", __FUNCTION__, (int) m_groups.size(), m_bRadio ? "radio" : "TV");

  // need at least 1 group
  return m_groups.size() > 0;
}

bool CPVRChannelGroups::PersistAll(void)
{
  bool bReturn(true);
  CLog::Log(LOGDEBUG, "PVR - %s - persisting all changes in channel groups", __FUNCTION__);

  CSingleLock lock(m_critSection);
  for (std::vector<CPVRChannelGroupPtr>::iterator it = m_groups.begin(); it != m_groups.end(); it++)
    bReturn &= (*it)->Persist();

  return bReturn;
}

CPVRChannelGroupPtr CPVRChannelGroups::GetGroupAll(void) const
{
  CSingleLock lock(m_critSection);
  if (m_groups.size() > 0)
    return m_groups.at(0);

  CPVRChannelGroupPtr empty;
  return empty;
}

CPVRChannelGroupPtr CPVRChannelGroups::GetLastGroup(void) const
{
  CSingleLock lock(m_critSection);
  if (m_groups.size() > 0)
    return m_groups.at(m_groups.size() - 1);

  CPVRChannelGroupPtr empty;
  return empty;
}

std::vector<CPVRChannelGroupPtr> CPVRChannelGroups::GetMembers() const
{
  CSingleLock lock(m_critSection);
  std::vector<CPVRChannelGroupPtr> groups(m_groups.begin(), m_groups.end());
  return groups;
}

int CPVRChannelGroups::GetGroupList(CFileItemList* results) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  CStdString strPath;
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); it++)
  {
    strPath.Format("channels/%s/%i", m_bRadio ? "radio" : "tv", (*it)->GroupID());
    CFileItemPtr group(new CFileItem(strPath, true));
    group->m_strTitle = (*it)->GroupName();
    group->SetLabel((*it)->GroupName());
    results->Add(group);
    ++iReturn;
  }

  return iReturn;
}

CPVRChannelGroupPtr CPVRChannelGroups::GetPreviousGroup(const CPVRChannelGroup &group) const
{
  bool bReturnNext(false);

  {
    CSingleLock lock(m_critSection);
    for (std::vector<CPVRChannelGroupPtr>::const_reverse_iterator it = m_groups.rbegin(); it != m_groups.rend(); it++)
    {
      // return this entry
      if (bReturnNext)
        return *it;

      // return the next entry
      if ((*it)->GroupID() == group.GroupID())
        bReturnNext = true;
    }
  }

  // no match
  return GetLastGroup();
}

CPVRChannelGroupPtr CPVRChannelGroups::GetNextGroup(const CPVRChannelGroup &group) const
{
  bool bReturnNext(false);

  {
    CSingleLock lock(m_critSection);
    for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); it++)
    {
      // return this entry
      if (bReturnNext)
        return *it;

      // return the next entry
      if ((*it)->GroupID() == group.GroupID())
        bReturnNext = true;
    }
  }

  // no match
  return GetFirstGroup();
}

CPVRChannelGroupPtr CPVRChannelGroups::GetSelectedGroup(void) const
{
  CSingleLock lock(m_critSection);
  return m_selectedGroup;
}

void CPVRChannelGroups::SetSelectedGroup(CPVRChannelGroupPtr group)
{
  // update the selected group
  {
    CSingleLock lock(m_critSection);
    if (m_selectedGroup)
      m_selectedGroup->SetSelectedGroup(false);
    m_selectedGroup = group;
    group->SetSelectedGroup(true);
  }

  // update the channel number cache
  group->Renumber();
}

bool CPVRChannelGroups::AddGroup(const CStdString &strName)
{
  bool bPersist(false);
  CPVRChannelGroupPtr group;

  {
    CSingleLock lock(m_critSection);

    // check if there's no group with the same name yet
    group = GetByName(strName);
    if (!group)
    {
      // create a new group
      group = CPVRChannelGroupPtr(new CPVRChannelGroup(m_bRadio, -1, strName));
      m_groups.push_back(group);
      bPersist = true;
    }
  }

  // persist in the db if a new group was added
  return bPersist ? group->Persist() : true;
}

bool CPVRChannelGroups::DeleteGroup(const CPVRChannelGroup &group)
{
  // don't delete internal groups
  if (group.IsInternalGroup())
  {
    CLog::Log(LOGERROR, "PVR - %s - cannot delete internal group '%s'", __FUNCTION__, group.GroupName().c_str());
    return false;
  }

  bool bFound(false);

  // delete the group in this container
  {
    CSingleLock lock(m_critSection);
    for (std::vector<CPVRChannelGroupPtr>::iterator it = m_groups.begin(); !bFound && it != m_groups.end(); it++)
    {
      if ((*it)->GroupID() == group.GroupID())
      {
        // update the selected group in the gui if it's deleted
        CPVRChannelGroupPtr selectedGroup = GetSelectedGroup();
        if (selectedGroup && *selectedGroup == group)
          g_PVRManager.SetPlayingGroup(GetGroupAll());

        m_groups.erase(it);
        bFound = true;
      }
    }
  }

  if (group.GroupID() > 0)
  {
    // delete the group from the database
    CPVRDatabase *database = GetPVRDatabase();
    return database ? database->Delete(group) : false;
  }
  return bFound;
}

void CPVRChannelGroups::FillGroupsGUI(int iWindowId, int iControlId) const
{
  int iListGroupPtr(0);
  int iSelectedGroupPtr(0);
  CPVRChannelGroupPtr selectedGroup = g_PVRManager.GetPlayingGroup(false);
  std::vector<CGUIMessage> messages;

  // fetch all groups
  {
    CSingleLock lock(m_critSection);
    for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); it++)
    {
      // skip empty groups
      if ((*it)->Size() == 0)
        continue;

      if ((*it)->GroupID() == selectedGroup->GroupID())
        iSelectedGroupPtr = iListGroupPtr;

      CGUIMessage msg(GUI_MSG_LABEL_ADD, iWindowId, iControlId, iListGroupPtr++);
      msg.SetLabel((*it)->GroupName());
      messages.push_back(msg);
    }
  }

  // send updates
  for (std::vector<CGUIMessage>::iterator it = messages.begin(); it != messages.end(); it++)
    g_windowManager.SendMessage(*it);

  // selected group
  CGUIMessage msgSel(GUI_MSG_ITEM_SELECT, iWindowId, iControlId, iSelectedGroupPtr);
  g_windowManager.SendMessage(msgSel);
}
