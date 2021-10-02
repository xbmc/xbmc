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

namespace PVR
{
  class CPVRChannel;
  class CPVRChannelGroup;
  class CPVRChannelGroupMember;
  class CPVRChannelGroups;
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
     * @brief Load all channel groups and all channels in those channel groups.
     * @return True if all groups were loaded, false otherwise.
     */
    bool Load();

    /*!
     * @brief Checks whether groups were already loaded.
     * @return True if groups were successfully loaded, false otherwise.
     */
    bool Loaded() const;

    /*!
     * @brief Unload and destruct all channel groups and all channels in them.
     */
    void Unload();

    /*!
     * @brief Update the contents of all the groups in this container.
     * @param bChannelsOnly Set to true to only update channels, not the groups themselves.
     * @return True if the update was successful, false otherwise.
     */
    bool Update(bool bChannelsOnly = false);

    /*!
     * @brief Get the TV channel groups.
     * @return The TV channel groups.
     */
    CPVRChannelGroups* GetTV() const { return Get(false); }

    /*!
     * @brief Get the radio channel groups.
     * @return The radio channel groups.
     */
    CPVRChannelGroups* GetRadio() const { return Get(true); }

    /*!
     * @brief Get the radio or TV channel groups.
     * @param bRadio If true, get the radio channel groups. Get the TV channel groups otherwise.
     * @return The requested groups.
     */
    CPVRChannelGroups* Get(bool bRadio) const;

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
     * @brief Get a group given it's ID.
     * @param iGroupId The ID of the group.
     * @return The requested group or NULL if it wasn't found.
     */
    std::shared_ptr<CPVRChannelGroup> GetByIdFromAll(int iGroupId) const;

    /*!
     * @brief Get a channel given it's database ID.
     * @param iChannelId The ID of the channel.
     * @return The channel or NULL if it wasn't found.
     */
    std::shared_ptr<CPVRChannel> GetChannelById(int iChannelId) const;

    /*!
     * @brief Get the channel for the given epg tag.
     * @param epgTag The epg tag.
     * @return The channel.
     */
    std::shared_ptr<CPVRChannel> GetChannelForEpgTag(const std::shared_ptr<CPVREpgInfoTag>& epgTag) const;

    /*!
     * @brief Get a channel given it's path.
     * @param strPath The path.
     * @return The channel or nullptr if it wasn't found.
     */
    std::shared_ptr<CPVRChannel> GetByPath(const std::string& strPath) const;

    /*!
     * @brief Get a channel group member given it's path.
     * @param strPath The path.
     * @return The channel group member or nullptr if it wasn't found.
     */
    std::shared_ptr<CPVRChannelGroupMember> GetChannelGroupMemberByPath(
        const std::string& strPath) const;

    /*!
     * @brief Get a channel given it's channel ID from all containers.
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
     * @brief Create EPG tags for channels in all internal channel groups.
     * @return True if EPG tags were created successfully.
     */
    bool CreateChannelEpgs();

    /*!
     * @brief Erase stale texture db entries and image files.
     * @return number of cleaned up images.
     */
    int CleanupCachedImages();

  private:
    CPVRChannelGroupsContainer& operator=(const CPVRChannelGroupsContainer&) = delete;
    CPVRChannelGroupsContainer(const CPVRChannelGroupsContainer&) = delete;

    CPVRChannelGroups* m_groupsRadio; /*!< all radio channel groups */
    CPVRChannelGroups* m_groupsTV; /*!< all TV channel groups */
    CCriticalSection m_critSection;
    bool m_bIsUpdating = false;
    bool m_bLoaded = false;
  };
}
