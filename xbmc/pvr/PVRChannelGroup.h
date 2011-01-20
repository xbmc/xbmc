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

class CPVRChannelGroup : public std::vector<CPVRChannel *>
{
private:
  /**
   * Called by GetDirectory to get the directory for a group
   */
  static bool GetGroupsDirectory(const CStdString &strBase, CFileItemList *results, bool bRadio);

protected:
  bool          m_bRadio;          /* true if this container holds radio channels, false if it holds TV channels */
  bool          m_bIsSorted;       /* true if this container is sorted by channel number, false if not */

  unsigned long m_iGroupId;
  CStdString    m_strGroupName;
  int           m_iSortOrder;

  /**
   * Load the channels stored in the database.
   * Returns the amount of channels that were added.
   */
  virtual int LoadFromDb(bool bCompress = false);

  /**
   * Get the channels from the clients and add them to this container. Does not sort, renumber or add channels to the database.
   * Returns the amount of channels that were added.
   */
  virtual int GetFromClients(void);

  /**
   * Update the current channel list with the given list.
   * Only the new channels will be present in the passed list after this call.
   * Return true if everything went well, false otherwise.
   */
  virtual bool Update(CPVRChannelGroup *channels);

  /**
   * Remove invalid channels from this container.
   */
  void RemoveInvalidChannels(void);

public:
  CPVRChannelGroup(bool bRadio, unsigned int iGroupId, const CStdString &strGroupName, int iSortOrder);
  CPVRChannelGroup(bool bRadio) { m_bRadio = bRadio; }
  virtual ~CPVRChannelGroup(void);

  /**
   * Load the channels from the database. If no channels are stored in the database, then the channels will be loaded from the clients.
   * Returns the amount of channels that were added.
   */
  virtual int Load();

  /**
   * Clear this channel list.
   */
  virtual void Unload();

  /**
   * Refresh the channel list from the clients.
   */
  virtual bool Update();

  /**
   * Remove a channel.
   * Returns true if the channel was found and removed, false otherwise
   */
  bool RemoveByUniqueID(long iUniqueID);

  /**
   * Move a channel from position iOldIndex to iNewIndex.
   */
  virtual void MoveChannel(unsigned int iOldIndex, unsigned int iNewIndex);

  /**
   * Show a hidden channel or hide a visible channel.
   */
  virtual bool HideChannel(CPVRChannel *channel, bool bShowDialog = true);

  /**
   * Search missing channel icons for all known channels.
   */
  void SearchAndSetChannelIcons(bool bUpdateDb = false);

  /**
   * Load the channels from the clients.
   * Returns the amount of channels that were added.
   */
  virtual int LoadFromClients(bool bAddToDb = true);

  /**
   * Remove a channel from this container
   */
  bool RemoveFromGroup(const CPVRChannel *channel);

  /**
   * Add a channel to this container
   */
  bool AddToGroup(CPVRChannel *channel);

  /**
   * Change the name of this group
   */
  virtual bool SetGroupName(const CStdString &strGroupName, bool bSaveInDb = false);

  /**
   * Persist changed or new data
   */
  virtual bool Persist(bool bQueueWrite = false);

  /**
   * Check whether a channel is in this container
   */
  bool IsGroupMember(const CPVRChannel *channel);

  bool IsRadio(void) const { return m_bRadio; }
  long GroupID(void) const { return m_iGroupId; }
  void SetGroupID(long iGroupId) { m_iGroupId = iGroupId; }
  CStdString GroupName(void) const { return m_strGroupName; }
  long SortOrder(void) const { return m_iSortOrder; }
  void SetSortOrder(long iSortOrder) { m_iSortOrder = iSortOrder; }

  /********** sort methods **********/

  /**
   * Sort the current channel list by client channel number.
   */
  void SortByClientChannelNumber(void);

  /**
   * Sort the current channel list by channel number.
   */
  void SortByChannelNumber(void);

  /********** getters **********/

  /**
   * Get a channel given the channel number on the client.
   */
  CPVRChannel *GetByClient(int iClientChannelNumber, int iClientID);

  /**
   * Get a channel given it's channel ID.
   */
  CPVRChannel *GetByChannelID(long iChannelID);

  /**
   * Get a channel given it's unique ID.
   */
  CPVRChannel *GetByUniqueID(int iUniqueID);

  /**
   * Get a channel given it's channel number.
   */
  CPVRChannel *GetByChannelNumber(int iChannelNumber);
  CPVRChannel *GetByChannelNumberUp(int iChannelNumber);
  CPVRChannel *GetByChannelNumberDown(int iChannelNumber);

  /**
   * Get a channel given it's index in this container.
   */
  CPVRChannel *GetByIndex(unsigned int index);

  /**
   * Get the list of channels in a group.
   */
  int GetChannels(CFileItemList* results, int iGroupID = -1, bool bHidden = false);

  /**
   * The amount of channels in this container.
   */
  int GetNumChannels() const { return size(); }

  /**
   * Get the list of hidden channels.
   */
  int GetHiddenChannels(CFileItemList* results);

  /**
   * The amount of channels in this container.
   */
  virtual int GetNumHiddenChannels() const { return 0; }

  /********** operations on all channels **********/

  /**
   * Try to find missing channel icons automatically
   */
  static void SearchMissingChannelIcons();

  /********** static getters **********/

  /**
   * Get a channel given it's path.
   */
  static CPVRChannel *GetByPath(const CStdString &strPath);

  /**
   * Get the directory for a path
   */
  static bool GetDirectory(const CStdString& strPath, CFileItemList &results);

  /**
   * The total amount of channels in all containers.
   */
  static int GetNumChannelsFromAll();

  /**
   * Get a channel given it's channel ID from all containers.
   */
  static CPVRChannel *GetByClientFromAll(int iClientChannelNumber, int iClientID);

  /**
   * Get a channel given it's channel ID from all containers.
   */
  static CPVRChannel *GetByChannelIDFromAll(long iChannelID);

  /**
   * Get a channel given it's unique ID.
   */
  static CPVRChannel *GetByUniqueIDFromAll(int iUniqueID);
};
