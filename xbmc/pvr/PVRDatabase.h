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

#include "dbwrappers/Database.h"
#include "utils/log.h"

#include "pvr/PVRManager.h"

namespace PVR
{
  class CPVRChannelGroup;
  class CPVRChannelGroupInternal;
  class CPVRChannelsContainer;
  class CPVRChannel;
  class CPVRChannelGroups;
  class CPVRClient;

  /** The PVR database */

  class CPVRDatabase : public CDatabase
  {
  public:
    /*!
     * @brief Create a new instance of the PVR database.
     */
    CPVRDatabase(void) {};
    virtual ~CPVRDatabase(void) {};

    /*!
     * @brief Open the database.
     * @return True if it was opened successfully, false otherwise.
     */
    bool Open() override;

    /*!
     * @brief Get the minimal database version that is required to operate correctly.
     * @return The minimal database version.
     */
    int GetSchemaVersion() const override { return 30; };

    /*!
     * @brief Get the default sqlite database filename.
     * @return The default filename.
     */
    const char *GetBaseDBName() const override { return "TV"; };

    /*! @name Channel methods */
    //@{

    /*!
     * @brief Remove all channels from the database.
     * @return True if all channels were removed, false otherwise.
     */
    bool DeleteChannels(void);

    /*!
     * @brief Add or update a channel entry in the database
     * @param channel The channel to persist.
     * @return True when persisted or queued, false otherwise.
     */
    bool Persist(CPVRChannel &channel);

    /*!
     * @brief Remove a channel entry from the database
     * @param channel The channel to remove.
     * @return True if the channel was removed, false otherwise.
     */
    bool Delete(const CPVRChannel &channel);

    /*!
     * @brief Get the list of channels from the database
     * @param results The channel group to store the results in.
     * @return The amount of channels that were added.
     */
    int Get(CPVRChannelGroupInternal &results);

    //@}

    /*! @name Channel group methods */
    //@{

    /*!
     * @brief Remove all channel groups from the database
     * @return True if all channel groups were removed.
     */
    bool DeleteChannelGroups(void);

    /*!
     * @brief Delete a channel group from the database.
     * @param group The group to delete.
     * @return True if the group was deleted successfully, false otherwise.
     */
    bool Delete(const CPVRChannelGroup &group);

    /*!
     * @brief Get the channel groups.
     * @param results The container to store the results in.
     * @return True if the list was fetched successfully, false otherwise.
     */
    bool Get(CPVRChannelGroups &results);

    /*!
     * @brief Add the group members to a group.
     * @param group The group to get the channels for.
     * @return The amount of channels that were added.
     */
    int Get(CPVRChannelGroup &group);

    /*!
     * @brief Add or update a channel group entry in the database.
     * @param group The group to persist.
     * @return True if the group was persisted successfully, false otherwise.
     */
    bool Persist(CPVRChannelGroup &group);

    /*!
     * @brief Reset all epg ids to 0
     * @return True when reset, false otherwise.
     */
    bool ResetEPG(void);

    //@}

    /*! @name Client methods */
    //@{

    /*!
     * @brief Sets the 'was playing on last app quit' flag for a channel.
     * @param channel the channel
     * @param bSet True to set the flag, false to reset the flag
     * @return True if the operation was successful, false otherwise
     */
    bool SetWasPlayingOnLastQuit(const CPVRChannel &channel, bool bSet);

    /*!
     * @brief Sets the 'was playing on last app quit' flag for a channel.
     * @param channel the channel
     * @param bSet True to set the flag, false to reset the flag
     * @param bWasPlaying on return contains the previous value of the flag
     * @return True if the operation was successful, false otherwise
     */
    bool SetWasPlayingOnLastQuit(const CPVRChannel &channel, bool bSet, bool& bWasPlaying);

    /*!
    * @brief Updates the last watched timestamp for the channel
    * @param channel the channel
    * @return whether the update was successful
    */
    bool UpdateLastWatched(const CPVRChannel &channel);

    /*!
    * @brief Updates the last watched timestamp for the channel group
    * @param group the group
    * @return whether the update was successful
    */
    bool UpdateLastWatched(const CPVRChannelGroup &group);
    //@}

  private:
    /*!
     * @brief Create the PVR database tables.
     */
    void CreateTables() override;
    void CreateAnalytics() override;
    /*!
     * @brief Update an old version of the database.
     * @param version The version to update the database from.
     */
    void UpdateTables(int version) override;
    int GetMinSchemaVersion() const override { return 11; }

    bool DeleteChannelsFromGroup(const CPVRChannelGroup &group, const std::vector<int> &channelsToDelete);

    bool GetCurrentGroupMembers(const CPVRChannelGroup &group, std::vector<int> &members);
    bool RemoveStaleChannelsFromGroup(const CPVRChannelGroup &group);

    bool PersistGroupMembers(const CPVRChannelGroup &group);

    bool PersistChannels(CPVRChannelGroup &group);

    bool RemoveChannelsFromGroup(const CPVRChannelGroup &group);
  };
}
