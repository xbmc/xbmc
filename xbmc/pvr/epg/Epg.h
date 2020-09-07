/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_epg.h"
#include "pvr/epg/EpgTagsContainer.h"
#include "threads/CriticalSection.h"
#include "utils/EventStream.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace PVR
{
  enum class PVREvent;

  class CPVREpgChannelData;
  class CPVREpgDatabase;
  class CPVREpgInfoTag;

  class CPVREpg
  {
    friend class CPVREpgDatabase;

  public:
    /*!
     * @brief Create a new EPG instance.
     * @param iEpgID The ID of this table or <= 0 to create a new ID.
     * @param strName The name of this table.
     * @param strScraperName The name of the scraper to use.
     * @param database The EPG database
     */
    CPVREpg(int iEpgID,
            const std::string& strName,
            const std::string& strScraperName,
            const std::shared_ptr<CPVREpgDatabase>& database);

    /*!
     * @brief Create a new EPG instance.
     * @param iEpgID The ID of this table or <= 0 to create a new ID.
     * @param strName The name of this table.
     * @param strScraperName The name of the scraper to use.
     * @param channelData The channel data.
     * @param database The EPG database
     */
    CPVREpg(int iEpgID,
            const std::string& strName,
            const std::string& strScraperName,
            const std::shared_ptr<CPVREpgChannelData>& channelData,
            const std::shared_ptr<CPVREpgDatabase>& database);

    /*!
     * @brief Destroy this EPG instance.
     */
    virtual ~CPVREpg();

    /*!
     * @brief Get data for the channel associated with this EPG.
     * @return The data.
     */
    std::shared_ptr<CPVREpgChannelData> GetChannelData() const;

    /*!
     * @brief Set data for the channel associated with this EPG.
     * @param data The data.
     */
    void SetChannelData(const std::shared_ptr<CPVREpgChannelData>& data);

    /*!
     * @brief The id of the channel associated with this EPG.
     * @return The channel id or -1 if no channel is associated
     */
    int ChannelID() const;

    /*!
     * @brief Get the name of the scraper to use for this table.
     * @return The name of the scraper to use for this table.
     */
    const std::string& ScraperName() const;

    /*!
     * @brief Returns if there is a manual update pending for this EPG
     * @return True if there is a manual update pending, false otherwise
     */
    bool UpdatePending() const;

    /*!
     * @brief Clear the current tags and schedule manual update
     */
    void ForceUpdate();

    /*!
     * @brief Get the name of this table.
     * @return The name of this table.
     */
    const std::string& Name() const;

    /*!
     * @brief Get the database ID of this table.
     * @return The database ID of this table.
     */
    int EpgID() const;

    /*!
     * @brief Remove all entries from this EPG that finished before the given time.
     * @param time Delete entries with an end time before this time in UTC.
     */
    void Cleanup(const CDateTime& time);

    /*!
     * @brief Remove all entries from this EPG.
     */
    void Clear();

    /*!
     * @brief Get the event that is occurring now
     * @return The current event or NULL if it wasn't found.
     */
    std::shared_ptr<CPVREpgInfoTag> GetTagNow(bool bUpdateIfNeeded = true) const;

    /*!
     * @brief Get the event that will occur next
     * @return The next event or NULL if it wasn't found.
     */
    std::shared_ptr<CPVREpgInfoTag> GetTagNext() const;

    /*!
     * @brief Get the event that occurred previously
     * @return The previous event or NULL if it wasn't found.
     */
    std::shared_ptr<CPVREpgInfoTag> GetTagPrevious() const;

    /*!
     * @brief Get the event that occurs between the given begin and end time.
     * @param beginTime Minimum start time in UTC of the event.
     * @param endTime Maximum end time in UTC of the event.
     * @param bUpdateFromClient if true, try to fetch the event from the client if not found locally.
     * @return The found tag or NULL if it wasn't found.
     */
    std::shared_ptr<CPVREpgInfoTag> GetTagBetween(const CDateTime& beginTime, const CDateTime& endTime, bool bUpdateFromClient = false);

    /*!
     * @brief Get the event matching the given unique broadcast id
     * @param iUniqueBroadcastId The uid to look up
     * @return The matching event or NULL if it wasn't found.
     */
    std::shared_ptr<CPVREpgInfoTag> GetTagByBroadcastId(unsigned int iUniqueBroadcastId) const;

    /*!
     * @brief Update an entry in this EPG.
     * @param data The tag to update.
     * @param iClientId The id of the pvr client this event belongs to.
     * @return True if it was updated successfully, false otherwise.
     */
    bool UpdateEntry(const EPG_TAG* data, int iClientId);

    /*!
     * @brief Update an entry in this EPG.
     * @param tag The tag to update.
     * @param newState the new state of the event.
     * @return True if it was updated successfully, false otherwise.
     */
    bool UpdateEntry(const std::shared_ptr<CPVREpgInfoTag>& tag, EPG_EVENT_STATE newState);

    /*!
     * @brief Update the EPG from 'start' till 'end'.
     * @param start The start time.
     * @param end The end time.
     * @param iUpdateTime Update the table after the given amount of time has passed.
     * @param iPastDays Amount of past days from now on, for which past entries are to be kept.
     * @param database If given, the database to store the data.
     * @param bForceUpdate Force update from client even if it's not the time to
     * @return True if the update was successful, false otherwise.
     */
    bool Update(time_t start, time_t end, int iUpdateTime, int iPastDays, const std::shared_ptr<CPVREpgDatabase>& database, bool bForceUpdate = false);

    /*!
     * @brief Get all EPG tags.
     * @return The tags.
     */
    std::vector<std::shared_ptr<CPVREpgInfoTag>> GetTags() const;

    /*!
     * @brief Get all EPG tags for the given time frame, including "gap" tags.
     * @param timelineStart Start of time line
     * @param timelineEnd End of time line
     * @param minEventEnd The minimum end time of the events to return
     * @param maxEventStart The maximum start time of the events to return
     * @return The matching tags.
     */
    std::vector<std::shared_ptr<CPVREpgInfoTag>> GetTimeline(const CDateTime& timelineStart,
                                                             const CDateTime& timelineEnd,
                                                             const CDateTime& minEventEnd,
                                                             const CDateTime& maxEventStart) const;

    /*!
     * @brief Write the query to persist data into given database's queue
     * @param database The database.
     * @return True on success, false otherwise.
     */
    bool QueuePersistQuery(const std::shared_ptr<CPVREpgDatabase>& database);

    /*!
     * @brief Delete this table from the given database
     * @param database The database.
     * @return True if the table was deleted, false otherwise.
     */
    bool Delete(const std::shared_ptr<CPVREpgDatabase>& database);

    /*!
     * @brief Get the start time of the first entry in this table.
     * @return The first date in UTC.
     */
    CDateTime GetFirstDate() const;

    /*!
     * @brief Get the end time of the last entry in this table.
     * @return The last date in UTC.
     */
    CDateTime GetLastDate() const;

    /*!
     * @brief Notify observers when the currently active tag changed.
     * @return True if the playing tag has changed, false otherwise.
     */
    bool CheckPlayingEvent();

    /*!
     * @brief Convert a genre id and subid to a human readable name.
     * @param iID The genre ID.
     * @param iSubID The genre sub ID.
     * @return A human readable name.
     */
    static const std::string& ConvertGenreIdToString(int iID, int iSubID);

    /*!
     * @brief Check whether this EPG has unsaved data.
     * @return True if this EPG contains unsaved data, false otherwise.
     */
    bool NeedsSave() const;

    /*!
     * @brief Check whether this EPG is valid.
     * @return True if this EPG is valid and can be updated, false otherwise.
     */
    bool IsValid() const;

    /*!
     * @brief Query the events available for CEventStream
     */
    CEventStream<PVREvent>& Events() { return m_events; }

    /*!
     * @brief Lock the instance. No other thread gets access to this EPG until Unlock was called.
     */
    void Lock() { m_critSection.lock(); }

    /*!
     * @brief Unlock the instance. Other threads may get access to this EPG again.
     */
    void Unlock() { m_critSection.unlock(); }

  private:
    CPVREpg() = delete;
    CPVREpg(const CPVREpg&) = delete;
    CPVREpg& operator =(const CPVREpg&) = delete;

    /*!
     * @brief Update the EPG from a scraper set in the channel tag.
     * @todo not implemented yet for non-pvr EPGs
     * @param start Get entries with a start date after this time.
     * @param end Get entries with an end date before this time.
     * @param bForceUpdate Force update from client even if it's not the time to
     * @return True if the update was successful, false otherwise.
     */
    bool UpdateFromScraper(time_t start, time_t end, bool bForceUpdate);

    /*!
     * @brief Load all EPG entries from clients into a temporary table and update this table with the contents of that temporary table.
     * @param start Only get entries after this start time. Use 0 to get all entries before "end".
     * @param end Only get entries before this end time. Use 0 to get all entries after "begin". If both "begin" and "end" are 0, all entries will be updated.
     * @param bForceUpdate Force update from client even if it's not the time to
     * @return True if the update was successful, false otherwise.
     */
    bool LoadFromClients(time_t start, time_t end, bool bForceUpdate);

    /*!
     * @brief Update the contents of this table with the contents provided in "epg"
     * @param epg The updated contents.
     * @return True if the update was successful, false otherwise.
     */
    bool UpdateEntries(const CPVREpg& epg);

    /*!
     * @brief Remove all entries from this EPG that finished before the given amount of days.
     * @param iPastDays Delete entries with an end time before the given amount of days from now on.
     */
    void Cleanup(int iPastDays);

    bool m_bChanged = false; /*!< true if anything changed that needs to be persisted, false otherwise */
    bool m_bUpdatePending = false; /*!< true if manual update is pending */
    int m_iEpgID = 0; /*!< the database ID of this table */
    std::string m_strName; /*!< the name of this table */
    std::string m_strScraperName; /*!< the name of the scraper to use */
    CDateTime m_lastScanTime; /*!< the last time the EPG has been updated */
    mutable CCriticalSection m_critSection; /*!< critical section for changes in this table */
    bool m_bUpdateLastScanTime = false;
    std::shared_ptr<CPVREpgChannelData> m_channelData;
    CPVREpgTagsContainer m_tags;

    CEventSource<PVREvent> m_events;
  };
}
