/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <memory>
#include <vector>

namespace PVR
{
class CPVRChannel;
class CPVRChannelGroup;
class CPVRChannelGroupMember;
class CPVRChannelGroups;
class CPVRClient;
class CPVREpgInfoTag;

class CPVRChannelGroupsContainer
{
public:
  /*!
   * @brief Create a new container for all channel groups
   */
  CPVRChannelGroupsContainer();

  /*!
   * @brief Destroy this container.
   */
  virtual ~CPVRChannelGroupsContainer();

  /*!
   * @brief Update all channel groups and all channels from PVR database and from given clients.
   * @param clients The PVR clients data should be loaded for. Leave empty for all clients.
   * @return True on success, false otherwise.
   */
  bool Update(const std::vector<std::shared_ptr<CPVRClient>>& clients);

  /*!
   * @brief Update data with groups and channels from the given clients, sync with local data.
   * @param clients The clients to fetch data from. Leave empty to fetch data from all created clients.
   * @param bChannelsOnly Set to true to only update channels, not the groups themselves.
   * @return True on success, false otherwise.
   */
  bool UpdateFromClients(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                         bool bChannelsOnly = false);

  /*!
   * @brief Unload and destruct all channel groups and all channels in them.
   */
  void Unload();

  /*!
   * @brief Get the TV channel groups.
   * @return The TV channel groups.
   */
  std::shared_ptr<CPVRChannelGroups> GetTV() const { return Get(false); }

  /*!
   * @brief Get the radio channel groups.
   * @return The radio channel groups.
   */
  std::shared_ptr<CPVRChannelGroups> GetRadio() const { return Get(true); }

  /*!
   * @brief Get the radio or TV channel groups.
   * @param bRadio If true, get the radio channel groups. Get the TV channel groups otherwise.
   * @return The requested groups.
   */
  std::shared_ptr<CPVRChannelGroups> Get(bool bRadio) const;

  /*!
   * @brief Get the group containing all TV channels.
   * @return The group containing all TV channels.
   */
  std::shared_ptr<CPVRChannelGroup> GetGroupAllTV() const { return GetGroupAll(false); }

  /*!
   * @brief Get the group containing all radio channels.
   * @return The group containing all radio channels.
   */
  std::shared_ptr<CPVRChannelGroup> GetGroupAllRadio() const { return GetGroupAll(true); }

  /*!
   * @brief Get the group containing all TV or radio channels.
   * @param bRadio If true, get the group containing all radio channels. Get the group containing all TV channels otherwise.
   * @return The requested group.
   */
  std::shared_ptr<CPVRChannelGroup> GetGroupAll(bool bRadio) const;

  /*!
   * @brief Get a group given its path.
   * @param path The path to the channel group.
   * @return The channel group, or nullptr if not found.
   */
  std::shared_ptr<CPVRChannelGroup> GetGroupByPath(const std::string& path) const;

  /*!
   * @brief Get a group given its ID.
   * @param iGroupId The ID of the group.
   * @return The requested group or NULL if it wasn't found.
   */
  std::shared_ptr<CPVRChannelGroup> GetByIdFromAll(int iGroupId) const;

  /*!
   * @brief Get a channel given its database ID.
   * @param iChannelId The ID of the channel.
   * @return The channel or NULL if it wasn't found.
   */
  std::shared_ptr<CPVRChannel> GetChannelById(int iChannelId) const;

  /*!
   * @brief Get the channel for the given epg tag.
   * @param epgTag The epg tag.
   * @return The channel.
   */
  std::shared_ptr<CPVRChannel> GetChannelForEpgTag(
      const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const;

  /*!
   * @brief Get a channel given its path.
   * @param strPath The path.
   * @return The channel or nullptr if it wasn't found.
   */
  std::shared_ptr<CPVRChannel> GetByPath(const std::string& strPath) const;

  /*!
   * @brief Get a channel group member given its path.
   * @param strPath The path.
   * @return The channel group member or nullptr if it wasn't found.
   */
  std::shared_ptr<CPVRChannelGroupMember> GetChannelGroupMemberByPath(
      const std::string& strPath) const;

  /*!
   * @brief Get a channel given its channel ID from all containers.
   * @param iUniqueChannelId The unique channel id on the client.
   * @param iClientID The ID of the client.
   * @return The channel or NULL if it wasn't found.
   */
  std::shared_ptr<CPVRChannel> GetByUniqueID(int iUniqueChannelId, int iClientID) const;

  /*!
   * @brief Get the channel group member that was played last.
   * @return The requested channel group member or nullptr.
   */
  std::shared_ptr<CPVRChannelGroupMember> GetLastPlayedChannelGroupMember() const;

  /*!
   * @brief Erase stale texture db entries and image files.
   * @return number of cleaned up images.
   */
  int CleanupCachedImages();

private:
  CPVRChannelGroupsContainer& operator=(const CPVRChannelGroupsContainer&) = delete;
  CPVRChannelGroupsContainer(const CPVRChannelGroupsContainer&) = delete;

  /*!
   * @brief Load all channel groups and all channels from PVR database.
   * @param clients The PVR clients data should be loaded for. Leave empty for all clients.
   * @return True on success, false otherwise.
   */
  bool LoadFromDatabase(const std::vector<std::shared_ptr<CPVRClient>>& clients);

  std::shared_ptr<CPVRChannelGroups> m_groupsRadio; /*!< all radio channel groups */
  std::shared_ptr<CPVRChannelGroups> m_groupsTV; /*!< all TV channel groups */
  CCriticalSection m_critSection;
  bool m_bIsUpdating = false;
};
} // namespace PVR
