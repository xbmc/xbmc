/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroups.h"

#include <algorithm>

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupInternal.h"

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
  m_failedClientsForChannelGroups.clear();
}

bool CPVRChannelGroups::GetGroupsFromClients(void)
{
  if (! CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRMANAGER_SYNCCHANNELGROUPS))
    return true;

  return CServiceBroker::GetPVRManager().Clients()->GetChannelGroups(this, m_failedClientsForChannelGroups) == PVR_ERROR_NO_ERROR;
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

CFileItemPtr CPVRChannelGroups::GetByPath(const std::string &strInPath) const
{
  std::string strPath = strInPath;
  URIUtils::RemoveSlashAtEnd(strPath);
  std::string strCheckPath;

  CSingleLock lock(m_critSection);
  for (const auto& group: m_groups)
  {
    // check if the path matches
    strCheckPath = group->GetPath();
    if (URIUtils::PathHasParent(strPath, strCheckPath))
    {
      strPath.erase(0, strCheckPath.size());
      std::vector<std::string> split(StringUtils::Split(strPath, '_', 2));
      if (split.size() == 2)
      {
        const CPVRChannelPtr channel = group->GetByUniqueID(atoi(split[1].c_str()), CServiceBroker::GetPVRManager().Clients()->GetClientId(split[0]));
        if (channel)
          return CFileItemPtr(new CFileItem(channel));
      }
    }
  }

  // no match
  return CFileItemPtr(new CFileItem());
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

std::vector<CPVRChannelGroupPtr> CPVRChannelGroups::GetGroupsByChannel(const CPVRChannelPtr &channel, bool bExcludeHidden /* = false */) const
{
  std::vector<CPVRChannelGroupPtr> groups;

  CSingleLock lock(m_critSection);
  for (CPVRChannelGroupPtr group : m_groups)
  {
    if ((!bExcludeHidden || !group->IsHidden()) && group->IsGroupMember(channel))
      groups.push_back(group);
  }
  return groups;
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
  const CPVRChannelGroupPtr allGroup = GetGroupAll();

  for (const auto& group : m_groups)
  {
    // only delete the channel from non-system groups and if it was deleted from "all" group
    if (!group->IsInternalGroup() && !allGroup->IsGroupMember(channel))
      group->RemoveFromGroup(channel);
  }
}

bool CPVRChannelGroups::Update(bool bChannelsOnly /* = false */)
{
  bool bUpdateAllGroups = !bChannelsOnly && CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRMANAGER_SYNCCHANNELGROUPS);
  bool bReturn(true);

  // sync groups
  if (bUpdateAllGroups)
    GetGroupsFromClients();

  // sync channels in groups
  std::vector<CPVRChannelGroupPtr> groups;
  {
    CSingleLock lock(m_critSection);
    groups = m_groups;
  }

  for (const auto &group : groups)
  {
    if (bUpdateAllGroups || group->IsInternalGroup())
      bReturn = group->Update() && bReturn;
  }

  // persist changes
  return PersistAll() && bReturn;
}

bool CPVRChannelGroups::LoadUserDefinedChannelGroups(void)
{
  bool bSyncWithBackends = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRMANAGER_SYNCCHANNELGROUPS);

  CSingleLock lock(m_critSection);

  // load groups from the backends if the option is enabled
  int iSize = m_groups.size();
  if (bSyncWithBackends)
  {
    GetGroupsFromClients();
    CLog::LogFC(LOGDEBUG, LOGPVR, "%d new user defined %s channel groups fetched from clients", (m_groups.size() - iSize), m_bRadio ? "radio" : "TV");
  }
  else
    CLog::LogFC(LOGDEBUG, LOGPVR, "'sync channelgroups' is disabled; skipping groups from clients");

  std::vector<CPVRChannelGroupPtr> emptyGroups;

  // load group members
  for (std::vector<CPVRChannelGroupPtr>::iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    // load only user defined groups, as internal group is already loaded
    if (!(*it)->IsInternalGroup())
    {
      if (!(*it)->Load())
      {
        CLog::LogFC(LOGDEBUG, LOGPVR, "Failed to load user defined channel group '%s'", (*it)->GroupName().c_str());
        return false;
      }

      // remove empty groups when sync with backend is enabled
      if (bSyncWithBackends && (*it)->Size() == 0)
        emptyGroups.push_back(*it);
    }
  }

  for (std::vector<CPVRChannelGroupPtr>::iterator it = emptyGroups.begin(); it != emptyGroups.end(); ++it)
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting empty channel group '%s'", (*it)->GroupName().c_str());
    DeleteGroup(*(*it));
  }

  // persist changes if we fetched groups from the backends
  return bSyncWithBackends ? PersistAll() : true;
}

bool CPVRChannelGroups::Load(void)
{
  const CPVRDatabasePtr database(CServiceBroker::GetPVRManager().GetTVDatabase());
  if (!database)
    return false;

  CSingleLock lock(m_critSection);

  // remove previous contents
  Clear();

  CLog::LogFC(LOGDEBUG, LOGPVR, "Loading all %s channel groups", m_bRadio ? "radio" : "TV");

  // create the internal channel group
  CPVRChannelGroupPtr internalGroup = CPVRChannelGroupPtr(new CPVRChannelGroupInternal(m_bRadio));
  m_groups.push_back(internalGroup);

  // load groups from the database
  database->Get(*this);
  CLog::LogFC(LOGDEBUG, LOGPVR, "%d %s groups fetched from the database", m_groups.size(), m_bRadio ? "radio" : "TV");

  // load channels of internal group
  if (!internalGroup->Load())
  {
    CLog::LogF(LOGERROR, "Failed to load 'all channels' group");
    return false;
  }

  // load the other groups from the database
  if (!LoadUserDefinedChannelGroups())
  {
    CLog::LogF(LOGERROR, "Failed to load user defined channel groups");
    return false;
  }

  // set the last played group as selected group at startup
  CPVRChannelGroupPtr lastPlayedGroup = GetLastPlayedGroup();
  SetSelectedGroup(lastPlayedGroup ? lastPlayedGroup : internalGroup);

  CLog::LogFC(LOGDEBUG, LOGPVR, "%d %s channel groups loaded", m_groups.size(), m_bRadio ? "radio" : "TV");

  // need at least 1 group
  return m_groups.size() > 0;
}

bool CPVRChannelGroups::PersistAll(void)
{
  bool bReturn(true);
  CLog::LogFC(LOGDEBUG, LOGPVR, "Persisting all channel group changes");

  CSingleLock lock(m_critSection);
  for (std::vector<CPVRChannelGroupPtr>::iterator it = m_groups.begin(); it != m_groups.end(); ++it)
    bReturn &= (*it)->Persist();

  return bReturn;
}

CPVRChannelGroupPtr CPVRChannelGroups::GetGroupAll(void) const
{
  CSingleLock lock(m_critSection);
  if (!m_groups.empty())
    return m_groups.front();

  return CPVRChannelGroupPtr();
}

CPVRChannelGroupPtr CPVRChannelGroups::GetLastGroup(void) const
{
  CSingleLock lock(m_critSection);
  if (!m_groups.empty())
    return m_groups.back();

  return CPVRChannelGroupPtr();
}

CPVRChannelGroupPtr CPVRChannelGroups::GetLastPlayedGroup(int iChannelID /* = -1 */) const
{
  CPVRChannelGroupPtr group;

  CSingleLock lock(m_critSection);
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    if ((*it)->LastWatched() > 0 && (!group || (*it)->LastWatched() > group->LastWatched()) &&
        (iChannelID == -1 || (iChannelID >= 0 && (*it)->IsGroupMember(iChannelID))) && !(*it)->IsHidden())
      group = (*it);
  }

  return group;
}

std::vector<CPVRChannelGroupPtr> CPVRChannelGroups::GetMembers(bool bExcludeHidden /* = false */) const
{
  std::vector<CPVRChannelGroupPtr> groups;

  CSingleLock lock(m_critSection);
  for (CPVRChannelGroupPtr group : m_groups)
  {
    if (!bExcludeHidden || !group->IsHidden())
      groups.push_back(group);
  }
  return groups;
}

int CPVRChannelGroups::GetGroupList(CFileItemList* results, bool bExcludeHidden /* = false */) const
{
  int iReturn(0);

  CSingleLock lock(m_critSection);
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    // exclude hidden groups if desired
    if (bExcludeHidden && (*it)->IsHidden())
      continue;

    CFileItemPtr group(new CFileItem((*it)->GetPath(), true));
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

void CPVRChannelGroups::SetSelectedGroup(const CPVRChannelGroupPtr &group)
{
  CSingleLock lock(m_critSection);
  m_selectedGroup = group;
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
    CLog::LogF(LOGERROR, "Internal channel group cannot be deleted");
    return false;
  }

  bool bFound(false);
  CPVRChannelGroupPtr playingGroup;

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
          playingGroup = GetGroupAll();

        it = m_groups.erase(it);
        bFound = true;
      }
      else
      {
        ++it;
      }
    }
  }

  if (playingGroup)
    CServiceBroker::GetPVRManager().SetPlayingGroup(playingGroup);

  if (group.GroupID() > 0)
  {
    // delete the group from the database
    const CPVRDatabasePtr database(CServiceBroker::GetPVRManager().GetTVDatabase());
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
