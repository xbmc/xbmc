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

#include "PVRChannelGroups.h"

#include "FileItem.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "URL.h"

#include "PVRChannelGroupInternal.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"

#include <algorithm>

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
  if (! CSettings::Get().GetBool("pvrmanager.syncchannelgroups"))
    return true;

  return g_PVRClients->GetChannelGroups(this) == PVR_ERROR_NO_ERROR;
}

bool CPVRChannelGroups::Update(const CPVRChannelGroup &group, bool bUpdateFromClient /* = false */)
{
  if (group.GroupName().empty() && group.GroupID() <= 0)
    return true;

  CPVRChannelGroupPtr updateGroup;
  {
    CSingleLock lock(m_critSection);

    // There can be only one internal group! Make sure we never push a new one!
    if (group.IsInternalGroup())
      updateGroup = GetGroupAll();

    // try to find the group by id
    if (!updateGroup && group.GroupID() > 0)
      updateGroup = GetById(group.GroupID());

    // try to find the group by name if we didn't find it yet
    if (!updateGroup)
      updateGroup = GetByName(group.GroupName());

    if (!updateGroup)
    {
      // create a new group if none was found. Copy the properties immediately 
      // so the group doesn't get flagged as "changed" further down.
      updateGroup = CPVRChannelGroupPtr(new CPVRChannelGroup(group.IsRadio(), group.GroupID(), group.GroupName()));
      m_groups.push_back(updateGroup);
    }

    updateGroup->SetRadio(group.IsRadio());
    updateGroup->SetGroupID(group.GroupID());
    updateGroup->SetGroupName(group.GroupName());
    updateGroup->SetGroupType(group.GroupType());
    updateGroup->SetPosition(group.GetPosition());

    // don't override properties we only store locally in our PVR database
    if (!bUpdateFromClient)
    {
      updateGroup->SetLastWatched(group.LastWatched());
      updateGroup->SetHidden(group.IsHidden());
    }
  }

  // sort groups
  SortGroups();

  // persist changes
  if (bUpdateFromClient)
    return updateGroup->Persist();

  return true;
}

void CPVRChannelGroups::SortGroups()
{
  CSingleLock lock(m_critSection);

  // check if one of the group holds a valid sort position
  std::vector<CPVRChannelGroupPtr>::iterator it = std::find_if(m_groups.begin(), m_groups.end(), [](const CPVRChannelGroupPtr &group) {
    return (group->GetPosition() > 0);
  });

  // sort by position if we found a valid sort position
  if (it != m_groups.end())
  {
    std::sort(m_groups.begin(), m_groups.end(), [](const CPVRChannelGroupPtr &group1, const CPVRChannelGroupPtr &group2) {
      return group1->GetPosition() < group2->GetPosition();
    });
  }
}

CFileItemPtr CPVRChannelGroups::GetByPath(const std::string &strPath) const
{
  // get the filename from curl
  CURL url(strPath);
  std::string strFileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(strFileName);

  std::string strCheckPath;
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    // check if the path matches
    strCheckPath = StringUtils::Format("channels/%s/%s/", (*it)->IsRadio() ? "radio" : "tv", (*it)->GroupName().c_str());
    if (StringUtils::StartsWith(strFileName, strCheckPath))
    {
      strFileName.erase(0, strCheckPath.length());
      std::vector<std::string> split(StringUtils::Split(strFileName, '_', 2));
      if (split.size() == 2)
      {
        CPVRChannelPtr channel((*it)->GetByUniqueID(atoi(split[1].c_str()), g_PVRClients->GetClientId(split[0])));
        if (channel)
          return CFileItemPtr(new CFileItem(channel));
      }
    }
  }

  // no match
  CFileItemPtr retVal(new CFileItem);
  return retVal;
}

CPVRChannelGroupPtr CPVRChannelGroups::GetById(int iGroupId) const
{
  CSingleLock lock(m_critSection);
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    if ((*it)->GroupID() == iGroupId)
      return *it;
  }

  CPVRChannelGroupPtr empty;
  return empty;
}

CPVRChannelGroupPtr CPVRChannelGroups::GetByName(const std::string &strName) const
{
  CSingleLock lock(m_critSection);
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    if ((*it)->GroupName() == strName)
      return *it;
  }

  CPVRChannelGroupPtr empty;
  return empty;
}

void CPVRChannelGroups::RemoveFromAllGroups(const CPVRChannelPtr &channel)
{
  CSingleLock lock(m_critSection);
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    // only delete the channel from non-system groups
    if (!(*it)->IsInternalGroup())
      (*it)->RemoveFromGroup(channel);
  }
}

bool CPVRChannelGroups::Update(bool bChannelsOnly /* = false */)
{
  bool bUpdateAllGroups = !bChannelsOnly && CSettings::Get().GetBool("pvrmanager.syncchannelgroups");
  bool bReturn(true);

  // sync groups
  if (bUpdateAllGroups)
    GetGroupsFromClients();

  // sync channels in groups
  {
    CSingleLock lock(m_critSection);
    for (std::vector<CPVRChannelGroupPtr>::iterator it = m_groups.begin(); it != m_groups.end(); ++it)
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
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = groups.m_groups.begin(); it != groups.m_groups.end(); ++it)
  {
    // check if this group is present in this container
    CPVRChannelGroupPtr existingGroup = GetByName((*it)->GroupName());

    // add it if not
    if (!existingGroup)
    {
      CPVRChannelGroupPtr newGroup(CPVRChannelGroupPtr(new CPVRChannelGroup()));
      newGroup->SetRadio(m_bRadio);
      newGroup->SetGroupName((*it)->GroupName());
      m_groups.push_back(newGroup);
    }
  }

  return true;
}

bool CPVRChannelGroups::LoadUserDefinedChannelGroups(void)
{
  bool bSyncWithBackends = CSettings::Get().GetBool("pvrmanager.syncchannelgroups");

  CSingleLock lock(m_critSection);

  // load groups from the backends if the option is enabled
  int iSize = m_groups.size();
  if (bSyncWithBackends)
  {
    GetGroupsFromClients();
    CLog::Log(LOGDEBUG, "PVR - %s - %" PRIuS" new user defined %s channel groups fetched from clients", __FUNCTION__, (m_groups.size() - iSize), m_bRadio ? "radio" : "TV");
  }
  else
    CLog::Log(LOGDEBUG, "PVR - %s - 'synchannelgroups' is disabled; skipping groups from clients", __FUNCTION__);

  std::vector<CPVRChannelGroupPtr> emptyGroups;

  // load group members
  for (std::vector<CPVRChannelGroupPtr>::iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    // load only user defined groups, as internal group is already loaded
    if (!(*it)->IsInternalGroup())
    {
      if (!(*it)->Load())
      {
        CLog::Log(LOGDEBUG, "PVR - %s - failed to load channel group '%s'", __FUNCTION__, (*it)->GroupName().c_str());
        return false;
      }

      // remove empty groups when sync with backend is enabled
      if (bSyncWithBackends && (*it)->Size() == 0)
        emptyGroups.push_back(*it);
    }
  }

  for (std::vector<CPVRChannelGroupPtr>::iterator it = emptyGroups.begin(); it != emptyGroups.end(); ++it)
  {
    CLog::Log(LOGDEBUG, "PVR - %s - deleting empty group '%s'", __FUNCTION__, (*it)->GroupName().c_str());
    DeleteGroup(*(*it));
  }

  // persist changes if we fetched groups from the backends
  return bSyncWithBackends ? PersistAll() : true;
}

bool CPVRChannelGroups::Load(void)
{
  CPVRDatabase *database = GetPVRDatabase();
  if (!database)
    return false;

  CSingleLock lock(m_critSection);

  // remove previous contents
  Clear();

  CLog::Log(LOGDEBUG, "PVR - %s - loading all %s channel groups", __FUNCTION__, m_bRadio ? "radio" : "TV");

  // create the internal channel group
  CPVRChannelGroupPtr internalGroup = CPVRChannelGroupPtr(new CPVRChannelGroupInternal(m_bRadio));
  m_groups.push_back(internalGroup);

  // load groups from the database
  database->Get(*this);
  CLog::Log(LOGDEBUG, "PVR - %s - %" PRIuS" %s groups fetched from the database", __FUNCTION__, m_groups.size(), m_bRadio ? "radio" : "TV");

  // load channels of internal group
  if (!internalGroup->Load())
  {
    CLog::Log(LOGERROR, "PVR - %s - failed to load channels", __FUNCTION__);
    return false;
  }

  // load the other groups from the database
  if (!LoadUserDefinedChannelGroups())
  {
    CLog::Log(LOGERROR, "PVR - %s - failed to load channel groups", __FUNCTION__);
    return false;
  }

  // set the last played group as selected group at startup
  CPVRChannelGroupPtr lastPlayedGroup = GetLastPlayedGroup();
  SetSelectedGroup(lastPlayedGroup ? lastPlayedGroup : internalGroup);

  CLog::Log(LOGDEBUG, "PVR - %s - %" PRIuS" %s channel groups loaded", __FUNCTION__, m_groups.size(), m_bRadio ? "radio" : "TV");

  // need at least 1 group
  return m_groups.size() > 0;
}

bool CPVRChannelGroups::PersistAll(void)
{
  bool bReturn(true);
  CLog::Log(LOGDEBUG, "PVR - %s - persisting all changes in channel groups", __FUNCTION__);

  CSingleLock lock(m_critSection);
  for (std::vector<CPVRChannelGroupPtr>::iterator it = m_groups.begin(); it != m_groups.end(); ++it)
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

CPVRChannelGroupPtr CPVRChannelGroups::GetLastPlayedGroup(int iChannelID /* = -1 */) const
{
  CSingleLock lock(m_critSection);

  CPVRChannelGroupPtr group;
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    if ((*it)->LastWatched() > 0 && (!group || (*it)->LastWatched() > group->LastWatched()) &&
        (iChannelID == -1 || (iChannelID >= 0 && (*it)->IsGroupMember(iChannelID))) && !(*it)->IsHidden())
      group = (*it);
  }

  return group;
}

std::vector<CPVRChannelGroupPtr> CPVRChannelGroups::GetMembers() const
{
  CSingleLock lock(m_critSection);
  std::vector<CPVRChannelGroupPtr> groups(m_groups.begin(), m_groups.end());
  return groups;
}

int CPVRChannelGroups::GetGroupList(CFileItemList* results, bool bExcludeHidden /* = false */) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  std::string strPath;
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    // exclude hidden groups if desired
    if (bExcludeHidden && (*it)->IsHidden())
      continue;

    strPath = StringUtils::Format("pvr://channels/%s/%s/", m_bRadio ? "radio" : "tv", (*it)->GroupName().c_str());
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
    for (std::vector<CPVRChannelGroupPtr>::const_reverse_iterator it = m_groups.rbegin(); it != m_groups.rend(); ++it)
    {
      // return this entry
      if (bReturnNext && !(*it)->IsHidden())
        return *it;

      // return the next entry
      if ((*it)->GroupID() == group.GroupID())
        bReturnNext = true;
    }

    // no match return last visible group
    for (std::vector<CPVRChannelGroupPtr>::const_reverse_iterator it = m_groups.rbegin(); it != m_groups.rend(); ++it)
    {
      if (!(*it)->IsHidden())
        return *it;
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
    for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
    {
      // return this entry
      if (bReturnNext && !(*it)->IsHidden())
        return *it;

      // return the next entry
      if ((*it)->GroupID() == group.GroupID())
        bReturnNext = true;
    }

    // no match return first visible group
    for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
    {
      if (!(*it)->IsHidden())
        return *it;
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

bool CPVRChannelGroups::AddGroup(const std::string &strName)
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
      group = CPVRChannelGroupPtr(new CPVRChannelGroup());
      group->SetRadio(m_bRadio);
      group->SetGroupName(strName);
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
    for (std::vector<CPVRChannelGroupPtr>::iterator it = m_groups.begin(); !bFound && it != m_groups.end();)
    {
      if (*(*it) == group || (group.GroupID() > 0 && (*it)->GroupID() == group.GroupID()))
      {
        // update the selected group in the gui if it's deleted
        CPVRChannelGroupPtr selectedGroup = GetSelectedGroup();
        if (selectedGroup && *selectedGroup == group)
          g_PVRManager.SetPlayingGroup(GetGroupAll());

        it = m_groups.erase(it);
        bFound = true;
      }
      else
      {
        ++it;
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

bool CPVRChannelGroups::CreateChannelEpgs(void)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);
  for (std::vector<CPVRChannelGroupPtr>::iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    /* Only create EPGs for the internatl groups */
    if ((*it)->IsInternalGroup())
      bReturn = (*it)->CreateChannelEpgs();
  }
  return bReturn;
}
