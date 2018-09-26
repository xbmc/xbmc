/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/Observer.h"

#include "pvr/PVRSettings.h"
#include "pvr/PVRTypes.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgDatabase.h"

class CFileItemList;

namespace PVR
{
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
     * @param channel The channel.
     * @return the created EPG
     */
    CPVREpgPtr CreateChannelEpg(const CPVRChannelPtr &channel);

    /*!
     * @brief Get all EPG tables and apply a filter.
     * @param results The fileitem list to store the results in.
     * @param filter The filter to apply.
     * @return The amount of entries that were added.
     */
    int GetEPGSearch(CFileItemList &results, const CPVREpgSearchFilter &filter);

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
     * @brief Get an EPG table given it's ID.
     * @param iEpgId The database ID of the table.
     * @return The table or NULL if it wasn't found.
     */
    CPVREpgPtr GetById(int iEpgId) const;

    /*!
     * @brief Get the EPG event with the given event id
     * @param channel The channel to get the event for.
     * @param iBroadcastId The event id to get
     * @return The requested event, or an empty tag when not found
     */
    CPVREpgInfoTagPtr GetTagById(const CPVRChannelPtr &channel, unsigned int iBroadcastId) const;

    /*!
     * @brief Get the EPG events matching the given timer
     * @param timer The timer to get the matching events for.
     * @return The matching events, or an empty vector when no matching tag was found
     */
    std::vector<CPVREpgInfoTagPtr> GetEpgTagsForTimer(const CPVRTimerInfoTagPtr &timer) const;

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
    void UpdateRequest(int iClientID, unsigned int iUniqueChannelID);

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
    void OnPlaybackStarted(const CFileItemPtr &item);

    /*!
     * @brief Inform the epg container that playback of an item was stopped due to user interaction.
     * @param item The item that stopped to play.
     */
    void OnPlaybackStopped(const CFileItemPtr &item);


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
    unsigned int NextEpgId(void);

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
    unsigned int m_iNextEpgId = 0;             /*!< the next epg ID that will be given to a new table when the db isn't being used */
    std::map<unsigned int, CPVREpgPtr> m_epgs; /*!< the EPGs in this container */

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
