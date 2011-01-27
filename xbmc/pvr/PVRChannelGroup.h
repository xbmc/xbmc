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

#include "FileItem.h"
#include "PVRChannel.h"
#include "../addons/include/xbmc_pvr_types.h"

#define XBMC_INTERNAL_GROUPID 0

class CPVRChannelGroups;
class CPVREpg;

/** A group of channels */

class CPVRChannelGroup : public std::vector<CPVRChannel *>
{
  friend class CPVRChannelGroups;

private:
  /*!
   * @brief Get the groups list for a directory.
   * @param strBase The directory path.
   * @param results The file list to store the results in.
   * @param bRadio Get radio channels or tv channels.
   * @return True if the list was filled succesfully.
   */
  static bool GetGroupsDirectory(const CStdString &strBase, CFileItemList *results, bool bRadio);

protected:
  bool          m_bRadio;       /*!< true if this container holds radio channels, false if it holds TV channels */
  bool          m_bIsSorted;    /*!< true if this container is sorted by channel number, false if not */

  unsigned long m_iGroupId;     /*!< The ID of this group in the database */
  CStdString    m_strGroupName; /*!< The name of this group */
  int           m_iSortOrder;   /*!< The sort order to use */

  /*!
   * @brief Load the channels stored in the database.
   * @param bCompress If true, compress the database after storing the channels.
   * @return The amount of channels that were added.
   */
  virtual int LoadFromDb(bool bCompress = false);

  /*!
   * @brief Get the channels from the clients and add them to this container.
   *
   * Get the channels from the clients and add them to this container.
   * Does not sort, renumber or add channels to the database.
   *
   * @return The amount of channels that were added.
   */
  virtual int GetFromClients(void);

  /*!
   * @brief Update the current channel list with the given list.
   *
   * Update the current channel list with the given list.
   * Only the new channels will be present in the passed list after this call.
   *
   * @param channels The channels to use to update this list.
   * @return True if everything went well, false otherwise.
   */
  virtual bool Update(CPVRChannelGroup *channels);

  /*!
   * @brief Remove invalid channels from this container.
   */
  void RemoveInvalidChannels(void);

public:
  /*!
   * Create a new channel group instance.
   * @param bRadio True if this group holds radio channels.
   * @param iGroupId The database ID of this group.
   * @param strGroupName The name of this group.
   * @param iSortOrder The sort order to use.
   */
  CPVRChannelGroup(bool bRadio, unsigned int iGroupId, const CStdString &strGroupName, int iSortOrder);

  /*!
   * @brief Create a new channel group.
   * @param bRadio True if this group holds radio channels.
   */
  CPVRChannelGroup(bool bRadio) { m_bRadio = bRadio; }

  virtual ~CPVRChannelGroup(void);

  /*!
   * @brief Load the channels from the database.
   * @return The amount of channels that were added.
   */
  virtual int Load();

  /*!
   * @brief Clear this channel list.
   */
  virtual void Unload();

  /*!
   * @brief Refresh the channel list from the clients.
   */
  virtual bool Update();

  /*!
   * @brief Remove a channel.
   * @param iUniqueID The ID of the channel to delete.
   * @return True if the channel was found and removed, false otherwise.
   */
  bool RemoveByUniqueID(long iUniqueID);

  /*!
   * @brief Move a channel from position iOldIndex to iNewIndex.
   * @param iOldIndex The old index.
   * @param iNewIndex The new index.
   */
  virtual void MoveChannel(unsigned int iOldIndex, unsigned int iNewIndex);

  /*!
   * @brief Show a hidden channel or hide a visible channel.
   * @param channel The channel to change.
   * @param bShowDialog If true, show a confirmation dialog.
   * @return True if the channel was changed, false otherwise.
   */
  virtual bool HideChannel(CPVRChannel *channel, bool bShowDialog = true);

  /*!
   * @brief Search missing channel icons for all known channels.
   * @param bUpdateDb If true, update the changed values in the database.
   */
  void SearchAndSetChannelIcons(bool bUpdateDb = false);

  /*!
   * @brief Load the channels from the clients.
   * @param bAddToDb If true, add the new channels to the database too.
   * @return The amount of channels that were added.
   */
  virtual int LoadFromClients(bool bAddToDb = true);

  /*!
   * @brief Remove a channel from this container.
   * @param channel The channel to remove.
   * @return True if the channel was found and removed, false otherwise.
   */
  bool RemoveFromGroup(const CPVRChannel *channel);

  /*!
   * @brief Add a channel to this container.
   * @param channel The channel to add.
   * @return True if the channel was added, false otherwise.
   */
  bool AddToGroup(CPVRChannel *channel);

  /*!
   * @brief Change the name of this group.
   * @param strGroupName The new group name.
   * @param bSaveInDb Save in the database or not.
   * @return True if the something changed, false otherwise.
   */
  virtual bool SetGroupName(const CStdString &strGroupName, bool bSaveInDb = false);

  /*!
   * @brief Persist changed or new data.
   * @param bQueueWrite If true, don't execute the query directly.
   * @return True if the channel was persisted, false otherwise.
   */
  virtual bool Persist(bool bQueueWrite = false);

  /*!
   * @brief Check whether a channel is in this container.
   * @param channel The channel to find.
   * @return True if the channel was found, false otherwise.
   */
  bool IsGroupMember(const CPVRChannel *channel);

  /*!
   * @brief Get the first channel in this group.
   * @return The first channel.
   */
  CPVRChannel *GetFirstChannel(void);

  /*!
   * @brief True if this group holds radio channels, false if it holds TV channels.
   * @return True if this group holds radio channels, false if it holds TV channels.
   */
  bool IsRadio(void) const { return m_bRadio; }

  /*!
   * @brief The database ID of this group.
   * @return The database ID of this group.
   */
  long GroupID(void) const { return m_iGroupId; }

  /*!
   * @brief Set the database ID of this group.
   * @param iGroupId The new database ID.
   */
  void SetGroupID(long iGroupId) { m_iGroupId = iGroupId; }

  /*!
   * @brief The name of this group.
   * @return The name of this group.
   */
  CStdString GroupName(void) const { return m_strGroupName; }

  /*!
   * @brief The sort order of this group.
   * @return The sort order of this group.
   */
  long SortOrder(void) const { return m_iSortOrder; }

  /*!
   * @brief Change the sort order of this group.
   * @param iSortOrder The new sort order of this group.
   */
  void SetSortOrder(long iSortOrder) { m_iSortOrder = iSortOrder; }

  /*! @name Sort methods
   */
  //@{

  /*!
   * @brief Sort the current channel list by client channel number.
   */
  void SortByClientChannelNumber(void);

  /*!
   * @brief Sort the current channel list by channel number.
   */
  void SortByChannelNumber(void);

  //@}

  /*! @name getters
   */
  //@{

  /*!
   * @brief Get a channel given the channel number on the client.
   * @param iClientChannelNumber The channel number on the client.
   * @param iClientID The ID of the client.
   * @return The channel or NULL if it wasn't found.
   */
  CPVRChannel *GetByClient(int iClientChannelNumber, int iClientID);

  /*!
   * @brief Get a channel given it's channel ID.
   * @param iChannelID The channel ID.
   * @return The channel or NULL if it wasn't found.
   */
  CPVRChannel *GetByChannelID(long iChannelID);

  /*!
   * @brief Get a channel given it's unique ID.
   * @param iUniqueID The unique ID.
   * @return The channel or NULL if it wasn't found.
   */
  CPVRChannel *GetByUniqueID(int iUniqueID);

  /*!
   * @brief Get a channel given it's channel number.
   * @param iChannelNumber The channel number.
   * @return The channel or NULL if it wasn't found.
   */
  CPVRChannel *GetByChannelNumber(int iChannelNumber);

  /*!
   * @brief Get the next channel given it's channel number.
   * @param iChannelNumber The channel number.
   * @return The channel or NULL if it wasn't found.
   */
  CPVRChannel *GetByChannelNumberUp(int iChannelNumber);

  /*!
   * @brief Get the previous channel given it's channel number.
   * @param iChannelNumber The channel number.
   * @return The channel or NULL if it wasn't found.
   */
  CPVRChannel *GetByChannelNumberDown(int iChannelNumber);

  /*!
   * @brief Get a channel given it's index in this container.
   * @param index The index in this container.
   * @return The channel or NULL if it wasn't found.
   */
  CPVRChannel *GetByIndex(unsigned int index);

  /*!
   * @brief Get the list of channels in a group.
   * @param results The file list to store the results in.
   * @param iGroupID The ID of the group.
   * @param bHidden Get hidden channels or not.
   * @return The amount of channels that were added to the list.
   */
  int GetChannels(CFileItemList* results, int iGroupID = -1, bool bHidden = false);

  /*!
   * @brief The amount of channels in this container.
   * @return The amount of channels in this container.
   */
  int GetNumChannels() const { return size(); }

  /*!
   * @brief Get the list of hidden channels.
   * @param results The file list to store the results in.
   * @return The amount of channels that were added to the list.
   */
  int GetHiddenChannels(CFileItemList* results);

  /*!
   * @brief The amount of hidden channels in this container.
   * @return The amount of hidden channels in this container.
   */
  virtual int GetNumHiddenChannels() const { return 0; }

  //@}

  /*! @name operations on all channels
   */
  //{

  /*!
   * @brief Try to find missing channel icons automatically
   */
  static void SearchMissingChannelIcons();

  //@}

  /*! @name Static getters
   */
  //@{

  /*!
   * @brief Get a channel given it's path.
   * @param strPath The path.
   * @return The channel or NULL if it wasn't found.
   */
  static CPVRChannel *GetByPath(const CStdString &strPath);

  /*!
   * @brief Get the directory for a path.
   * @param strPath The path.
   * @param results The file list to store the results in.
   * @return True if the directory was found, false if not.
   */
  static bool GetDirectory(const CStdString& strPath, CFileItemList &results);

  /*!
   * @brief The total amount of unique channels in all containers.
   * @return The total amount of unique channels in all containers.
   */
  static int GetNumChannelsFromAll();

  /*!
   * @brief Get a channel given it's channel ID from all containers.
   * @param iClientChannelNumber The channel number on the client.
   * @param iClientID The ID of the client.
   * @return The channel or NULL if it wasn't found.
   */
  static CPVRChannel *GetByClientFromAll(int iClientChannelNumber, int iClientID);

  /*!
   * @brief Get a channel given it's channel ID from all containers.
   * @param iChannelID The channel ID.
   * @return The channel or NULL if it wasn't found.
   */
  static CPVRChannel *GetByChannelIDFromAll(long iChannelID);

  /*!
   * @brief Get a channel given it's unique ID.
   * @param iUniqueID The unique ID of the channel.
   * @return The channel or NULL if it wasn't found.
   */
  static CPVRChannel *GetByUniqueIDFromAll(int iUniqueID);

  //@}
};
