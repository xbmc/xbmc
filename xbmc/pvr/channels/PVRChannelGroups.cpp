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
#include "pvr/addons/PVRClientUID.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupAllChannels.h"
#include "pvr/channels/PVRChannelGroupFromClient.h"
#include "pvr/channels/PVRChannelGroupFromUser.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelsPath.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
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

CPVRChannelGroups::CPVRChannelGroups(bool bRadio)
  : m_bRadio(bRadio), m_settings({CSettings::SETTING_PVRMANAGER_BACKENDCHANNELGROUPSORDER})
{
  m_settings.RegisterCallback(this);
}

CPVRChannelGroups::~CPVRChannelGroups()
{
  m_settings.UnregisterCallback(this);
  Unload();
}

void CPVRChannelGroups::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
  {
    CLog::LogF(LOGERROR, "No setting");
    return;
  }

  const std::string& settingId = setting->GetId();

  if (settingId == CSettings::SETTING_PVRMANAGER_BACKENDCHANNELGROUPSORDER)
  {
    SortGroups();
  }
}

void CPVRChannelGroups::Unload()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& group : m_groups)
    group->Unload();

  CServiceBroker::GetPVRManager().Events().Unsubscribe(this);
  m_isSubscribed = false;

  m_groups.clear();
  m_allChannelsGroup.reset();
  m_failedClientsForChannelGroups.clear();
}

bool CPVRChannelGroups::UpdateFromClient(const std::shared_ptr<CPVRChannelGroup>& group)
{
  CLog::LogFC(LOGDEBUG, LOGPVR, "Got channel group '{}' from client {}.", group->GroupName(),
              group->GetClientID());

  if (!Update(group, true))
    return false;

  SortGroups();
  return true;
}

bool CPVRChannelGroups::Update(const std::shared_ptr<CPVRChannelGroup>& group,
                               bool bUpdateFromClient /* = false */)
{
  if (group->GroupName().empty() && group->GroupID() <= 0)
    return true;

  std::shared_ptr<CPVRChannelGroup> updateGroup;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);

    // There can be only one all channels group! Make sure we never push a new one!
    if (group->IsChannelsOwner())
      updateGroup = GetGroupAll();

    // try to find the group by id
    if (!updateGroup && group->GroupID() > 0)
      updateGroup = GetById(group->GroupID());

    // try to find the group by name if we didn't find it yet
    if (!updateGroup)
    {
      //! @todo If a group was renamed in the backend, no chance to find it here! PVR API should
      //! be extended by a uuid for channel groups which never must change, not even after rename.
      const auto it = std::find_if(
          m_groups.cbegin(), m_groups.cend(), [&group, bUpdateFromClient](const auto& g) {
            return (bUpdateFromClient ? g->ClientGroupName() == group->ClientGroupName()
                                      : g->GroupName() == group->GroupName()) &&
                   (g->GetClientID() == PVR_GROUP_CLIENT_ID_UNKNOWN ||
                    g->GetClientID() == group->GetClientID());
          });
      if (it != m_groups.cend())
        updateGroup = *it;
    }

    if (updateGroup)
    {
      updateGroup->SetGroupID(group->GroupID());
      updateGroup->SetClientPosition(group->GetClientPosition());
      updateGroup->SetClientID(group->GetClientID());
      updateGroup->SetClientGroupName(group->ClientGroupName());

      // don't override properties we only store locally in our PVR database
      if (!bUpdateFromClient)
      {
        updateGroup->SetGroupName(group->GroupName());
        updateGroup->SetLastWatched(group->LastWatched());
        updateGroup->SetHidden(group->IsHidden());
        updateGroup->SetLastOpened(group->LastOpened());
        updateGroup->SetPosition(group->GetPosition());
      }
      else
      {
        // only update the group name if the user hasn't changed it manually
        if (!updateGroup->IsUserSetName())
          updateGroup->SetGroupName(group->ClientGroupName());
      }
    }
    else
    {
      updateGroup = group;
      m_groups.emplace_back(updateGroup);
    }
  }

  // persist changes
  if (bUpdateFromClient)
    return updateGroup->Persist();

  return true;
}

int CPVRChannelGroups::GetGroupTypePriority(
    const std::shared_ptr<const CPVRChannelGroup>& group) const
{
  switch (group->GroupType())
  {
    case PVR_GROUP_TYPE_ALL_CHANNELS:
      return 0; // highest
    case PVR_GROUP_TYPE_LOCAL:
      return 1;
    case PVR_GROUP_TYPE_REMOTE:
      return 2;
    default:
      return 3;
  }
}

int CPVRChannelGroups::GetGroupClientPriority(
    const std::shared_ptr<const CPVRChannelGroup>& group) const
{
  // if no priority was set by the user, use the client id to distinguish between the clients
  const int priority = group->GetClientPriority();
  return priority > 0 ? -priority : -group->GetClientID();
}

void CPVRChannelGroups::SortGroupsByBackendOrder()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  // sort by group type, then by client priority, then by position, last by name
  std::sort(m_groups.begin(), m_groups.end(), [this](const auto& group1, const auto& group2) {
    if (GetGroupTypePriority(group1) == GetGroupTypePriority(group2))
    {
      if (GetGroupClientPriority(group1) == GetGroupClientPriority(group2))
      {
        if (group1->GetClientPosition() == group2->GetClientPosition())
        {
          return group1->GroupName() < group2->GroupName();
        }
        return group1->GetClientPosition() < group2->GetClientPosition();
      }
      return GetGroupClientPriority(group1) < GetGroupClientPriority(group2);
    }
    return GetGroupTypePriority(group1) < GetGroupTypePriority(group2);
  });
}

void CPVRChannelGroups::SortGroupsByLocalOrder()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  // sort by group's local position
  std::sort(m_groups.begin(), m_groups.end(), [](const auto& group1, const auto& group2) {
    return group1->GetPosition() < group2->GetPosition();
  });
}

void CPVRChannelGroups::SortGroups()
{
  const bool backendOrderSort =
      m_settings.GetBoolValue(CSettings::SETTING_PVRMANAGER_BACKENDCHANNELGROUPSORDER);

  if (backendOrderSort)
    SortGroupsByBackendOrder();
  else
    SortGroupsByLocalOrder();

  CServiceBroker::GetPVRManager().PublishEvent(PVREvent::ChannelGroupsInvalidated);
}

std::shared_ptr<CPVRChannelGroupMember> CPVRChannelGroups::GetChannelGroupMemberByPath(
    const CPVRChannelsPath& path) const
{
  if (path.IsChannel())
  {
    const std::shared_ptr<const CPVRChannelGroup> group =
        GetByName(path.GetGroupName(), path.GetGroupClientID());
    if (group)
      return group->GetByUniqueID(
          {CPVRClientUID(path.GetAddonID(), path.GetInstanceID()).GetUID(), path.GetChannelUID()});
  }

  return {};
}

std::vector<std::shared_ptr<CPVRChannelGroupMember>> CPVRChannelGroups::GetMembersAvailableForGroup(
    const std::shared_ptr<const CPVRChannelGroup>& group)
{
  std::vector<std::shared_ptr<CPVRChannelGroupMember>> result;

  if (group->SupportsMemberAdd())
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);

    const auto allGroupMembers = GetGroupAll()->GetMembers();
    for (const auto& groupMember : allGroupMembers)
    {
      if (!group->IsGroupMember(groupMember))
        result.emplace_back(groupMember);
    }
  }

  return result;
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetById(int iGroupId) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto it = std::find_if(m_groups.cbegin(), m_groups.cend(), [iGroupId](const auto& group) {
    return group->GroupID() == iGroupId;
  });
  return (it != m_groups.cend()) ? (*it) : std::shared_ptr<CPVRChannelGroup>();
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetGroupByPath(
    const std::string& strInPath) const
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

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetByName(const std::string& strName,
                                                               int clientID) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto it =
      std::find_if(m_groups.cbegin(), m_groups.cend(), [&strName, clientID](const auto& group) {
        return group->GroupName() == strName && group->GetClientID() == clientID;
      });
  return (it != m_groups.cend()) ? (*it) : std::shared_ptr<CPVRChannelGroup>();
}

bool CPVRChannelGroups::HasValidDataForClients(
    const std::vector<std::shared_ptr<CPVRClient>>& clients) const
{
  return m_failedClientsForChannelGroups.empty() ||
         std::none_of(clients.cbegin(), clients.cend(),
                      [this](const std::shared_ptr<const CPVRClient>& client) {
                        return std::find(m_failedClientsForChannelGroups.cbegin(),
                                         m_failedClientsForChannelGroups.cend(),
                                         client->GetID()) != m_failedClientsForChannelGroups.cend();
                      });
}

bool CPVRChannelGroups::UpdateFromClients(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                          bool bChannelsOnly /* = false */)
{
  bool bReturn = true;

  // sync groups
  if (!bChannelsOnly)
  {
    // get channel groups from the clients
    CServiceBroker::GetPVRManager().Clients()->GetChannelGroups(clients, this,
                                                                m_failedClientsForChannelGroups);
  }

  // sync channels in groups
  std::vector<std::shared_ptr<CPVRChannelGroup>> groups;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    groups = m_groups;
  }

  for (const auto& group : groups)
  {
    if (!bChannelsOnly || group->IsChannelsOwner())
    {
      if (!group->UpdateFromClients(clients))
      {
        CLog::LogFC(LOGERROR, LOGPVR, "Failed to update channel group '{}'", group->GroupName());
        bReturn = false;
      }
    }

    if (bReturn && group->IsChannelsOwner() &&
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bPVRChannelIconsAutoScan)
    {
      CServiceBroker::GetPVRManager().TriggerSearchMissingChannelIcons(group);
    }
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
  if (iType == PVR_GROUP_TYPE_ALL_CHANNELS)
    return std::make_shared<CPVRChannelGroupAllChannels>(path);
  else if (iType == PVR_GROUP_TYPE_LOCAL)
    return std::make_shared<CPVRChannelGroupFromUser>(path, GetGroupAll());
  else
    return std::make_shared<CPVRChannelGroupFromClient>(path, GetGroupAll());
}

bool CPVRChannelGroups::LoadFromDatabase(const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  const std::shared_ptr<const CPVRDatabase> database(
      CServiceBroker::GetPVRManager().GetTVDatabase());
  if (!database)
    return false;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  // Ensure we have an all channels group. It is important that the all channels group is
  // created before loading contents from database.
  if (m_groups.empty())
  {
    const auto internalGroup = std::make_shared<CPVRChannelGroupAllChannels>(m_bRadio);
    m_groups.emplace_back(internalGroup);
    m_allChannelsGroup = internalGroup;
  }

  CLog::LogFC(LOGDEBUG, LOGPVR, "Loading all {} channel groups and members",
              m_bRadio ? "radio" : "TV");

  // load all channels from the database
  std::map<std::pair<int, int>, std::shared_ptr<CPVRChannel>> channels;
  database->Get(m_bRadio, clients, channels);
  CLog::LogFC(LOGDEBUG, LOGPVR, "Fetched {} {} channels from the database", channels.size(),
              m_bRadio ? "radio" : "TV");

  // load local groups from the database
  int iLoaded = database->GetLocalGroups(*this);

  // load backend-supplied groups from the database
  iLoaded += database->Get(*this, clients);
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
    if ((*it)->Size() == 0 && !(*it)->IsChannelsOwner())
      it = m_groups.erase(it);
    else
      ++it;
  }

  // Register for client priority changes
  if (!m_isSubscribed)
  {
    CServiceBroker::GetPVRManager().Events().Subscribe(this, &CPVRChannelGroups::OnPVRManagerEvent);
    m_isSubscribed = true;
  }

  SortGroups();
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
  return m_allChannelsGroup;
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
  return std::accumulate(m_groups.cbegin(), m_groups.cend(), std::shared_ptr<CPVRChannelGroup>{},
                         [](const std::shared_ptr<CPVRChannelGroup>& last,
                            const std::shared_ptr<CPVRChannelGroup>& group) {
                           return group->LastOpened() > 0 &&
                                          (!last || group->LastOpened() > last->LastOpened())
                                      ? group
                                      : last;
                         });
}

std::vector<std::shared_ptr<CPVRChannelGroup>> CPVRChannelGroups::GetMembers(
    bool bExcludeHidden /* = false */) const
{
  std::vector<std::shared_ptr<CPVRChannelGroup>> groups;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  std::copy_if(
      m_groups.cbegin(), m_groups.cend(), std::back_inserter(groups),
      [bExcludeHidden](const auto& group) { return (!bExcludeHidden || !group->IsHidden()); });
  return groups;
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetPreviousGroup(
    const CPVRChannelGroup& group) const
{
  {
    bool bReturnNext = false;

    std::unique_lock<CCriticalSection> lock(m_critSection);
    for (std::vector<std::shared_ptr<CPVRChannelGroup>>::const_reverse_iterator it =
             m_groups.rbegin();
         it != m_groups.rend(); ++it)
    {
      // return this entry
      if (bReturnNext && !(*it)->IsHidden())
        return *it;

      // return the next entry
      if ((*it)->GroupID() == group.GroupID())
        bReturnNext = true;
    }

    // no match return last visible group
    for (std::vector<std::shared_ptr<CPVRChannelGroup>>::const_reverse_iterator it =
             m_groups.rbegin();
         it != m_groups.rend(); ++it)
    {
      if (!(*it)->IsHidden())
        return *it;
    }
  }

  // no match
  return GetLastGroup();
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::GetNextGroup(
    const CPVRChannelGroup& group) const
{
  {
    bool bReturnNext = false;

    std::unique_lock<CCriticalSection> lock(m_critSection);
    for (std::vector<std::shared_ptr<CPVRChannelGroup>>::const_iterator it = m_groups.begin();
         it != m_groups.end(); ++it)
    {
      // return this entry
      if (bReturnNext && !(*it)->IsHidden())
        return *it;

      // return the next entry
      if ((*it)->GroupID() == group.GroupID())
        bReturnNext = true;
    }

    // no match return first visible group
    for (std::vector<std::shared_ptr<CPVRChannelGroup>>::const_iterator it = m_groups.begin();
         it != m_groups.end(); ++it)
    {
      if (!(*it)->IsHidden())
        return *it;
    }
  }

  // no match
  return GetFirstGroup();
}

std::shared_ptr<CPVRChannelGroup> CPVRChannelGroups::AddGroup(const std::string& strName)
{
  bool bPersist(false);
  std::shared_ptr<CPVRChannelGroup> group;

  {
    std::unique_lock<CCriticalSection> lock(m_critSection);

    // check if there's another local group with the same name already
    group = GetByName(strName, PVR_GROUP_CLIENT_ID_LOCAL);
    if (!group)
    {
      // create a new local group
      group = CreateChannelGroup(PVR_GROUP_TYPE_LOCAL,
                                 CPVRChannelsPath(m_bRadio, strName, PVR_GROUP_CLIENT_ID_LOCAL));

      m_groups.emplace_back(group);
      SortGroups();
      bPersist = true;

      CServiceBroker::GetPVRManager().PublishEvent(PVREvent::ChannelGroupsInvalidated);
    }
  }

  // persist in the db if a new group was added
  if (bPersist)
    group->Persist();

  return group;
}

bool CPVRChannelGroups::DeleteGroup(const std::shared_ptr<CPVRChannelGroup>& group)
{
  if (!group->SupportsDelete())
  {
    CLog::LogF(LOGERROR, "Channel group {} cannot be deleted", group->GroupName());
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

void CPVRChannelGroups::OnPVRManagerEvent(const PVR::PVREvent& event)
{
  if (event == PVREvent::ClientsPrioritiesInvalidated)
  {
    // Update group client priorities
    std::vector<std::shared_ptr<CPVRChannelGroup>> groups;
    {
      std::unique_lock<CCriticalSection> lock(m_critSection);
      groups = m_groups;
    }

    for (const auto& group : groups)
    {
      group->UpdateClientPriorities();
    }

    SortGroups();
  }
}
