/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_epg.h"
#include "pvr/settings/PVRSettings.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "utils/EventStream.h"

#include <atomic>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class CDateTime;

namespace PVR
{
  class CEpgUpdateRequest;
  class CEpgTagStateChange;
  class CPVREpg;
  class CPVREpgChannelData;
  class CPVREpgDatabase;
  class CPVREpgInfoTag;
  class CPVREpgSearchFilter;

  enum class PVREvent;

  struct PVREpgSearchData;

  class CPVREpgContainer : private CThread
  {
    friend class CPVREpgDatabase;

  public:
    CPVREpgContainer() = delete;

    /*!
     * @brief Create a new EPG table container.
     */
    explicit CPVREpgContainer(CEventSource<PVREvent>& eventSource);

    /*!
     * @brief Destroy this instance.
     */
    ~CPVREpgContainer() override;

    /*!
     * @brief Get a pointer to the database instance.
     * @return A pointer to the database instance.
     */
    std::shared_ptr<CPVREpgDatabase> GetEpgDatabase() const;

    /*!
     * @brief Start the EPG update thread.
     */
    void Start();

    /*!
     * @brief Stop the EPG update thread.
     */
    void Stop();

    /**
     * @brief (re)load EPG data.
     * @return True if loaded successfully, false otherwise.
     */
    bool Load();

    /**
     * @brief unload all EPG data.
     */
    void Unload();

    /*!
     * @brief Check whether the EpgContainer has fully started.
     * @return True if started, false otherwise.
     */
    bool IsStarted() const;

    /*!
     * @brief Queue the deletion of the given EPG tables from this container.
     * @param epg The tables to delete.
     * @return True on success, false otherwise.
     */
    bool QueueDeleteEpgs(const std::vector<std::shared_ptr<CPVREpg>>& epgs);

    /*!
     * @brief CEventStream callback for PVR events.
     * @param event The event.
     */
    void Notify(const PVREvent& event);

    /*!
     * @brief Create the EPg for a given channel.
     * @param iEpgId The EPG id.
     * @param strScraperName The scraper name.
     * @param channelData The channel data.
     * @return the created EPG
     */
    std::shared_ptr<CPVREpg> CreateChannelEpg(int iEpgId, const std::string& strScraperName, const std::shared_ptr<CPVREpgChannelData>& channelData);

    /*!
     * @brief Get the start and end time across all EPGs.
     * @return The times; first: start time, second: end time.
     */
    std::pair<CDateTime, CDateTime> GetFirstAndLastEPGDate() const;

    /*!
     * @brief Get all EPGs.
     * @return The EPGs.
     */
    std::vector<std::shared_ptr<CPVREpg>> GetAllEpgs() const;

    /*!
     * @brief Get an EPG given its ID.
     * @param iEpgId The database ID of the table.
     * @return The EPG or nullptr if it wasn't found.
     */
    std::shared_ptr<CPVREpg> GetById(int iEpgId) const;

    /*!
     * @brief Get an EPG given its client id and channel uid.
     * @param iClientId the id of the pvr client providing the EPG
     * @param iChannelUid the uid of the channel for the EPG
     * @return The EPG or nullptr if it wasn't found.
     */
    std::shared_ptr<CPVREpg> GetByChannelUid(int iClientId, int iChannelUid) const;

    /*!
     * @brief Get the EPG event with the given event id
     * @param epg The epg to lookup the event.
     * @param iBroadcastId The event id to lookup.
     * @return The requested event, or an empty tag when not found
     */
    std::shared_ptr<CPVREpgInfoTag> GetTagById(const std::shared_ptr<const CPVREpg>& epg,
                                               unsigned int iBroadcastId) const;

    /*!
     * @brief Get the EPG event with the given database id
     * @param iDatabaseId The id to lookup.
     * @return The requested event, or an empty tag when not found
     */
    std::shared_ptr<CPVREpgInfoTag> GetTagByDatabaseId(int iDatabaseId) const;

    /*!
     * @brief Get all EPG tags matching the given search criteria.
     * @param searchData The search criteria.
     * @return The matching tags.
     */
    std::vector<std::shared_ptr<CPVREpgInfoTag>> GetTags(const PVREpgSearchData& searchData) const;

    /*!
     * @brief Notify EPG container that there are pending manual EPG updates
     * @param bHasPendingUpdates The new value
     */
    void SetHasPendingUpdates(bool bHasPendingUpdates = true);

    /*!
     * @brief A client triggered an epg update request for a channel
     * @param iClientID The id of the client which triggered the update request
     * @param iUniqueChannelID The uid of the channel for which the epg shall be updated
     */
    void UpdateRequest(int iClientID, int iUniqueChannelID);

    /*!
     * @brief A client announced an updated epg tag for a channel
     * @param tag The epg tag containing the updated data
     * @param eNewState The kind of change (CREATED, UPDATED, DELETED)
     */
    void UpdateFromClient(const std::shared_ptr<CPVREpgInfoTag>& tag, EPG_EVENT_STATE eNewState);

    /*!
     * @brief Get the number of past days to show in the guide and to import from backends.
     * @return the number of past epg days.
     */
    int GetPastDaysToDisplay() const;

    /*!
     * @brief Get the number of future days to show in the guide and to import from backends.
     * @return the number of future epg days.
     */
    int GetFutureDaysToDisplay() const;

    /*!
     * @brief Inform the epg container that playback of an item just started.
     */
    void OnPlaybackStarted();

    /*!
     * @brief Inform the epg container that playback of an item was stopped due to user interaction.
     */
    void OnPlaybackStopped();

    /*!
     * @brief Inform the epg container that the system is going to sleep
     */
    void OnSystemSleep();

    /*!
     * @brief Inform the epg container that the system gets awake from sleep
     */
    void OnSystemWake();

    /*!
     * @brief Erase stale texture db entries and image files.
     * @return number of cleaned up images.
     */
    int CleanupCachedImages();

    /*!
     * @brief Get all saved searches from the database.
     * @param bRadio Whether to fetch saved searches for radio or TV.
     * @return The searches.
     */
    std::vector<std::shared_ptr<CPVREpgSearchFilter>> GetSavedSearches(bool bRadio);

    /*!
     * @brief Get the saved search matching the given id.
     * @param bRadio Whether to fetch a TV or radio saved search.
     * @param iId The id.
     * @return The saved search or nullptr if not found.
     */
    std::shared_ptr<CPVREpgSearchFilter> GetSavedSearchById(bool bRadio, int iId);

    /*!
     * @brief Persist a saved search in the database.
     * @param search The saved search.
     * @return True on success, false otherwise.
     */
    bool PersistSavedSearch(CPVREpgSearchFilter& search);

    /*!
     * @brief Update time last executed for the given search.
     * @param epgSearch The search.
     * @return True on success, false otherwise.
     */
    bool UpdateSavedSearchLastExecuted(const CPVREpgSearchFilter& epgSearch);

    /*!
     * @brief Delete a saved search from the database.
     * @param search The saved search.
     * @return True on success, false otherwise.
     */
    bool DeleteSavedSearch(const CPVREpgSearchFilter& search);

  private:
    /*!
     * @brief Notify EPG table observers when the currently active tag changed.
     * @return True if the check was done, false if it was not the right time to check
     */
    bool CheckPlayingEvents();

    /*!
     * @brief The next EPG ID to be given to a table when the db isn't being used.
     * @return The next ID.
     */
    int NextEpgId();

    /*!
     * @brief Wait for an EPG update to finish.
     */
    void WaitForUpdateFinish();

    /*!
     * @brief Call Persist() on each table
     * @param iMaxTimeslice time in milliseconds for max processing. Return after this time
     *        even if not all data was persisted, unless value is -1
     * @return True when they all were persisted, false otherwise.
     */
    bool PersistAll(unsigned int iMaxTimeslice) const;

    /*!
     * @brief Remove old EPG entries.
     * @return True if the old entries were removed successfully, false otherwise.
     */
    bool RemoveOldEntries();

    /*!
     * @brief Load and update the EPG data.
     * @param bOnlyPending Only check and update EPG tables with pending manual updates
     * @return True if the update has not been interrupted, false otherwise.
     */
    bool UpdateEPG(bool bOnlyPending = false);

    /*!
     * @brief Check whether a running update should be interrupted.
     * @return True if a running update should be interrupted, false otherwise.
     */
    bool InterruptUpdate() const;

    /*!
     * @brief EPG update thread
     */
    void Process() override;

    /*!
     * @brief Load all tables from the database
     */
    void LoadFromDatabase();

    /*!
     * @brief Insert data from database
     * @param newEpg the EPG containing the updated data.
     */
    void InsertFromDB(const std::shared_ptr<CPVREpg>& newEpg);

    /*!
     * @brief Queue the deletion of an EPG table from this container.
     * @param epg The table to delete.
     * @param database The database containing the epg data.
     * @return True on success, false otherwise.
     */
    bool QueueDeleteEpg(const std::shared_ptr<const CPVREpg>& epg,
                        const std::shared_ptr<CPVREpgDatabase>& database);

    std::shared_ptr<CPVREpgDatabase> m_database; /*!< the EPG database */

    bool m_bIsUpdating = false; /*!< true while an update is running */
    std::atomic<bool> m_bIsInitialising = {
        true}; /*!< true while the epg manager hasn't loaded all tables */
    bool m_bStarted = false; /*!< true if EpgContainer has fully started */
    bool m_bLoaded = false; /*!< true after epg data is initially loaded from the database */
    bool m_bPreventUpdates = false; /*!< true to prevent EPG updates */
    bool m_bPlaying = false; /*!< true if Kodi is currently playing something */
    int m_pendingUpdates = 0; /*!< count of pending manual updates */
    time_t m_iLastEpgCleanup = 0; /*!< the time the EPG was cleaned up */
    time_t m_iNextEpgUpdate = 0; /*!< the time the EPG will be updated */
    time_t m_iNextEpgActiveTagCheck = 0; /*!< the time the EPG will be checked for active tag updates */
    int m_iNextEpgId = 0; /*!< the next epg ID that will be given to a new table when the db isn't being used */

    std::map<int, std::shared_ptr<CPVREpg>> m_epgIdToEpgMap; /*!< the EPGs in this container. maps epg ids to epgs */
    std::map<std::pair<int, int>, std::shared_ptr<CPVREpg>> m_channelUidToEpgMap; /*!< the EPGs in this container. maps channel uids to epgs */

    mutable CCriticalSection m_critSection; /*!< a critical section for changes to this container */
    CEvent m_updateEvent; /*!< trigger when an update finishes */

    std::list<CEpgUpdateRequest> m_updateRequests; /*!< list of update requests triggered by addon */
    CCriticalSection m_updateRequestsLock; /*!< protect update requests */

    std::list<CEpgTagStateChange> m_epgTagChanges; /*!< list of updated epg tags announced by addon */
    CCriticalSection m_epgTagChangesLock; /*!< protect changed epg tags list */

    bool m_bUpdateNotificationPending = false; /*!< true while an epg updated notification to observers is pending. */
    CPVRSettings m_settings;
    CEventSource<PVREvent>& m_events;

    std::atomic<bool> m_bSuspended = {false};
  };
}
