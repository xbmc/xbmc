#pragma once

/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "video/VideoInfoTag.h"
#include "DateTime.h"
#include "FileItem.h"
#include "PVRChannelGroup.h"

class CPVRChannelGroupsContainer;

/** A container class for channel groups */

class CPVRChannelGroups : public std::vector<CPVRChannelGroup *>
{
  friend class CPVRChannelGroupsContainer;

private:
  bool  m_bRadio; /*!< true if this is a container for radio channels, false if it is for tv channels */

  /*!
   * @brief Get the index in this container of the channel group with the given ID.
   * @param iGroupId The ID to find.
   * @return The index or -1 if it wasn't found.
   */
  int GetIndexForGroupID(int iGroupId) const;

protected:
  /*!
   * @brief Update the contents of the groups in this container.
   * @return True if the update was successful, false otherwise.
   */
  bool Update(void);

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
   * @brief Update a group or add it if it's not in here yet.
   * @param group The group to update.
   * @return True if the group was added or update successfully, false otherwise.
   */
  bool Update(const CPVRChannelGroup &group);

  /*!
   * @brief Get a pointer to a channel group given it's ID.
   * @param iGroupId The ID of the group.
   * @return The group or NULL if it wasn't found.
   */
  const CPVRChannelGroup *GetById(int iGroupId) const;

  /*!
   * @brief Get a group given it's name.
   * @param strName The name.
   * @return The group or NULL if it wan't found.
   */
  const CPVRChannelGroup *GetByName(const CStdString &strName) const;

  /*!
   * @brief Get the group that contains all channels.
   * @return The group that contains all channels.
   */
  const CPVRChannelGroup *GetGroupAll(void) const;

  /*!
   * @brief Get the list of groups.
   * @param results The file list to store the results in.
   * @return The amount of items that were added.
   */
  int GetGroupList(CFileItemList* results) const;

  /*!
   * @brief Get the ID of the first channel in a group.
   * @param iGroupId The ID of the group.
   * @return The ID of the first channel or 1 if it wasn't found.
   */
  int GetFirstChannelForGroupID(int iGroupId) const;

  /*!
   * @brief Get the ID of the previous group in this container.
   * @param iGroupId The ID of the current group.
   * @return The ID of the previous group or the ID of the group containing all channels if it wasn't found.
   */
  int GetPreviousGroupID(int iGroupId) const;

  /*!
   * @brief Get the previous group in this container.
   * @param group The current group.
   * @return The previous group or the group containing all channels if it wasn't found.
   */
  const CPVRChannelGroup *GetPreviousGroup(const CPVRChannelGroup &group) const;

  /*!
   * @brief Get the ID of the next group in this container.
   * @param iGroupId The ID of the current group.
   * @return The ID of the next group or the ID of the group containing all channels if it wasn't found.
   */
  int GetNextGroupID(int iGroupId) const;

  /*!
   * @brief Get the next group in this container.
   * @param group The current group.
   * @return The next group or the group containing all channels if it wasn't found.
   */
  const CPVRChannelGroup *GetNextGroup(const CPVRChannelGroup &group) const;

  /*!
   * @brief Add a group to this container.
   * @param strName The name of the group.
   * @return True if the group was added, false otherwise.
   */
  bool AddGroup(const CStdString &strName);

  /*!
   * @brief Delete a group in this container.
   * @param group The group to delete.
   * @return True if it was deleted successfully, false if not.
   */
  bool DeleteGroup(const CPVRChannelGroup &group);

  /*!
   * @brief Add a channel to the group with the given ID.
   * @param channel The channel to add.
   * @param iGroupId The ID of the group to add the channel to.
   * @return True if the channel was added, false if not.
   */
  bool AddChannelToGroup(CPVRChannel *channel, int iGroupId);

  /*!
   * @brief Get the name of a group.
   * @param iGroupId The ID of the group.
   * @return The name of the group or localized string 953 if it wasn't found.
   */
  const CStdString &GetGroupName(int iGroupId) const;

  /*!
   * @brief Get the ID of a group given it's name.
   * @param strGroupName The name of the group.
   * @return The ID or -1 if it wasn't found.
   */
  int GetGroupId(CStdString strGroupName) const;

  /*!
   * @brief Remove a channel from all non-system groups.
   * @param channel The channel to remove.
   */
  void RemoveFromAllGroups(CPVRChannel *channel);

  /*!
   * @brief Persist all changes in channel groups.
   * @return True if everything was persisted, false otherwise.
   */
  bool PersistAll(void);
};
