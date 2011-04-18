#pragma once

/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#define XBMC_INTERNAL_GROUP_RADIO 1
#define XBMC_INTERNAL_GROUP_TV    2

class CPVRChannelGroups;
class CPVRChannelGroupInternal;

typedef struct {
  CPVRChannel *channel;
  unsigned int iChannelNumber;
} PVRChannelGroupMember;

/** A group of channels */

class CPVRChannelGroup : private std::vector<PVRChannelGroupMember>
{
  friend class CPVRChannelGroups;
  friend class CPVRChannelGroupInternal;
  friend class CPVRDatabase;

private:
  bool             m_bRadio;       /*!< true if this container holds radio channels, false if it holds TV channels */
  int              m_iGroupId;     /*!< The ID of this group in the database */
  CStdString       m_strGroupName; /*!< The name of this group */
  int              m_iSortOrder;   /*!< The sort order to use */
  bool             m_bLoaded;      /*!< True if this container is loaded, false otherwise */
  bool             m_bChanged;     /*!< true if anything changed in this group that hasn't been persisted, false otherwise */
  CCriticalSection m_critSection;

  /*!
   * @brief Load the channels stored in the database.
   * @param bCompress If true, compress the database after storing the channels.
   * @return The amount of channels that were added.
   */
  virtual int LoadFromDb(bool bCompress = false);

  /*!
   * @brief Update the current channel list with the given list.
   *
   * Update the current channel list with the given list.
   * Only the new channels will be present in the passed list after this call.
   *
   * @param channels The channels to use to update this list.
   * @return True if everything went well, false otherwise.
   */
  virtual bool UpdateGroupEntries(const CPVRChannelGroup &channels);

  /*!
   * @brief Remove invalid channels from this container.
   */
  virtual void RemoveInvalidChannels(void);

  /*!
   * @brief Load the channels from the database.
   * @return The amount of channels that were added or -1 if an error occured.
   */
  virtual int Load(void);

  /*!
   * @brief Clear this channel list.
   */
  virtual void Unload(void);

  /*!
   * @brief Remove a channel.
   * @param iUniqueID The ID of the channel to delete.
   * @return True if the channel was found and removed, false otherwise.
   */
  bool RemoveByUniqueID(int iUniqueID);

  /*!
   * @brief Load the channels from the clients.
   * @return The amount of channels that were added.
   */
  virtual int LoadFromClients(void);

  /*!
   * @brief Remove invalid channels and updates the channel numbers.
   */
  void Renumber(void);

  /*!
   * @return Cache all channel icons in this group if guisetting "pvrmenu.iconpath" is set.
   */
  void CacheIcons(void);

public:
  /*!
   * @brief Create a new channel group instance.
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
  CPVRChannelGroup(bool bRadio);

  /*!
   * @brief Create a new channel group instance from a channel group provided by an add-on.
   * @param group The channel group provided by the add-on.
   */
  CPVRChannelGroup(const PVR_CHANNEL_GROUP &group);

  /*!
   * @brief Destruct this channel group.
   */
  virtual ~CPVRChannelGroup(void);

  virtual bool operator ==(const CPVRChannelGroup &right) const;
  virtual bool operator !=(const CPVRChannelGroup &right) const;

  /*!
   * @brief Refresh the channel list from the clients.
   */
  virtual bool Update(void);

  /*!
   * @brief Update the information in this group with the passed group's info.
   * @param group The new info.
   * @return True if this group was updated, false otherwise.
   */
  virtual bool Update(const CPVRChannelGroup &group);

  /*!
   * @brief Move a channel from position iOldIndex to iNewIndex.
   * @param iOldChannelNumber The channel number of the channel to move.
   * @param iNewChannelNumber The new channel number.
   * @param bSaveInDb If true, save this change in the database.
   * @return True if the channel was moved successfully, false otherwise.
   */
  virtual bool MoveChannel(unsigned int iOldChannelNumber, unsigned int iNewChannelNumber, bool bSaveInDb = true);

  /*!
   * @brief Search missing channel icons for all known channels.
   * @param bUpdateDb If true, update the changed values in the database.
   */
  virtual void SearchAndSetChannelIcons(bool bUpdateDb = false);

  /*!
   * @brief Remove a channel from this container.
   * @param channel The channel to remove.
   * @return True if the channel was found and removed, false otherwise.
   */
  virtual bool RemoveFromGroup(CPVRChannel *channel);

  /*!
   * @brief Add a channel to this container.
   * @param channel The channel to add.
   * @param iChannelNumber The channel number of the channel number to add. Use -1 to add it at the end.
   * @param bSortAndRenumber Set to false to keep the channel list unsorted after adding a new channel.
   * @return True if the channel was added, false otherwise.
   */
  virtual bool AddToGroup(CPVRChannel *channel, int iChannelNumber = 0, bool bSortAndRenumber = true);

  /*!
   * @brief Change the name of this group.
   * @param strGroupName The new group name.
   * @param bSaveInDb Save in the database or not.
   * @return True if the something changed, false otherwise.
   */
  virtual bool SetGroupName(const CStdString &strGroupName, bool bSaveInDb = false);

  /*!
   * @brief Persist changed or new data.
   * @return True if the channel was persisted, false otherwise.
   */
  virtual bool Persist(void);

  /*!
   * @brief Check whether a channel is in this container.
   * @param channel The channel to find.
   * @return True if the channel was found, false otherwise.
   */
  virtual bool IsGroupMember(const CPVRChannel *channel) const;

  /*!
   * @brief Check if this group is the internal group containing all channels.
   * @return True if it's the internal group, false otherwise.
   */
  virtual bool IsInternalGroup(void) const { return false; }

  /*!
   * @brief Get the first channel in this group.
   * @return The first channel.
   */
  virtual const CPVRChannel *GetFirstChannel(void) const;

  /*!
   * @brief True if this group holds radio channels, false if it holds TV channels.
   * @return True if this group holds radio channels, false if it holds TV channels.
   */
  virtual bool IsRadio(void) const { return m_bRadio; }

  /*!
   * @brief The database ID of this group.
   * @return The database ID of this group.
   */
  virtual int GroupID(void) const { return m_iGroupId; }

  /*!
   * @brief Set the database ID of this group.
   * @param iGroupId The new database ID.
   */
  virtual void SetGroupID(int iGroupId) { m_iGroupId = iGroupId; }

  /*!
   * @brief The name of this group.
   * @return The name of this group.
   */
  virtual const CStdString &GroupName(void) const { return m_strGroupName; }

  /*!
   * @brief The sort order of this group.
   * @return The sort order of this group.
   */
  virtual int SortOrder(void) const { return m_iSortOrder; }

  /*!
   * @brief Change the sort order of this group.
   * @param iSortOrder The new sort order of this group.
   */
  virtual void SetSortOrder(int iSortOrder) { m_iSortOrder = iSortOrder; }

  /*! @name Sort methods
   */
  //@{

  /*!
   * @brief Sort the current channel list by client channel number.
   */
  virtual void SortByClientChannelNumber(void);

  /*!
   * @brief Sort the current channel list by channel number.
   */
  virtual void SortByChannelNumber(void);

  //@}

  virtual void SetSelectedGroup(void);
  virtual void ResetChannelNumbers(void);

  /*! @name getters
   */
  //@{

  /*!
   * @brief Get a channel given the channel number on the client.
   * @param iUniqueChannelId The unique channel id on the client.
   * @param iClientID The ID of the client.
   * @return The channel or NULL if it wasn't found.
   */
  virtual const CPVRChannel *GetByClient(int iUniqueChannelId, int iClientID) const;

  /*!
   * @brief Get a channel given it's channel ID.
   * @param iChannelID The channel ID.
   * @return The channel or NULL if it wasn't found.
   */
  virtual const CPVRChannel *GetByChannelID(int iChannelID) const;

  /*!
   * @brief Get a channel given it's unique ID.
   * @param iUniqueID The unique ID.
   * @return The channel or NULL if it wasn't found.
   */
  virtual const CPVRChannel *GetByUniqueID(int iUniqueID) const;

  /*!
   * @brief The channel that was played last that has a valid client or NULL if there was none.
   * @return The requested channel.
   */
  virtual const CPVRChannel *GetLastPlayedChannel(void) const;

  /*!
   * @brief Get a channel given it's channel number.
   * @param iChannelNumber The channel number.
   * @return The channel or NULL if it wasn't found.
   */
  virtual const CPVRChannel *GetByChannelNumber(unsigned int iChannelNumber) const;

  /*!
   * @brief Get the channel number in this group of the given channel.
   * @param channel The channel to get the channel number for.
   * @return The channel number in this group or 0 if the channel isn't a member of this group.
   */
  virtual unsigned int GetChannelNumber(const CPVRChannel &channel) const;

  /*!
   * @brief Get the next channel in this group.
   * @param channel The current channel.
   * @return The channel or NULL if it wasn't found.
   */
  virtual const CPVRChannel *GetByChannelUp(const CPVRChannel &channel) const;

  /*!
   * @brief Get the previous channel in this group.
   * @param channel The current channel.
   * @return The channel or NULL if it wasn't found.
   */
  virtual const CPVRChannel *GetByChannelDown(const CPVRChannel &channel) const;

  /*!
   * @brief Get a channel given it's index in this container.
   * @param index The index in this container.
   * @return The channel or NULL if it wasn't found.
   */
  virtual const CPVRChannel *GetByIndex(unsigned int index) const;

  /*!
   * @brief Get the list of channels in a group.
   * @param results The file list to store the results in.
   * @param bGroupMembers If true, get the channels that are in this group. Get the channels that are not in this group otherwise.
   * @return The amount of channels that were added to the list.
   */
  virtual int GetMembers(CFileItemList *results, bool bGroupMembers = true) const;

  /*!
   * @brief The amount of channels in this container.
   * @return The amount of channels in this container.
   */
  virtual int GetNumChannels(void) const { return size(); }

  /*!
   * @brief The amount of hidden channels in this container.
   * @return The amount of hidden channels in this container.
   */
  virtual int GetNumHiddenChannels(void) const { return 0; }

  /*!
   * @return True if there is at least one channel in this group with changes that haven't been persisted, false otherwise.
   */
  virtual bool HasChangedChannels(void) const;

  /*!
   * @return True if there is at least one new channel in this group that hasn't been persisted, false otherwise.
   */
  virtual bool HasNewChannels(void) const;

  /*!
   * @return True if anything changed in this group that hasn't been persisted, false otherwise.
   */
  virtual bool HasChanges(void) const;

  //@}
};
