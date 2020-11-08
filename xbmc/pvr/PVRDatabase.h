/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dbwrappers/Database.h"
#include "threads/CriticalSection.h"

#include <map>
#include <vector>

namespace PVR
{
  class CPVRChannel;
  class CPVRChannelGroup;
  class CPVRChannelGroups;
  class CPVRClient;
  class CPVRTimerInfoTag;
  class CPVRTimers;

  /** The PVR database */

  static constexpr int CHANNEL_COMMIT_QUERY_COUNT_LIMIT = 10000;

  class CPVRDatabase : public CDatabase
  {
  public:
    /*!
     * @brief Create a new instance of the PVR database.
     */
    CPVRDatabase() = default;
    ~CPVRDatabase() override = default;

    /*!
     * @brief Open the database.
     * @return True if it was opened successfully, false otherwise.
     */
    bool Open() override;

    /*!
     * @brief Close the database.
     */
    void Close() override;

    /*!
     * @brief Lock the database.
     */
    void Lock();

    /*!
     * @brief Unlock the database.
     */
    void Unlock();

    /*!
     * @brief Get the minimal database version that is required to operate correctly.
     * @return The minimal database version.
     */
    int GetSchemaVersion() const override { return 37; }

    /*!
     * @brief Get the default sqlite database filename.
     * @return The default filename.
     */
    const char* GetBaseDBName() const override { return "TV"; }

    /*! @name Client methods */
    //@{

    /*!
     * @brief Remove all client entries from the database.
     * @return True if all client entries were removed, false otherwise.
     */
    bool DeleteClients();

    /*!
     * @brief Add or update a client entry in the database
     * @param client The client to persist.
     * @return True when persisted, false otherwise.
     */
    bool Persist(const CPVRClient& client);

    /*!
     * @brief Remove a client entry from the database
     * @param client The client to remove.
     * @return True if the client was removed, false otherwise.
     */
    bool Delete(const CPVRClient& client);

    /*!
     * @brief Get the priority for a given client from the database.
     * @param client The client.
     * @return The priority.
     */
    int GetPriority(const CPVRClient& client);

    /*! @name Channel methods */
    //@{

    /*!
     * @brief Remove all channels from the database.
     * @return True if all channels were removed, false otherwise.
     */
    bool DeleteChannels();

    /*!
     * @brief Add or update a channel entry in the database
     * @param channel The channel to persist.
     * @param bCommit queue only or queue and commit
     * @return True when persisted or queued, false otherwise.
     */
    bool Persist(CPVRChannel& channel, bool bCommit);

    /*!
     * @brief Remove a channel entry from the database
     * @param channel The channel to remove.
     * @return True if the channel was removed, false otherwise.
     */
    bool QueueDeleteQuery(const CPVRChannel& channel);

    /*!
     * @brief Get the list of channels from the database
     * @param results The channel group to store the results in.
     * @param bCompressDB Compress the DB after getting the list
     * @return The amount of channels that were added.
     */
    int Get(CPVRChannelGroup& results, bool bCompressDB);

    //@}

    /*! @name Channel group methods */
    //@{

    /*!
     * @brief Remove all channel groups from the database
     * @return True if all channel groups were removed.
     */
    bool DeleteChannelGroups();

    /*!
     * @brief Delete a channel group from the database.
     * @param group The group to delete.
     * @return True if the group was deleted successfully, false otherwise.
     */
    bool Delete(const CPVRChannelGroup& group);

    /*!
     * @brief Get the channel groups.
     * @param results The container to store the results in.
     * @return True if the list was fetched successfully, false otherwise.
     */
    bool Get(CPVRChannelGroups& results);

    /*!
     * @brief Add the group members to a group.
     * @param group The group to get the channels for.
     * @param allGroup The "all channels group" matching param group's 'IsRadio' property.
     * @return The amount of channels that were added.
     */
    int Get(CPVRChannelGroup& group, const CPVRChannelGroup& allGroup);

    /*!
     * @brief Add or update a channel group entry in the database.
     * @param group The group to persist.
     * @return True if the group was persisted successfully, false otherwise.
     */
    bool Persist(CPVRChannelGroup& group);

    /*!
     * @brief Reset all epg ids to 0
     * @return True when reset, false otherwise.
     */
    bool ResetEPG();

    /*! @name Timer methods */
    //@{

    /*!
     * @brief Get the timers.
     * @param timers The container for the timers.
     * @return The timers.
     */
    std::vector<std::shared_ptr<CPVRTimerInfoTag>> GetTimers(CPVRTimers& timers);

    /*!
     * @brief Add or update a timer entry in the database
     * @param channel The timer to persist.
     * @return True if persisted, false otherwise.
     */
    bool Persist(CPVRTimerInfoTag& timer);

    /*!
     * @brief Remove a timer from the database
     * @param timer The timer to remove.
     * @return True if the timer was removed, false otherwise.
     */
    bool Delete(const CPVRTimerInfoTag& timer);

    /*!
     * @brief Remove all timer entries from the database.
     * @return True if all timer entries were removed, false otherwise.
     */
    bool DeleteTimers();
    //@}

    /*! @name Client methods */
    //@{

    /*!
     * @brief Updates the last watched timestamp for the channel
     * @param channel the channel
     * @return whether the update was successful
     */
    bool UpdateLastWatched(const CPVRChannel& channel);

    /*!
     * @brief Updates the last watched timestamp for the channel group
     * @param group the group
     * @return whether the update was successful
     */
    bool UpdateLastWatched(const CPVRChannelGroup& group);
    //@}

    /*!
     * @brief Updates the last opened timestamp for the channel group
     * @param group the group
     * @return whether the update was successful
     */
    bool UpdateLastOpened(const CPVRChannelGroup& group);
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

    bool DeleteChannelsFromGroup(const CPVRChannelGroup& group, const std::vector<int>& channelsToDelete);

    bool GetCurrentGroupMembers(const CPVRChannelGroup& group, std::vector<int>& members);
    bool RemoveStaleChannelsFromGroup(const CPVRChannelGroup& group);

    bool PersistGroupMembers(const CPVRChannelGroup& group);

    bool PersistChannels(CPVRChannelGroup& group);

    bool RemoveChannelsFromGroup(const CPVRChannelGroup& group);

    int GetClientIdByChannelId(int iChannelId);

    CCriticalSection m_critSection;
  };
}
