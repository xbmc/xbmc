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

#include "VideoInfoTag.h"
#include "DateTime.h"
#include "FileItem.h"
#include "PVRChannel.h"
#include "../addons/include/xbmc_pvr_types.h"

class CPVRChannelGroups;
class CPVREpg;

class CPVRChannels : public std::vector<CPVRChannel *>
{
private:
  bool m_bRadio;          /* true if this container holds radio channels, false if it holds TV channels */
  int  m_iHiddenChannels; /* the amount of hidden channels in this container */

  /**
   * Load the channels stored in the database.
   * Returns the amount of channels that were added.
   */
  int LoadFromDb(bool bCompress = false);

  /**
   * Load the channels from the clients.
   * Returns the amount of channels that were added.
   */
  int LoadFromClients(void);

  /**
   * Remove a channel.
   * Returns true if the channel was found and removed, false otherwise
   */
  bool RemoveByUniqueID(long iUniqueID);

  /**
   * Update the current channel list with the given list.
   * Only the new channels will be present in the passed list after this call.
   * Return true if everything went well, false otherwise.
   */
  bool Update(CPVRChannels *channels);

  /**
   * Update the icon path of a channel if the path is valid.
   */
  bool SetIconIfValid(CPVRChannel *channel, CStdString strIconPath, bool bUpdateDb = false);

  /**
   * Remove invalid channels from this container.
   */
  void RemoveInvalidChannels(void);

  /**
   * Called by GetDirectory to get the directory for a group
   */
  static bool GetGroupsDirectory(const CStdString &strBase, CFileItemList *results, bool bRadio);

public:
  CPVRChannels(bool bRadio);

  /**
   * Load the channels from the database. If no channels are stored in the database, then the channels will be loaded from the clients.
   * Returns the amount of channels that were added.
   */
  int Load();

  /**
   * Refresh the channel list from the clients.
   */
  bool Update();

  /**
   * Search missing channel icons for all known channels.
   */
  void SearchAndSetChannelIcons(bool bUpdateDb = false);

  /**
   * Sort the current channel list by client channel number.
   */
  void SortByClientChannelNumber(void);

  /**
   * Sort the current channel list by channel number.
   */
  void SortByChannelNumber(void);

  /**
   * Move a channel from position iOldIndex to iNewIndex.
   */
  void MoveChannel(unsigned int iOldIndex, unsigned int iNewIndex);

  /**
   * Remove invalid channels and updates the channel numbers.
   */
  void ReNumberAndCheck(void);

  /**
   * Clear this channel list.
   */
  void Clear();

  /**
   * Show a hidden channel or hide a visible channel.
   */
  bool HideChannel(CPVRChannel *channel, bool bShowDialog = true);

  /**
   * Get a channel given it's unique ID.
   */
  CPVRChannel *GetByUniqueID(int iUniqueID);

  /**
   * Get a channel given it's channel number.
   */
  CPVRChannel *GetByChannelNumber(int iChannelNumber);

  /**
   * Get a channel given the channel number on the client.
   */
  CPVRChannel *GetByClient(int iClientChannelNumber, int iClientID);

  /**
   * Get a channel given it's channel ID.
   */
  CPVRChannel *GetByChannelID(long iChannelID);

  /**
   * Get a channel given it's index in this container.
   */
  CPVRChannel *GetByIndex(unsigned int index);

  /**
   * Get the list of channels in a group.
   * XXX move this to PVRChannelGroup.
   */
  int GetChannels(CFileItemList* results, int iGroupID = -1, bool bHidden = false);

  /**
   * Get the list of hidden channels.
   */
  int GetHiddenChannels(CFileItemList* results);

  /**
   * The amount of channels in this container.
   */
  int GetNumChannels() const { return size(); }

  /**
   * The amount of channels in this container.
   */
  int GetNumHiddenChannels() const { return m_iHiddenChannels; }

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

  /**
   * Get a channel given it's path.
   */
  static CPVRChannel *GetByPath(const CStdString &strPath);

  /**
   * Try to find missing channel icons automatically
   */
  static void SearchMissingChannelIcons();

  /**
   * Get the directory for a path
   */
  static bool GetDirectory(const CStdString& strPath, CFileItemList &results);
};

extern CPVRChannels PVRChannelsTV;
extern CPVRChannels PVRChannelsRadio;
