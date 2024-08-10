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
  class CPVRChannelGroupMember;
  class CPVRChannelGroups;
  class CPVRProvider;
  class CPVRProviders;
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
    int GetSchemaVersion() const override { return 46; }

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
    int GetPriority(const CPVRClient& client) const;

    /*!
     * @brief Get the numeric client ID for given addon ID and instance ID from the database.
     * @param addonID The addon ID.
     * @param instanceID The instance ID.
     * @return The client ID on success, -1 otherwise.
     */
    int GetClientID(const std::string& addonID, unsigned int instanceID);

    /*! @name Channel methods */
    //@{

    /*!
     * @brief Remove all channels from the database.
     * @return True if all channels were removed, false otherwise.
     */
    bool DeleteChannels();

    /*!
     * @brief Get channels from the database.
     * @param bRadio Whether to fetch radio or TV channels.
     * @param clients The PVR clients the channels should be loaded for. Leave empty for all clients.
     * @param results The container for the channels.
     * @return The number of channels loaded.
     */
    int Get(bool bRadio,
            const std::vector<std::shared_ptr<CPVRClient>>& clients,
            std::map<std::pair<int, int>, std::shared_ptr<CPVRChannel>>& results) const;

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

    //@}

    /*! @name Channel group member methods */
    //@{

    /*!
     * @brief Remove a channel group member entry from the database
     * @param groupMember The group member to remove.
     * @return True if the member was removed, false otherwise.
     */
    bool QueueDeleteQuery(const CPVRChannelGroupMember& groupMember);

    //@}

    /*! @name Channel provider methods */
    //@{

    /*!
     * @brief Remove all providers from the database.
     * @return True if all providers were removed, false otherwise.
     */
    bool DeleteProviders();

    /*!
     * @brief Add or update a provider entry in the database
     * @param provider The provider to persist.
     * @param updateRecord True if record to be updated, false for insert
     * @return True when persisted, false otherwise.
     */
    bool Persist(CPVRProvider& provider, bool updateRecord = false);

    /*!
     * @brief Remove a provider entry from the database
     * @param provider The provider to remove.
     * @return True if the provider was removed, false otherwise.
     */
    bool Delete(const CPVRProvider& provider);

    /*!
     * @brief Get the list of providers from the database
     * @param results The providers to store the results in.
     * @param clients The PVR clients the providers should be loaded for. Leave empty for all clients.
     * @return The amount of providers that were added.
     */
    bool Get(CPVRProviders& results, const std::vector<std::shared_ptr<CPVRClient>>& clients) const;

    /*!
     * @brief Get the maximum provider id in the database
     * @return The maximum provider id in the database
     */
    int GetMaxProviderId() const;

    //@}

    /*! @name Channel group methods */
    //@{

    /*!
     * @brief Remove all channel groups from the database
     * @return True if all channel groups were removed.
     */
    bool DeleteChannelGroups();

    /*!
     * @brief Delete a channel group and all its members from the database.
     * @param group The group to delete.
     * @return True if the group was deleted successfully, false otherwise.
     */
    bool Delete(const CPVRChannelGroup& group);

    /*!
     * @brief Get all local channel groups.
     * @param results The container to store the results in.
     * @return The number of groups loaded.
     */
    int GetLocalGroups(CPVRChannelGroups& results) const;

    /*!
     * @brief Get client-supplied channel groups.
     * @param results The container to store the results in.
     * @param clients The PVR clients the groups should be loaded for. Leave empty for all clients.
     * @return The number of groups loaded.
     */
    int Get(CPVRChannelGroups& results,
            const std::vector<std::shared_ptr<CPVRClient>>& clients) const;

    /*!
     * @brief Get the members of a channel group.
     * @param group The group to get the members for.
     * @param clients The PVR clients the group members should be loaded for. Leave empty for all clients.
     * @return The group members.
     */
    std::vector<std::shared_ptr<CPVRChannelGroupMember>> Get(
        const CPVRChannelGroup& group,
        const std::vector<std::shared_ptr<CPVRClient>>& clients) const;

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
     * @param clients The PVR clients the timers should be loaded for. Leave empty for all clients.
     * @return The timers.
     */
    std::vector<std::shared_ptr<CPVRTimerInfoTag>> GetTimers(
        CPVRTimers& timers, const std::vector<std::shared_ptr<CPVRClient>>& clients) const;

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
     * @param groupId the id of the group used to watch the channel
     * @return whether the update was successful
     */
    bool UpdateLastWatched(const CPVRChannel& channel, int groupId);

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

    int GetGroups(CPVRChannelGroups& results, const std::string& query) const;

    bool PersistGroupMembers(const CPVRChannelGroup& group);

    bool PersistChannels(const CPVRChannelGroup& group);

    bool RemoveChannelsFromGroup(const CPVRChannelGroup& group);

    void FixupClientIDs();

    mutable CCriticalSection m_critSection;
  };
}
