/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/settings/PVRSettings.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace PVR
{
enum class PVREvent;

class CPVRChannel;
class CPVRChannelGroupFactory;
class CPVRClient;

/** A container class for channel groups */

class CPVRChannelGroups : public ISettingCallback
{
public:
  /*!
   * @brief Create a new group container.
   * @param bRadio True if this is a container for radio channels, false if it is for tv channels.
   */
  explicit CPVRChannelGroups(bool bRadio);
  virtual ~CPVRChannelGroups();

  // ISettingCallback implementation
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  /*!
   * @brief Remove all groups from this container.
   */
  void Unload();

  /*!
   * @brief Load all channel groups and all channels from PVR database.
   * @param clients The PVR clients data should be loaded for. Leave empty for all clients.
   * @return True on success, false otherwise.
   */
  bool LoadFromDatabase(const std::vector<std::shared_ptr<CPVRClient>>& clients);

  /*!
   * @brief Get the channel group factory
   * @return The factory.
   */
  std::shared_ptr<CPVRChannelGroupFactory> GetGroupFactory() const { return m_channelGroupFactory; }

  /*!
   * @return Amount of groups in this container
   */
  size_t Size() const
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    return m_groups.size();
  }

  /*!
   * @brief Update a group or add it if it's not in here yet.
   * @param group The group to update.
   * @param bUpdateFromClient True to save the changes in the db.
   * @return True if the group was added or update successfully, false otherwise.
   */
  bool Update(const std::shared_ptr<CPVRChannelGroup>& group, bool bUpdateFromClient = false);

  /*!
   * @brief Called by the add-on callback to add a new group
   * @param group The group to add
   * @return True when updated, false otherwise
   */
  bool UpdateFromClient(const std::shared_ptr<CPVRChannelGroup>& group);

  /*!
   * @brief Get a channel group member given its path
   * @param strPath The path to the channel group member
   * @return The channel group member, or nullptr if not found
   */
  std::shared_ptr<CPVRChannelGroupMember> GetChannelGroupMemberByPath(
      const CPVRChannelsPath& path) const;

  /*!
   * @brief Get all channel group members that could be added to the given group
   * @param group The group
   * @return The channel group members that could be added to the group
   */
  std::vector<std::shared_ptr<CPVRChannelGroupMember>> GetMembersAvailableForGroup(
      const std::shared_ptr<const CPVRChannelGroup>& group);

  /*!
   * @brief Get a pointer to a channel group given its ID.
   * @param iGroupId The ID of the group.
   * @return The group or NULL if it wasn't found.
   */
  std::shared_ptr<CPVRChannelGroup> GetById(int iGroupId) const;

  /*!
   * @brief Get a channel group given its path
   * @param strPath The path to the channel group
   * @return The channel group, or nullptr if not found
   */
  std::shared_ptr<CPVRChannelGroup> GetGroupByPath(const std::string& strPath) const;

  /*!
   * @brief Get a group given its name.
   * @param strName The name.
   * @param clientID The id of the client of the group to obtain or PVR_GROUP_CLIENT_ID_LOCAL for local groups.
   * @return The group or NULL if it wasn't found.
   */
  std::shared_ptr<CPVRChannelGroup> GetByName(const std::string& strName, int clientID) const;

  /*!
   * @brief Get the group that contains all channels.
   * @return The group that contains all channels.
   */
  std::shared_ptr<CPVRChannelGroup> GetGroupAll() const;

  /*!
   * @return The first group in this container, which always is the group with all channels.
   */
  std::shared_ptr<CPVRChannelGroup> GetFirstGroup() const { return GetGroupAll(); }

  /*!
   * @return The last group in this container.
   */
  std::shared_ptr<CPVRChannelGroup> GetLastGroup() const;

  /*!
   * @return The last and previous to last played channel group members. pair.first contains the last, pair.second the previous to last member.
   */
  GroupMemberPair GetLastAndPreviousToLastPlayedChannelGroupMember() const;

  /*!
   * @return The last opened group.
   */
  std::shared_ptr<CPVRChannelGroup> GetLastOpenedGroup() const;

  /*!
   * @brief Get the list of groups.
   * @param groups The list to store the results in.
   * @param bExcludeHidden Whenever to exclude hidden channel groups.
   * @return The amount of items that were added.
   */
  std::vector<std::shared_ptr<CPVRChannelGroup>> GetMembers(bool bExcludeHidden = false) const;

  /*!
   * @brief Get the previous group in this container.
   * @param group The current group.
   * @return The previous group or the group containing all channels if it wasn't found.
   */
  std::shared_ptr<CPVRChannelGroup> GetPreviousGroup(const CPVRChannelGroup& group) const;

  /*!
   * @brief Get the next group in this container.
   * @param group The current group.
   * @return The next group or the group containing all channels if it wasn't found.
   */
  std::shared_ptr<CPVRChannelGroup> GetNextGroup(const CPVRChannelGroup& group) const;

  /*!
   * @brief Add a group to this container.
   * @param strName The name of the group.
   * @return The new group on success, nullptr otherwise.
   */
  std::shared_ptr<CPVRChannelGroup> AddGroup(const std::string& strName);

  /*!
   * @brief Remove a group from this container and delete it from the database.
   * @param group The group to delete.
   * @return True if it was deleted successfully, false if not.
   */
  bool DeleteGroup(const std::shared_ptr<CPVRChannelGroup>& group);

  /*!
   * @brief Hide/unhide a group in this container.
   * @param group The group to hide/unhide.
   * @param bHide True to hide the group, false to unhide it.
   * @return True on success, false otherwise.
   */
  bool HideGroup(const std::shared_ptr<CPVRChannelGroup>& group, bool bHide);

  /*!
   * @brief Persist all changes in channel groups.
   * @return True if everything was persisted, false otherwise.
   */
  bool PersistAll();

  /*!
   * @return True when this container contains radio groups, false otherwise
   */
  bool IsRadio() const { return m_bRadio; }

  /*!
   * @brief Update data with groups and channels from the given clients, sync with local data.
   * @param clients The clients to fetch data from. Leave empty to fetch data from all created clients.
   * @param bChannelsOnly Set to true to only update channels, not the groups themselves.
   * @return True on success, false otherwise.
   */
  bool UpdateFromClients(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                         bool bChannelsOnly = false);

  /*!
   * @brief Update the channel numbers across the channel groups from the all channels group
   * @return True if any channel number was changed, false otherwise.
   */
  bool UpdateChannelNumbersFromAllChannelsGroup();

  /*!
   * @brief Erase stale texture db entries and image files.
   * @return number of cleaned up images.
   */
  int CleanupCachedImages();

  /*!
   * @brief Sort the groups.
   */
  void SortGroups();

private:
  void SortGroupsByBackendOrder();
  void SortGroupsByLocalOrder();

  /*!
   * @brief Check, whether data for given pvr clients are currently valid. For instance, data
   * can be invalid because the client's backend was offline when data was last queried.
   * @param clients The clients to check. Check all active clients if vector is empty.
   * @return True, if data is currently valid, false otherwise.
   */
  bool HasValidDataForClients(const std::vector<std::shared_ptr<CPVRClient>>& clients) const;

  void OnPVRManagerEvent(const PVR::PVREvent& event);

  int GetGroupClientPriority(const std::shared_ptr<const CPVRChannelGroup>& group) const;

  bool m_bRadio{false};
  std::vector<std::shared_ptr<CPVRChannelGroup>> m_groups;
  mutable CCriticalSection m_critSection;
  std::vector<int> m_failedClientsForChannelGroups;
  bool m_isSubscribed{false};
  CPVRSettings m_settings;
  std::shared_ptr<CPVRChannelGroup> m_allChannelsGroup;
  std::shared_ptr<CPVRChannelGroupFactory> m_channelGroupFactory;
};
} // namespace PVR
