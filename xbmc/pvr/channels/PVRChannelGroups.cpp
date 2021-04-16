/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroups.h"

#include "ServiceBroker.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/channels/PVRChannelsPath.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

using namespace PVR;

CPVRChannelGroups::CPVRChannelGroups(bool bRadio) :
    m_bRadio(bRadio)
{
}

CPVRChannelGroups::~CPVRChannelGroups()
{
  Clear();
}

void CPVRChannelGroups::Clear()
{
  CSingleLock lock(m_critSection);
  m_groups.clear();
  m_failedClientsForChannelGroups.clear();
}

bool CPVRChannelGroups::GetGroupsFromClients()
{
  if (! CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRMANAGER_SYNCCHANNELGROUPS))
    return true;

  return CServiceBroker::GetPVRManager().Clients()->GetChannelGroups(this, m_failedClientsForChannelGroups) == PVR_ERROR_NO_ERROR;
}

bool CPVRChannelGroups::Update(const CPVRChannelGroup& group, bool bUpdateFromClient /* = false */)
{
  if (group.GroupName().empty() && group.GroupID() <= 0)
    return true;

  std::shared_ptr<CPVRChannelGroup> updateGroup;
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
      updateGroup.reset(new CPVRChannelGroup(CPVRChannelsPath(group.IsRadio(), group.GroupName()), group.GroupID(), GetGroupAll()));
      m_groups.push_back(updateGroup);
    }

    updateGroup->SetPath(group.GetPath());
    updateGroup->SetGroupID(group.GroupID());
    updateGroup->SetGroupType(group.GroupType());
    updateGroup->SetPosition(group.GetPosition());

    // don't override properties we only store locally in our PVR database
    if (!bUpdateFromClient)
    {
      updateGroup->SetLastWatched(group.LastWatched());
      updateGroup->SetHidden(group.IsHidden());
      updateGroup->SetLastOpened(group.LastOpened());
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
  std::vector<std::shared_ptr<CPVRChannelGroup>>::iterator it = std::find_if(m_groups.begin(), m_groups.end(), [](const std::shared_ptr<CPVRChannelGroup>& group) {
    return (group->GetPosition() > 0);
  });

  // sort by position if we found a valid sort position
  if (it != m_groups.end())
  {
    std::sort(m_groups.begin(), m_groups.end(), [](const std::shared_ptr<CPVRChannelGroup>& group1, const std::shared_ptr<CPVRChannelGroup>& group2) {
      return group1->GetPosition() < group2->GetPosition();
    });
  }
}

std::shared_ptr<CPVRChannel> CPVRChannelGroups::GetByPath(const CPVRChannelsPath& path) const
{
  if (path.IsChannel())
  {
    const std::shared_ptr<CPVRChannelGroup> group = GetByName(path.GetGroupName());
    if (group)
      return group->GetByUniqueID(path.GetChannelUID(),
                                  CServiceBroker::GetPVRManager().Clients()->GetClientId(path.GetClientID()));
  }

  return {};
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetById(int iGroupId) const
{
  CSingleLock lock(m_critSection);
  for (std::vector<std::shared_ptr<CPVRChannelGroup>>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    if ((*it)->GroupID() == iGroupId)
      return *it;
  }

  std::shared_ptr<CPVRChannelGroup> empty;
  return empty;
}

std::vector<std::shared_ptr<CPVRChannelGroup>> CPVRChannelGroups::GetGroupsByChannel(const std::shared_ptr<CPVRChannel>& channel, bool bExcludeHidden /* = false */) const
{
  std::vector<std::shared_ptr<CPVRChannelGroup>> groups;

  CSingleLock lock(m_critSection);
  for (const std::shared_ptr<CPVRChannelGroup>& group : m_groups)
  {
    if ((!bExcludeHidden || !group->IsHidden()) && group->IsGroupMember(channel))
      groups.push_back(group);
  }
  return groups;
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetGroupByPath(const std::string& strInPath) const
{
  const CPVRChannelsPath path(strInPath);
  if (path.IsChannelGroup())
  {
    CSingleLock lock(m_critSection);
    for (const auto& group : m_groups)
    {
      if (group->GetPath() == path)
        return group;
    }
  }
  return {};
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetByName(const std::string& strName) const
{
  CSingleLock lock(m_critSection);
  for (std::vector<std::shared_ptr<CPVRChannelGroup>>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    if ((*it)->GroupName() == strName)
      return *it;
  }

  std::shared_ptr<CPVRChannelGroup> empty;
  return empty;
}

void CPVRChannelGroups::RemoveFromAllGroups(const std::vector<std::shared_ptr<CPVRChannel>>& channelsToRemove)
{
  for (const auto& channel : channelsToRemove)
  {
    // remove this channel from all non-system groups
    RemoveFromAllGroups(channel);
  }
}

void CPVRChannelGroups::RemoveFromAllGroups(const std::shared_ptr<CPVRChannel>& channel)
{
  CSingleLock lock(m_critSection);
  const std::shared_ptr<CPVRChannelGroup> allGroup = GetGroupAll();

  for (const auto& group : m_groups)
  {
    // only delete the channel from non-system groups and if it was deleted from "all" group
    if (!group->IsInternalGroup() && !allGroup->IsGroupMember(channel))
      group->RemoveFromGroup(channel);
  }
}

bool CPVRChannelGroups::Update(bool bChannelsOnly /* = false */)
{
  bool bSyncWithBackends = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      CSettings::SETTING_PVRMANAGER_SYNCCHANNELGROUPS);
  bool bUpdateAllGroups = !bChannelsOnly && bSyncWithBackends;
  bool bReturn = true;

  // sync groups
  if (bUpdateAllGroups)
    GetGroupsFromClients();

  // sync channels in groups
  std::vector<std::shared_ptr<CPVRChannelGroup>> groups;
  {
    CSingleLock lock(m_critSection);
    groups = m_groups;
  }

  std::vector<std::shared_ptr<CPVRChannelGroup>> emptyGroups;

  for (const auto& group : groups)
  {
    if (bUpdateAllGroups || group->IsInternalGroup())
    {
      std::vector<std::shared_ptr<CPVRChannel>> channelsToRemove;
      bReturn = group->Update(channelsToRemove) && bReturn;
      RemoveFromAllGroups(channelsToRemove);
    }

    // remove empty groups when sync with backend is enabled
    if (bSyncWithBackends && !group->IsInternalGroup() && group->Size() == 0)
      emptyGroups.emplace_back(group);

    if (bReturn && group == m_selectedGroup)
      UpdateSelectedGroup();

    if (bReturn &&
        group->IsInternalGroup() &&
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bPVRChannelIconsAutoScan)
    {
      CServiceBroker::GetPVRManager().TriggerSearchMissingChannelIcons(group);
    }
  }

  for (const auto& group : emptyGroups)
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting empty channel group '{}'", group->GroupName());
    DeleteGroup(*group);
  }

  CServiceBroker::GetPVRManager().PublishEvent(PVREvent::ChannelGroupsInvalidated);

  // persist changes
  return PersistAll() && bReturn;
}

bool CPVRChannelGroups::PropagateChannelNumbersAndPersist()
{
  CSingleLock lock(m_critSection);

  bool bChanged = false;
  for (auto& group : m_groups)
    bChanged = group->UpdateChannelNumbersFromAllChannelsGroup();

  m_selectedGroup->UpdateChannelNumbers();

  return bChanged;
}

bool CPVRChannelGroups::LoadUserDefinedChannelGroups()
{
  bool bSyncWithBackends = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRMANAGER_SYNCCHANNELGROUPS);

  CSingleLock lock(m_critSection);

  // load groups from the backends if the option is enabled
  int iSize = m_groups.size();
  if (bSyncWithBackends)
  {
    GetGroupsFromClients();
    CLog::LogFC(LOGDEBUG, LOGPVR, "{} new user defined {} channel groups fetched from clients",
                (m_groups.size() - iSize), m_bRadio ? "radio" : "TV");
  }
  else
    CLog::LogFC(LOGDEBUG, LOGPVR, "'sync channelgroups' is disabled; skipping groups from clients");

  std::vector<std::shared_ptr<CPVRChannelGroup>> emptyGroups;

  // load group members
  for (std::vector<std::shared_ptr<CPVRChannelGroup>>::iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    // load only user defined groups, as internal group is already loaded
    if (!(*it)->IsInternalGroup())
    {
      std::vector<std::shared_ptr<CPVRChannel>> channelsToRemove;
      if (!(*it)->Load(channelsToRemove))
      {
        CLog::LogFC(LOGDEBUG, LOGPVR, "Failed to load user defined channel group '{}'",
                    (*it)->GroupName());
        return false;
      }

      RemoveFromAllGroups(channelsToRemove);

      // remove empty groups when sync with backend is enabled
      if (bSyncWithBackends && (*it)->Size() == 0)
        emptyGroups.push_back(*it);
    }
  }

  for (std::vector<std::shared_ptr<CPVRChannelGroup>>::iterator it = emptyGroups.begin(); it != emptyGroups.end(); ++it)
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting empty channel group '{}'", (*it)->GroupName());
    DeleteGroup(*(*it));
  }

  // persist changes if we fetched groups from the backends
  return bSyncWithBackends ? PersistAll() : true;
}

bool CPVRChannelGroups::Load()
{
  const std::shared_ptr<CPVRDatabase> database(CServiceBroker::GetPVRManager().GetTVDatabase());
  if (!database)
    return false;

  CSingleLock lock(m_critSection);

  // remove previous contents
  Clear();

  CLog::LogFC(LOGDEBUG, LOGPVR, "Loading all {} channel groups", m_bRadio ? "radio" : "TV");

  // create the internal channel group
  std::shared_ptr<CPVRChannelGroup> internalGroup = std::shared_ptr<CPVRChannelGroup>(new CPVRChannelGroupInternal(m_bRadio));
  m_groups.push_back(internalGroup);

  // load groups from the database
  database->Get(*this);
  CLog::LogFC(LOGDEBUG, LOGPVR, "{} {} groups fetched from the database", m_groups.size(),
              m_bRadio ? "radio" : "TV");

  // load channels of internal group
  std::vector<std::shared_ptr<CPVRChannel>> channelsToRemove;
  if (!internalGroup->Load(channelsToRemove))
  {
    CLog::LogF(LOGERROR, "Failed to load 'all channels' group");
    return false;
  }

  RemoveFromAllGroups(channelsToRemove);

  // load the other groups from the database
  if (!LoadUserDefinedChannelGroups())
  {
    CLog::LogF(LOGERROR, "Failed to load user defined channel groups");
    return false;
  }

  // set the last opened group as selected group at startup
  std::shared_ptr<CPVRChannelGroup> lastOpenedGroup = GetLastOpenedGroup();
  SetSelectedGroup(lastOpenedGroup ? lastOpenedGroup : internalGroup);

  CLog::LogFC(LOGDEBUG, LOGPVR, "{} {} channel groups loaded", m_groups.size(),
              m_bRadio ? "radio" : "TV");

  // need at least 1 group
  return m_groups.size() > 0;
}

bool CPVRChannelGroups::PersistAll()
{
  bool bReturn(true);
  CLog::LogFC(LOGDEBUG, LOGPVR, "Persisting all channel group changes");

  CSingleLock lock(m_critSection);
  for (std::vector<std::shared_ptr<CPVRChannelGroup>>::iterator it = m_groups.begin(); it != m_groups.end(); ++it)
    bReturn &= (*it)->Persist();

  return bReturn;
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetGroupAll() const
{
  CSingleLock lock(m_critSection);
  if (!m_groups.empty())
    return m_groups.front();

  return std::shared_ptr<CPVRChannelGroup>();
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetLastGroup() const
{
  CSingleLock lock(m_critSection);
  if (!m_groups.empty())
    return m_groups.back();

  return std::shared_ptr<CPVRChannelGroup>();
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetLastPlayedGroup(int iChannelID /* = -1 */) const
{
  std::shared_ptr<CPVRChannelGroup> group;

  CSingleLock lock(m_critSection);
  for (std::vector<std::shared_ptr<CPVRChannelGroup>>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    if ((*it)->LastWatched() > 0 && (!group || (*it)->LastWatched() > group->LastWatched()) &&
        (iChannelID == -1 || (iChannelID >= 0 && (*it)->IsGroupMember(iChannelID))) && !(*it)->IsHidden())
      group = (*it);
  }

  return group;
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetLastOpenedGroup() const
{
  std::shared_ptr<CPVRChannelGroup> lastOpenedGroup;

  CSingleLock lock(m_critSection);
  for (const auto& group : m_groups)
  {
    if (group->LastOpened() > 0 &&
        (!lastOpenedGroup || group->LastOpened() > lastOpenedGroup->LastOpened()))
      lastOpenedGroup = group;
  }

  return lastOpenedGroup;
}

std::vector<std::shared_ptr<CPVRChannelGroup>> CPVRChannelGroups::GetMembers(bool bExcludeHidden /* = false */) const
{
  std::vector<std::shared_ptr<CPVRChannelGroup>> groups;

  CSingleLock lock(m_critSection);
  for (const std::shared_ptr<CPVRChannelGroup>& group : m_groups)
  {
    if (!bExcludeHidden || !group->IsHidden())
      groups.push_back(group);
  }
  return groups;
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetPreviousGroup(const CPVRChannelGroup& group) const
{
  bool bReturnNext(false);

  {
    CSingleLock lock(m_critSection);
    for (std::vector<std::shared_ptr<CPVRChannelGroup>>::const_reverse_iterator it = m_groups.rbegin(); it != m_groups.rend(); ++it)
    {
      // return this entry
      if (bReturnNext && !(*it)->IsHidden())
        return *it;

      // return the next entry
      if ((*it)->GroupID() == group.GroupID())
        bReturnNext = true;
    }

    // no match return last visible group
    for (std::vector<std::shared_ptr<CPVRChannelGroup>>::const_reverse_iterator it = m_groups.rbegin(); it != m_groups.rend(); ++it)
    {
      if (!(*it)->IsHidden())
        return *it;
    }
  }

  // no match
  return GetLastGroup();
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetNextGroup(const CPVRChannelGroup& group) const
{
  bool bReturnNext(false);

  {
    CSingleLock lock(m_critSection);
    for (std::vector<std::shared_ptr<CPVRChannelGroup>>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
    {
      // return this entry
      if (bReturnNext && !(*it)->IsHidden())
        return *it;

      // return the next entry
      if ((*it)->GroupID() == group.GroupID())
        bReturnNext = true;
    }

    // no match return first visible group
    for (std::vector<std::shared_ptr<CPVRChannelGroup>>::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
    {
      if (!(*it)->IsHidden())
        return *it;
    }
  }

  // no match
  return GetFirstGroup();
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetSelectedGroup() const
{
  CSingleLock lock(m_critSection);
  return m_selectedGroup;
}

void CPVRChannelGroups::SetSelectedGroup(const std::shared_ptr<CPVRChannelGroup>& selectedGroup)
{
  CSingleLock lock(m_critSection);
  m_selectedGroup = selectedGroup;
  m_selectedGroup->UpdateClientOrder();
  m_selectedGroup->UpdateChannelNumbers();

  for (auto& group : m_groups)
    group->SetSelectedGroup(group == m_selectedGroup);

  auto duration = std::chrono::system_clock::now().time_since_epoch();
  uint64_t tsMillis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  m_selectedGroup->SetLastOpened(tsMillis);
}

void CPVRChannelGroups::UpdateSelectedGroup()
{
  CSingleLock lock(m_critSection);
  m_selectedGroup->UpdateClientOrder();
  m_selectedGroup->UpdateChannelNumbers();
}

bool CPVRChannelGroups::AddGroup(const std::string& strName)
{
  bool bPersist(false);
  std::shared_ptr<CPVRChannelGroup> group;

  {
    CSingleLock lock(m_critSection);

    // check if there's no group with the same name yet
    group = GetByName(strName);
    if (!group)
    {
      // create a new group
      group.reset(new CPVRChannelGroup(CPVRChannelsPath(m_bRadio, strName), CPVRChannelGroup::INVALID_GROUP_ID, GetGroupAll()));

      m_groups.push_back(group);
      bPersist = true;

      CServiceBroker::GetPVRManager().PublishEvent(PVREvent::ChannelGroupsInvalidated);
    }
  }

  // persist in the db if a new group was added
  return bPersist ? group->Persist() : true;
}

bool CPVRChannelGroups::DeleteGroup(const CPVRChannelGroup& group)
{
  // don't delete internal groups
  if (group.IsInternalGroup())
  {
    CLog::LogF(LOGERROR, "Internal channel group cannot be deleted");
    return false;
  }

  bool bFound(false);
  std::shared_ptr<CPVRChannelGroup> playingGroup;

  // delete the group in this container
  {
    CSingleLock lock(m_critSection);
    for (std::vector<std::shared_ptr<CPVRChannelGroup>>::iterator it = m_groups.begin(); !bFound && it != m_groups.end();)
    {
      if (*(*it) == group || (group.GroupID() > 0 && (*it)->GroupID() == group.GroupID()))
      {
        // update the selected group in the gui if it's deleted
        std::shared_ptr<CPVRChannelGroup> selectedGroup = GetSelectedGroup();
        if (selectedGroup && *selectedGroup == group)
          playingGroup = GetGroupAll();

        it = m_groups.erase(it);
        bFound = true;

        CServiceBroker::GetPVRManager().PublishEvent(PVREvent::ChannelGroupsInvalidated);
      }
      else
      {
        ++it;
      }
    }
  }

  if (playingGroup)
    SetSelectedGroup(playingGroup);

  if (group.GroupID() > 0)
  {
    // delete the group from the database
    const std::shared_ptr<CPVRDatabase> database(CServiceBroker::GetPVRManager().GetTVDatabase());
    return database ? database->Delete(group) : false;
  }
  return bFound;
}

bool CPVRChannelGroups::HideGroup(const std::shared_ptr<CPVRChannelGroup>& group, bool bHide)
{
  bool bReturn = false;

  if (group)
  {
    if (group->SetHidden(bHide))
    {
      // state changed
      CServiceBroker::GetPVRManager().PublishEvent(PVREvent::ChannelGroupsInvalidated);
    }
    bReturn = true;
  }
  return bReturn;
}

bool CPVRChannelGroups::CreateChannelEpgs()
{
  bool bReturn(false);

  CSingleLock lock(m_critSection);
  for (std::vector<std::shared_ptr<CPVRChannelGroup>>::iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    /* Only create EPGs for the internal groups */
    if ((*it)->IsInternalGroup())
      bReturn = (*it)->CreateChannelEpgs();
  }
  return bReturn;
}
