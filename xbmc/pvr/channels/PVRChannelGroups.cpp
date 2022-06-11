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
#include "pvr/addons/PVRClient.h"
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
#include <iterator>
#include <memory>
#include <mutex>
#include <numeric>
#include <string>
#include <vector>

using namespace PVR;

CPVRChannelGroups::CPVRChannelGroups(bool bRadio) :
    m_bRadio(bRadio)
{
}

CPVRChannelGroups::~CPVRChannelGroups()
{
  Unload();
}

void CPVRChannelGroups::Unload()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& group : m_groups)
    group->Unload();

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
    std::unique_lock<CCriticalSection> lock(m_critSection);

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
  std::unique_lock<CCriticalSection> lock(m_critSection);

  // check if one of the group holds a valid sort position
  const auto it = std::find_if(m_groups.cbegin(), m_groups.cend(),
                               [](const auto& group) { return (group->GetPosition() > 0); });

  // sort by position if we found a valid sort position
  if (it != m_groups.cend())
  {
    std::sort(m_groups.begin(), m_groups.end(), [](const auto& group1, const auto& group2) {
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
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto it = std::find_if(m_groups.cbegin(), m_groups.cend(), [iGroupId](const auto& group) {
    return group->GroupID() == iGroupId;
  });
  return (it != m_groups.cend()) ? (*it) : std::shared_ptr<CPVRChannelGroup>();
}

std::vector<std::shared_ptr<CPVRChannelGroup>> CPVRChannelGroups::GetGroupsByChannel(const std::shared_ptr<CPVRChannel>& channel, bool bExcludeHidden /* = false */) const
{
  std::vector<std::shared_ptr<CPVRChannelGroup>> groups;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  std::copy_if(m_groups.cbegin(), m_groups.cend(), std::back_inserter(groups),
               [bExcludeHidden, &channel](const auto& group) {
                 return (!bExcludeHidden || !group->IsHidden()) && group->IsGroupMember(channel);
               });
  return groups;
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetGroupByPath(const std::string& strInPath) const
{
  const CPVRChannelsPath path(strInPath);
  if (path.IsChannelGroup())
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    const auto it = std::find_if(m_groups.cbegin(), m_groups.cend(),
                                 [&path](const auto& group) { return group->GetPath() == path; });
    if (it != m_groups.cend())
      return (*it);
  }
  return {};
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetByName(const std::string& strName) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto it = std::find_if(m_groups.cbegin(), m_groups.cend(), [&strName](const auto& group) {
    return group->GroupName() == strName;
  });
  return (it != m_groups.cend()) ? (*it) : std::shared_ptr<CPVRChannelGroup>();
}

bool CPVRChannelGroups::HasValidDataForClients(
    const std::vector<std::shared_ptr<CPVRClient>>& clients) const
{
  return m_failedClientsForChannelGroups.empty() ||
         std::none_of(clients.cbegin(), clients.cend(),
                      [this](const std::shared_ptr<CPVRClient>& client) {
                        return std::find(m_failedClientsForChannelGroups.cbegin(),
                                         m_failedClientsForChannelGroups.cend(),
                                         client->GetID()) != m_failedClientsForChannelGroups.cend();
                      });
}

bool CPVRChannelGroups::UpdateFromClients(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                          bool bChannelsOnly /* = false */)
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
    CServiceBroker::GetPVRManager().Clients()->GetChannelGroups(clients, this,
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
    std::unique_lock<CCriticalSection> lock(m_critSection);
    groups = m_groups;
  }

  std::vector<std::shared_ptr<CPVRChannelGroup>> emptyGroups;

  for (const auto& group : groups)
  {
    if (bUpdateAllGroups || group->IsInternalGroup())
    {
      const int iMemberCount = group->Size();
      if (!group->UpdateFromClients(clients))
      {
        CLog::LogFC(LOGERROR, LOGPVR, "Failed to update channel group '{}'", group->GroupName());
        bReturn = false;
      }

      const int iChangedMembersCount = static_cast<int>(group->Size()) - iMemberCount;
      if (iChangedMembersCount > 0)
      {
        CLog::LogFC(LOGDEBUG, LOGPVR, "{} channel group members added to group '{}'",
                    iChangedMembersCount, group->GroupName());
      }
      else if (iChangedMembersCount < 0)
      {
        CLog::LogFC(LOGDEBUG, LOGPVR, "{} channel group members removed from group '{}'",
                    -iChangedMembersCount, group->GroupName());
      }
      else
      {
        // could still be changed if same amount of members was removed as was added, but too
        // complicated to calculate just for debug logging
      }
    }

    // remove empty groups if sync with backend is enabled and we have valid data from all clients
    if (bSyncWithBackends && group->Size() == 0 && !group->IsInternalGroup() &&
        HasValidDataForClients(clients) && group->HasValidDataForClients(clients))
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
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::accumulate(
      m_groups.cbegin(), m_groups.cend(), false, [](bool changed, const auto& group) {
        return group->UpdateChannelNumbersFromAllChannelsGroup() ? true : changed;
      });
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::CreateChannelGroup(
    int iType, const CPVRChannelsPath& path)
{
  if (iType == PVR_GROUP_TYPE_INTERNAL)
    return std::make_shared<CPVRChannelGroupInternal>(path);
  else
    return std::make_shared<CPVRChannelGroup>(path, GetGroupAll());
}

bool CPVRChannelGroups::LoadFromDatabase(const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  const std::shared_ptr<CPVRDatabase> database(CServiceBroker::GetPVRManager().GetTVDatabase());
  if (!database)
    return false;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  // Ensure we have an internal group. It is important that the internal group is created before
  // loading contents from database and that it gets inserted in front of m_groups. Look at
  // GetGroupAll() implementation to see why.
  if (m_groups.empty())
  {
    const auto internalGroup = std::make_shared<CPVRChannelGroupInternal>(m_bRadio);
    m_groups.emplace_back(internalGroup);
  }

  CLog::LogFC(LOGDEBUG, LOGPVR, "Loading all {} channel groups and members",
              m_bRadio ? "radio" : "TV");

  // load all channels from the database
  std::map<std::pair<int, int>, std::shared_ptr<CPVRChannel>> channels;
  database->Get(m_bRadio, clients, channels);
  CLog::LogFC(LOGDEBUG, LOGPVR, "Fetched {} {} channels from the database", channels.size(),
              m_bRadio ? "radio" : "TV");

  // load all groups from the database
  const int iLoaded = database->Get(*this);
  CLog::LogFC(LOGDEBUG, LOGPVR, "Fetched {} {} groups from the database", iLoaded,
              m_bRadio ? "radio" : "TV");

  // load all group members from the database
  for (const auto& group : m_groups)
  {
    if (!group->LoadFromDatabase(channels, clients))
    {
      CLog::LogFC(LOGERROR, LOGPVR,
                  "Failed to load members of {} channel group '{}' from the database",
                  m_bRadio ? "radio" : "TV", group->GroupName());
    }
  }

  // Hide empty groups
  for (auto it = m_groups.begin(); it != m_groups.end();)
  {
    if ((*it)->Size() == 0 && !(*it)->IsInternalGroup())
      it = m_groups.erase(it);
    else
      ++it;
  }

  return true;
}

bool CPVRChannelGroups::PersistAll()
{
  CLog::LogFC(LOGDEBUG, LOGPVR, "Persisting all channel group changes");

  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::accumulate(
      m_groups.cbegin(), m_groups.cend(), true,
      [](bool success, const auto& group) { return !group->Persist() ? false : success; });
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetGroupAll() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (!m_groups.empty())
    return m_groups.front();

  return std::shared_ptr<CPVRChannelGroup>();
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetLastGroup() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (!m_groups.empty())
    return m_groups.back();

  return std::shared_ptr<CPVRChannelGroup>();
}

GroupMemberPair CPVRChannelGroups::GetLastAndPreviousToLastPlayedChannelGroupMember() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_groups.empty())
    return {};

  auto groups = m_groups;
  lock.unlock();

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
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::accumulate(
      m_groups.cbegin(), m_groups.cend(), std::shared_ptr<CPVRChannelGroup>{},
      [](const std::shared_ptr<CPVRChannelGroup>& last,
         const std::shared_ptr<CPVRChannelGroup>& group)
      {
        return group->LastOpened() > 0 && (!last || group->LastOpened() > last->LastOpened())
                   ? group
                   : last;
      });
}

std::vector<std::shared_ptr<CPVRChannelGroup>> CPVRChannelGroups::GetMembers(bool bExcludeHidden /* = false */) const
{
  std::vector<std::shared_ptr<CPVRChannelGroup>> groups;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  std::copy_if(
      m_groups.cbegin(), m_groups.cend(), std::back_inserter(groups),
      [bExcludeHidden](const auto& group) { return (!bExcludeHidden || !group->IsHidden()); });
  return groups;
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetPreviousGroup(const CPVRChannelGroup& group) const
{
  {
    bool bReturnNext = false;

    std::unique_lock<CCriticalSection> lock(m_critSection);
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
  {
    bool bReturnNext = false;

    std::unique_lock<CCriticalSection> lock(m_critSection);
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
    std::unique_lock<CCriticalSection> lock(m_critSection);

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
    std::unique_lock<CCriticalSection> lock(m_critSection);
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

  std::unique_lock<CCriticalSection> lock(m_critSection);
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
    std::unique_lock<CCriticalSection> lock(m_critSection);
    std::transform(m_groups.cbegin(), m_groups.cend(), std::back_inserter(urlsToCheck),
                   [](const auto& group) { return group->GetPath(); });
  }

  // kodi-generated thumbnail (see CPVRThumbLoader)
  const std::string path = StringUtils::Format("pvr://channels/{}/", IsRadio() ? "radio" : "tv");
  iCleanedImages += CPVRCachedImages::Cleanup({{"pvr", path}}, urlsToCheck, true);

  return iCleanedImages;
}
