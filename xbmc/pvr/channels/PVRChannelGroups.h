#pragma once

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

#include <vector>

#include "FileItem.h"
#include "PVRChannelGroup.h"
#include "threads/CriticalSection.h"

namespace PVR
{
  /** A container class for channel groups */

  class CPVRChannelGroups
  {
  public:
    /*!
     * @brief Create a new group container.
     * @param bRadio True if this is a container for radio channels, false if it is for tv channels.
     */
    CPVRChannelGroups(bool bRadio);
    virtual ~CPVRChannelGroups(void);

    /*!
     * @brief Remove all channels from this group.
     */
    void Clear(void);

    /*!
     * @brief Load this container's contents from the database or PVR clients.
     * @return True if it was loaded successfully, false if not.
     */
    bool Load(void);

    /*!
     * @return Amount of groups in this container
     */
    int Size(void) const { CSingleLock lock(m_critSection); return m_groups.size(); }

    /*!
     * @brief Update a group or add it if it's not in here yet.
     * @param group The group to update.
     * @param bUpdateFromClient True to save the changes in the db.
     * @return True if the group was added or update successfully, false otherwise.
     */
    bool Update(const CPVRChannelGroup &group, bool bUpdateFromClient = false);

    /*!
     * @brief Called by the add-on callback to add a new group
     * @param group The group to add
     * @return True when updated, false otherwise
     */
    bool UpdateFromClient(const CPVRChannelGroup &group) { return Update(group, true); }

    /*!
     * @brief Get a channel given it's path
     * @param strPath The path to the channel
     * @return The channel, or an empty fileitem when not found
     */
    CFileItemPtr GetByPath(const std::string &strPath) const;

    /*!
     * @brief Get a pointer to a channel group given it's ID.
     * @param iGroupId The ID of the group.
     * @return The group or NULL if it wasn't found.
     */
    CPVRChannelGroupPtr GetById(int iGroupId) const;

    /*!
     * @brief Get a group given it's name.
     * @param strName The name.
     * @return The group or NULL if it wan't found.
     */
    CPVRChannelGroupPtr GetByName(const std::string &strName) const;

    /*!
     * @brief Get the group that contains all channels.
     * @return The group that contains all channels.
     */
    CPVRChannelGroupPtr GetGroupAll(void) const;

    /*!
     * @return The first group in this container, which always is the group with all channels.
     */
    CPVRChannelGroupPtr GetFirstGroup(void) const { return GetGroupAll(); }

    /*!
     * @return The last group in this container.
     */
    CPVRChannelGroupPtr GetLastGroup(void) const;

    /*!
     * @brief The group that was played last and optionally contains the given channel.
     * @param iChannelID The channel ID
     * @return The last watched group.
     */
    CPVRChannelGroupPtr GetLastPlayedGroup(int iChannelID = -1) const;

    /*!
     * @brief Get the list of groups.
     * @param groups The list to store the results in.
     * @return The amount of items that were added.
     */
    std::vector<CPVRChannelGroupPtr> GetMembers() const;

    /*!
     * @brief Get the list of groups.
     * @param results The file list to store the results in.
     * @param bExcludeHidden Decides whether to filter hidden groups
     * @return The amount of items that were added.
     */
    int GetGroupList(CFileItemList* results, bool bExcludeHidden = false) const;

    /*!
     * @brief Get the previous group in this container.
     * @param group The current group.
     * @return The previous group or the group containing all channels if it wasn't found.
     */
    CPVRChannelGroupPtr GetPreviousGroup(const CPVRChannelGroup &group) const;

    /*!
     * @brief Get the next group in this container.
     * @param group The current group.
     * @return The next group or the group containing all channels if it wasn't found.
     */
    CPVRChannelGroupPtr GetNextGroup(const CPVRChannelGroup &group) const;

    /*!
     * @brief Get the group that is currently selected in the UI.
     * @return The selected group.
     */
    CPVRChannelGroupPtr GetSelectedGroup(void) const;

    /*!
     * @brief Change the selected group.
     * @param group The group to select.
     */
    void SetSelectedGroup(CPVRChannelGroupPtr group);

    /*!
     * @brief Add a group to this container.
     * @param strName The name of the group.
     * @return True if the group was added, false otherwise.
     */
    bool AddGroup(const std::string &strName);

    /*!
     * @brief Delete a group in this container.
     * @param group The group to delete.
     * @return True if it was deleted successfully, false if not.
     */
    bool DeleteGroup(const CPVRChannelGroup &group);

    /*!
     * @brief Create EPG tags for all channels of the internal group.
     * @return True if EPG tags where created successfully, false if not.
     */
    bool CreateChannelEpgs(void);

    /*!
     * @brief Remove a channel from all non-system groups.
     * @param channel The channel to remove.
     */
    void RemoveFromAllGroups(const CPVRChannelPtr &channel);

    /*!
     * @brief Persist all changes in channel groups.
     * @return True if everything was persisted, false otherwise.
     */
    bool PersistAll(void);

    /*!
     * @return True when this container contains radio groups, false otherwise
     */
    bool IsRadio(void) const { return m_bRadio; }

    /*!
     * @brief Update the contents of the groups in this container.
     * @param bChannelsOnly Set to true to only update channels, not the groups themselves.
     * @return True if the update was successful, false otherwise.
     */
    bool Update(bool bChannelsOnly = false);

  private:
    bool UpdateGroupsEntries(const CPVRChannelGroups &groups);
    bool LoadUserDefinedChannelGroups(void);
    bool GetGroupsFromClients(void);
    void SortGroups(void);

    bool                             m_bRadio;         /*!< true if this is a container for radio channels, false if it is for tv channels */
    CPVRChannelGroupPtr              m_selectedGroup;  /*!< the group that's currently selected in the UI */
    std::vector<CPVRChannelGroupPtr> m_groups;         /*!< the groups in this container */
    CCriticalSection m_critSection;
  };
}
