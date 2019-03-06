/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <list>
#include <map>
#include <memory>
#include <utility>

#include "XBDateTime.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/Observer.h"

#include "pvr/PVRSettings.h"
#include "pvr/PVRTypes.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgDatabase.h"

class CFileItem;

namespace PVR
{
  class CPVREpgChannelData;
  class CEpgUpdateRequest;
  class CEpgTagStateChange;

  class CPVREpgContainer : public Observer, public Observable, private CThread
  {
    friend class CPVREpgDatabase;

  public:
    /*!
     * @brief Create a new EPG table container.
     */
    CPVREpgContainer(void);

    /*!
     * @brief Destroy this instance.
     */
    ~CPVREpgContainer(void) override;

    /*!
     * @brief Get a pointer to the database instance.
     * @return A pointer to the database instance.
     */
    CPVREpgDatabasePtr GetEpgDatabase() const;

    /*!
     * @brief Start the EPG update thread.
     * @param bAsync Should the EPG container starts asynchronously
     */
    void Start(bool bAsync);

    /*!
     * @brief Stop the EPG update thread.
     */
    void Stop(void);

    /*!
     * @brief Clear all EPG entries.
     */
    void Clear();

    /*!
     * @brief Check whether the EpgContainer has fully started.
     * @return True if started, false otherwise.
     */
    bool IsStarted(void) const;

    /*!
     * @brief Delete an EPG table from this container.
     * @param epg The table to delete.
     * @param bDeleteFromDatabase Delete this table from the database too if true.
     * @return True on success, false otherwise.
     */
    bool DeleteEpg(const CPVREpgPtr &epg, bool bDeleteFromDatabase = false);

    /*!
     * @brief Process a notification from an observable.
     * @param obs The observable that sent the update.
     * @param msg The update message.
     */
    void Notify(const Observable &obs, const ObservableMessage msg) override;

    /*!
     * @brief Create the EPg for a given channel.
     * @param iEpgId The EPG id.
     * @param strScraperName The scraper name.
     * @param channelData The channel data.
     * @return the created EPG
     */
    std::shared_ptr<CPVREpg> CreateChannelEpg(int iEpgId, const std::string& strScraperName, const std::shared_ptr<CPVREpgChannelData>& channelData);

    /*!
     * @brief Get the start time of the first entry.
     * @return The start time.
     */
    const CDateTime GetFirstEPGDate(void);

    /*!
     * @brief Get the end time of the last entry.
     * @return The end time.
     */
    const CDateTime GetLastEPGDate(void);

    /*!
     * @brief Get an EPG given its ID.
     * @param iEpgId The database ID of the table.
     * @return The EPG or nullptr if it wasn't found.
     */
    CPVREpgPtr GetById(int iEpgId) const;

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
    std::shared_ptr<CPVREpgInfoTag> GetTagById(const std::shared_ptr<CPVREpg>& epg, unsigned int iBroadcastId) const;

    /*!
     * @brief Get all EPG tags.
     * @return The tags.
     */
    std::vector<std::shared_ptr<CPVREpgInfoTag>> GetAllTags() const;

    /*!
     * @brief Check whether data should be persisted to the EPG database.
     * @return True if data should not be persisted to the EPG database, false otherwise.
     */
    bool IgnoreDB() const;

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
    void UpdateFromClient(const CPVREpgInfoTagPtr &tag, EPG_EVENT_STATE eNewState);

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
     * @param item The item that started to play.
     */
    void OnPlaybackStarted(const std::shared_ptr<CFileItem>& item);

    /*!
     * @brief Inform the epg container that playback of an item was stopped due to user interaction.
     * @param item The item that stopped to play.
     */
    void OnPlaybackStopped(const std::shared_ptr<CFileItem>& item);

  private:
    /*!
     * @brief Notify EPG table observers when the currently active tag changed.
     * @return True if the check was done, false if it was not the right time to check
     */
    bool CheckPlayingEvents(void);

    /*!
     * @brief The next EPG ID to be given to a table when the db isn't being used.
     * @return The next ID.
     */
    int NextEpgId(void);

    /*!
     * @brief Wait for an EPG update to finish.
     */
    void WaitForUpdateFinish();

    /*!
     * @brief Call Persist() on each table
     * @return True when they all were persisted, false otherwise.
     */
    bool PersistAll(void);

    /*!
     * @brief Remove old EPG entries.
     * @return True if the old entries were removed successfully, false otherwise.
     */
    bool RemoveOldEntries(void);

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
    bool InterruptUpdate(void) const;

    /*!
     * @brief EPG update thread
     */
    void Process(void) override;

    /*!
     * @brief Load all tables from the database
     */
    void LoadFromDB(void);

    /*!
     * @brief Insert data from database
     * @param newEpg the EPG containing the updated data.
     */
    void InsertFromDB(const CPVREpgPtr &newEpg);

    CPVREpgDatabasePtr m_database; /*!< the EPG database */

    bool m_bIsUpdating = false;                /*!< true while an update is running */
    bool m_bIsInitialising = true;             /*!< true while the epg manager hasn't loaded all tables */
    bool m_bStarted = false;                   /*!< true if EpgContainer has fully started */
    bool m_bLoaded = false;                    /*!< true after epg data is initially loaded from the database */
    bool m_bPreventUpdates = false;            /*!< true to prevent EPG updates */
    bool m_bPlaying = false;                   /*!< true if Kodi is currently playing something */
    int m_pendingUpdates = 0;                  /*!< count of pending manual updates */
    time_t m_iLastEpgCleanup = 0;              /*!< the time the EPG was cleaned up */
    time_t m_iNextEpgUpdate = 0;               /*!< the time the EPG will be updated */
    time_t m_iNextEpgActiveTagCheck = 0;       /*!< the time the EPG will be checked for active tag updates */
    int m_iNextEpgId = 0;                      /*!< the next epg ID that will be given to a new table when the db isn't being used */

    std::map<int, std::shared_ptr<CPVREpg>> m_epgIdToEpgMap; /*!< the EPGs in this container. maps epg ids to epgs */
    std::map<std::pair<int, int>, std::shared_ptr<CPVREpg>> m_channelUidToEpgMap; /*!< the EPGs in this container. maps channel uids to epgs */

    mutable CCriticalSection m_critSection;    /*!< a critical section for changes to this container */
    CEvent m_updateEvent;                      /*!< trigger when an update finishes */

    std::list<CEpgUpdateRequest> m_updateRequests; /*!< list of update requests triggered by addon */
    CCriticalSection m_updateRequestsLock;         /*!< protect update requests */

    std::list<CEpgTagStateChange> m_epgTagChanges; /*!< list of updated epg tags announced by addon */
    CCriticalSection m_epgTagChangesLock;          /*!< protect changed epg tags list */

    bool m_bUpdateNotificationPending = false; /*!< true while an epg updated notification to observers is pending. */
    CPVRSettings m_settings;
  };
}
