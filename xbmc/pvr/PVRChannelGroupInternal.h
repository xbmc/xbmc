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

#include "PVRChannelGroup.h"

class CPVRChannelGroups;

/** XBMC's internal group, the group containing all channels */

class CPVRChannelGroupInternal : public CPVRChannelGroup
{
  friend class CPVRChannelGroups;

private:
  int m_iHiddenChannels; /*!< the amount of hidden channels in this container */

  /*!
   * @brief Load all channels from the database.
   * @param bCompress Compress the database after changing anything.
   * @return The amount of channels that were loaded.
   */
  int LoadFromDb(bool bCompress = false);

  /*!
   * @brief Load all channels from the clients.
   * @param bAddToDb If true, add the channels to the database.
   * @return The amount of channels that were loaded.
   */
  int LoadFromClients(bool bAddToDb = true);

  /*!
   * @brief Add client channels to this container.
   * @return The amount of channels that were added.
   */
  int GetFromClients(void);

  /*!
   * @brief Update the current channel list with the given list.
   *
   * Update the current channel list with the given list.
   * Only the new channels will be present in the passed list after this call.
   *
   * @param channels The channels to use to update this list.
   * @return True if everything went well, false otherwise.
   */
  bool Update(CPVRChannelGroup *channels);

  /*!
   * @brief Refresh the channel list from the clients.
   */
  bool Update();

  /*!
   * @brief Remove invalid channels and updates the channel numbers.
   */
  void ReNumberAndCheck(void);

  /*!
   * @brief Load the channels from the database.
   *
   * Load the channels from the database.
   * If no channels are stored in the database, then the channels will be loaded from the clients.
   *
   * @return The amount of channels that were added.
   */
  int Load();

  /*!
   * @brief Clear this channel list and destroy all channel instances in it.
   */
  void Unload();

public:
  /*!
   * @brief Create a new internal channel group.
   * @param bRadio True if this group holds radio channels.
   */
  CPVRChannelGroupInternal(bool bRadio);

  /*!
   * @brief Move a channel from position iOldIndex to iNewIndex.
   * @param iOldIndex The old index.
   * @param iNewIndex The new index.
   * @param bSaveInDb If true, save this change in the database.
   * @return True if the channel was moved successfully, false otherwise.
   */
  bool MoveChannel(unsigned int iOldIndex, unsigned int iNewIndex, bool bSaveInDb = true);

  /*!
   * @brief Show a hidden channel or hide a visible channel.
   * @param channel The channel to change.
   * @param bShowDialog If true, show a confirmation dialog.
   * @return True if the channel was changed, false otherwise.
   */
  bool HideChannel(CPVRChannel *channel, bool bShowDialog = true);

  /**
   * @brief The amount of channels in this container.
   * @return The amount of channels in this container.
   */
  int GetNumHiddenChannels() const { return m_iHiddenChannels; }

  /*!
   * @brief Persist changed or new data.
   * @param bQueueWrite If true, don't execute the query directly.
   * @return True if the channel was persisted, false otherwise.
   */
  bool Persist(bool bQueueWrite = false) { return true; }

  /*!
   * @brief Update all channel numbers on timers.
   * @return True if the channel number were updated, false otherwise.
   */
  bool UpdateTimers(void);
};
