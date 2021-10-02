/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroups.h"

#include "ServiceBroker.h"
#include "pvr/PVRCachedImages.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/channels/PVRChannelGroupMember.h"
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

bool CPVRChannelGroups::Update(const std::shared_ptr<CPVRChannelGroup>& group,
                               bool bUpdateFromClient /* = false */)
{
  if (group->GroupName().empty() && group->GroupID() <= 0)
    return true;

  std::shared_ptr<CPVRChannelGroup> updateGroup;
  {
    CSingleLock lock(m_critSection);

    // There can be only one internal group! Make sure we never push a new one!
    if (group->IsInternalGroup())
      updateGroup = GetGroupAll();

    // try to find the group by id
    if (!updateGroup && group->GroupID() > 0)
      updateGroup = GetById(group->GroupID());

    // try to find the group by name if we didn't find it yet
    if (!updateGroup)
      updateGroup = GetByName(group->GroupName());

    if (updateGroup)
    {
      updateGroup->SetPath(group->GetPath());
      updateGroup->SetGroupID(group->GroupID());
      updateGroup->SetGroupType(group->GroupType());
      updateGroup->SetPosition(group->GetPosition());

      // don't override properties we only store locally in our PVR database
      if (!bUpdateFromClient)
      {
        updateGroup->SetLastWatched(group->LastWatched());
        updateGroup->SetHidden(group->IsHidden());
        updateGroup->SetLastOpened(group->LastOpened());
      }
    }
    else
    {
      updateGroup = group;
      m_groups.emplace_back(updateGroup);
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

std::shared_ptr<CPVRChannelGroupMember> CPVRChannelGroups::GetChannelGroupMemberByPath(
    const CPVRChannelsPath& path) const
{
  if (path.IsChannel())
  {
    const std::shared_ptr<CPVRChannelGroup> group = GetByName(path.GetGroupName());
    if (group)
      return group->GetByUniqueID(
          {CServiceBroker::GetPVRManager().Clients()->GetClientId(path.GetClientID()),
           path.GetChannelUID()});
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

bool CPVRChannelGroups::HasValidDataForAllClients() const
{
  return m_failedClientsForChannelGroups.empty();
}

bool CPVRChannelGroups::Update(bool bChannelsOnly /* = false */)
{
  bool bSyncWithBackends = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      CSettings::SETTING_PVRMANAGER_SYNCCHANNELGROUPS);
  bool bUpdateAllGroups = !bChannelsOnly && bSyncWithBackends;
  bool bReturn = true;

  // sync groups
  const int iSize = m_groups.size();
  if (bUpdateAllGroups)
  {
    // get channel groups from the clients
    CServiceBroker::GetPVRManager().Clients()->GetChannelGroups(this,
                                                                m_failedClientsForChannelGroups);
    CLog::LogFC(LOGDEBUG, LOGPVR, "{} new user defined {} channel groups fetched from clients",
                (m_groups.size() - iSize), m_bRadio ? "radio" : "TV");
  }
  else if (!bSyncWithBackends)
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "'sync channelgroups' is disabled; skipping groups from clients");
  }

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
      const int iMemberCount = group->Size();
      if (!group->Update())
      {
        CLog::LogFC(LOGERROR, LOGPVR, "Failed to update channel group '{}'", group->GroupName());
        bReturn = false;
      }

      if (group->Size() - iMemberCount > 0)
      {
        CLog::LogFC(LOGDEBUG, LOGPVR, "{} channel group members added from clients to group '{}'",
                    static_cast<int>(group->Size() - iMemberCount), group->GroupName());
      }
    }

    // remove empty groups if sync with backend is enabled and we have valid data from all clients
    if (bSyncWithBackends && !group->IsInternalGroup() && HasValidDataForAllClients() &&
        group->HasValidDataForAllClients() && group->Size() == 0)
    {
      emptyGroups.emplace_back(group);
    }

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
    DeleteGroup(group);
  }

  if (bChannelsOnly)
  {
    // changes in the all channels group may require resorting/renumbering of other groups.
    // if we updated all groups this already has been done while updating the single groups.
    UpdateChannelNumbersFromAllChannelsGroup();
  }

  CServiceBroker::GetPVRManager().PublishEvent(PVREvent::ChannelGroupsInvalidated);

  // persist changes
  return PersistAll() && bReturn;
}

bool CPVRChannelGroups::UpdateChannelNumbersFromAllChannelsGroup()
{
  CSingleLock lock(m_critSection);

  bool bChanged = false;
  for (auto& group : m_groups)
    bChanged |= group->UpdateChannelNumbersFromAllChannelsGroup();

  return bChanged;
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::CreateChannelGroup(
    int iType, const CPVRChannelsPath& path)
{
  if (iType == PVR_GROUP_TYPE_INTERNAL)
    return std::make_shared<CPVRChannelGroupInternal>(path);
  else
    return std::make_shared<CPVRChannelGroup>(path, GetGroupAll());
}

bool CPVRChannelGroups::LoadFromDb()
{
  const std::shared_ptr<CPVRDatabase> database(CServiceBroker::GetPVRManager().GetTVDatabase());
  if (!database)
    return false;

  CLog::LogFC(LOGDEBUG, LOGPVR, "Loading all {} channel groups and members",
              m_bRadio ? "radio" : "TV");

  // load all channels from the database
  std::map<std::pair<int, int>, std::shared_ptr<CPVRChannel>> channels;
  database->Get(m_bRadio, channels);
  CLog::LogFC(LOGDEBUG, LOGPVR, "Fetched {} {} channels from the database", channels.size(),
              m_bRadio ? "radio" : "TV");

  // load all groups from the database
  const int iLoaded = database->Get(*this);
  CLog::LogFC(LOGDEBUG, LOGPVR, "Fetched {} {} groups from the database", iLoaded,
              m_bRadio ? "radio" : "TV");

  // load all group members from the database
  for (const auto& group : m_groups)
  {
    if (!group->Load(channels))
    {
      CLog::LogFC(LOGERROR, LOGPVR, "Failed to load {} channel group '{}'",
                  m_bRadio ? "radio" : "TV", group->GroupName());
    }
  }
  return true;
}

bool CPVRChannelGroups::Load()
{
  {
    CSingleLock lock(m_critSection);

    // Remove previous contents
    Clear();

    // Ensure we have an internal group. It is important that the internal group is created before
    // loading contents from database and that it gets inserted in front of m_groups. Look at
    // GetGroupAll() implementation to see why.
    const auto internalGroup = std::make_shared<CPVRChannelGroupInternal>(m_bRadio);
    m_groups.emplace_back(internalGroup);

    // Load groups, group members and channels from database
    LoadFromDb();
  }

  // Load data from clients and sync with local data
  Update();

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

GroupMemberPair CPVRChannelGroups::GetLastAndPreviousToLastPlayedChannelGroupMember() const
{
  CSingleLock lock(m_critSection);
  if (m_groups.empty())
    return {};

  auto groups = m_groups;
  lock.Leave();

  std::sort(groups.begin(), groups.end(),
            [](const auto& a, const auto& b) { return a->LastWatched() > b->LastWatched(); });

  // Last is always 'first' of last played group.
  const GroupMemberPair members = groups[0]->GetLastAndPreviousToLastPlayedChannelGroupMember();
  std::shared_ptr<CPVRChannelGroupMember> last = members.first;

  // Previous to last is either 'second' of first group or 'first' of second group.
  std::shared_ptr<CPVRChannelGroupMember> previousToLast = members.second;
  if (groups.size() > 1 && groups[0]->LastWatched() && groups[1]->LastWatched() && members.second &&
      members.second->Channel()->LastWatched())
  {
    if (groups[1]->LastWatched() >= members.second->Channel()->LastWatched())
    {
      const GroupMemberPair membersPreviousToLastPlayedGroup =
          groups[1]->GetLastAndPreviousToLastPlayedChannelGroupMember();
      previousToLast = membersPreviousToLastPlayedGroup.first;
    }
  }

  return {last, previousToLast};
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
      group.reset(new CPVRChannelGroup(CPVRChannelsPath(m_bRadio, strName), GetGroupAll()));

      m_groups.push_back(group);
      bPersist = true;

      CServiceBroker::GetPVRManager().PublishEvent(PVREvent::ChannelGroupsInvalidated);
    }
  }

  // persist in the db if a new group was added
  return bPersist ? group->Persist() : true;
}

bool CPVRChannelGroups::DeleteGroup(const std::shared_ptr<CPVRChannelGroup>& group)
{
  // don't delete internal groups
  if (group->IsInternalGroup())
  {
    CLog::LogF(LOGERROR, "Internal channel group cannot be deleted");
    return false;
  }

  bool bFound(false);

  // delete the group in this container
  {
    CSingleLock lock(m_critSection);
    for (auto it = m_groups.begin(); it != m_groups.end(); ++it)
    {
      if (*it == group || (group->GroupID() > 0 && (*it)->GroupID() == group->GroupID()))
      {
        m_groups.erase(it);
        bFound = true;
        break;
      }
    }
  }

  if (bFound && group->GroupID() > 0)
  {
    // delete the group from the database
    group->Delete();
    CServiceBroker::GetPVRManager().PublishEvent(PVREvent::ChannelGroupsInvalidated);
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

int CPVRChannelGroups::CleanupCachedImages()
{
  int iCleanedImages = 0;

  // cleanup channels
  iCleanedImages += GetGroupAll()->CleanupCachedImages();

  // cleanup groups
  std::vector<std::string> urlsToCheck;
  {
    CSingleLock lock(m_critSection);
    for (const auto& group : m_groups)
    {
      urlsToCheck.emplace_back(group->GetPath());
    }
  }

  // kodi-generated thumbnail (see CPVRThumbLoader)
  const std::string path = StringUtils::Format("pvr://channels/{}/", IsRadio() ? "radio" : "tv");
  iCleanedImages += CPVRCachedImages::Cleanup({{"pvr", path}}, urlsToCheck, true);

  return iCleanedImages;
}
