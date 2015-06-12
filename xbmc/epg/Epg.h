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

#include "FileItem.h"
#include "pvr/channels/PVRChannel.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

#include "EpgInfoTag.h"
#include "EpgSearchFilter.h"

namespace PVR
{
  class CPVRChannel;
}

/** EPG container for CEpgInfoTag instances */
namespace EPG
{
  class CEpg : public Observable
  {
    friend class CEpgDatabase;

  public:
    /*!
     * @brief Create a new EPG instance.
     * @param iEpgID The ID of this table or <= 0 to create a new ID.
     * @param strName The name of this table.
     * @param strScraperName The name of the scraper to use.
     * @param bLoadedFromDb True if this table was loaded from the database, false otherwise.
     */
    CEpg(int iEpgID, const std::string &strName = "", const std::string &strScraperName = "", bool bLoadedFromDb = false);

    /*!
     * @brief Create a new EPG instance for a channel.
     * @param channel The channel to create the EPG for.
     * @param bLoadedFromDb True if this table was loaded from the database, false otherwise.
     */
    CEpg(const PVR::CPVRChannelPtr &channel, bool bLoadedFromDb = false);

    /*!
     * @brief Destroy this EPG instance.
     */
    virtual ~CEpg(void);

    CEpg &operator =(const CEpg &right);

    /*!
     * @brief Load all entries for this table from the database.
     * @return True if any entries were loaded, false otherwise.
     */
    bool Load(void);

    /*!
     * @brief The channel this EPG belongs to.
     * @return The channel this EPG belongs to
     */
    PVR::CPVRChannelPtr Channel(void) const;

    int ChannelID(void) const;
    int ChannelNumber(void) const;
    int SubChannelNumber(void) const;

    /*!
     * @brief Channel the channel tag linked to this EPG table.
     * @param channel The new channel tag.
     */
    void SetChannel(const PVR::CPVRChannelPtr &channel);

    /*!
     * @brief Get the name of the scraper to use for this table.
     * @return The name of the scraper to use for this table.
     */
    const std::string &ScraperName(void) const { return m_strScraperName; }

    /*!
     * @brief Change the name of the scraper to use.
     * @param strScraperName The new scraper.
     */
    void SetScraperName(const std::string &strScraperName);

    /*!
     * @brief Specify if EPG should be manually updated on the next cycle
     * @param bUpdatePending True if EPG should be manually updated
     */
    void SetUpdatePending(bool bUpdatePending = true);

    /*!
     * @brief Returns if there is a manual update pending for this EPG
     * @returns True if there are is a manual update pending, false otherwise
     */
    bool UpdatePending(void) const;

    /*!
     * @brief Clear the current tag and schedule manual update
     */
    void ForceUpdate(void);

    /*!
     * @brief Get the name of this table.
     * @return The name of this table.
     */
    const std::string &Name(void) const { return m_strName; }

    /*!
     * @brief Changed the name of this table.
     * @param strName The new name.
     */
    void SetName(const std::string &strName);

    /*!
     * @brief Get the database ID of this table.
     * @return The database ID of this table.
     */
    int EpgID(void) const { return m_iEpgID; }

    /*!
     * @brief Check whether this EPG contains valid entries.
     * @return True if it has valid entries, false if not.
     */
    bool HasValidEntries(void) const;

    /*!
     * @return True if this EPG has a PVR channel set, false otherwise.
     */
    bool HasPVRChannel(void) const;

    /*!
     * @brief Remove all entries from this EPG that finished before the given time
     *        and that have no timers set.
     * @param Time Delete entries with an end time before this time in UTC.
     */
    void Cleanup(const CDateTime &Time);

    /*!
     * @brief Remove all entries from this EPG that finished before the given time
     *        and that have no timers set.
     */
    void Cleanup(void);

    /*!
     * @brief Remove all entries from this EPG.
     */
    void Clear(void);

    /*!
     * @brief Get the event that is occurring now
     * @return The current event or NULL if it wasn't found.
     */
    CEpgInfoTagPtr GetTagNow(bool bUpdateIfNeeded = true) const;

    /*!
     * @brief Get the event that will occur next
     * @return The next event or NULL if it wasn't found.
     */
    CEpgInfoTagPtr GetTagNext() const;

    /*!
     * @brief Get the event that occurs at the given time.
     * @param time The time in UTC to find the event for.
     * @return The found tag or NULL if it wasn't found.
     */
    CEpgInfoTagPtr GetTagAround(const CDateTime &time) const;

    /*!
     * Get the event that occurs between the given begin and end time.
     * @param beginTime Minimum start time in UTC of the event.
     * @param endTime Maximum end time in UTC of the event.
     * @return The found tag or NULL if it wasn't found.
     */
    CEpgInfoTagPtr GetTagBetween(const CDateTime &beginTime, const CDateTime &endTime) const;

    /*!
     * @brief Get the infotag with the given begin time.
     *
     * Get the infotag with the given ID.
     * If it wasn't found, try finding the event with the given start time
     *
     * @param beginTime The start time in UTC of the event to find if it wasn't found by it's unique ID.
     * @return The found tag or an empty tag if it wasn't found.
     */
    CEpgInfoTagPtr GetTag(const CDateTime &beginTime) const;
    /*!
     * @brief Get the infotag with the given ID.
     *
     * Get the infotag with the given ID.
     * If it wasn't found, try finding the event with the given start time
     *
     * @param uniqueID The unique ID of the event to find.
     * @return The found tag or an empty tag if it wasn't found.
     */
    CEpgInfoTagPtr GetTag(int uniqueID) const;

    /*!
     * @brief Update an entry in this EPG.
     * @param tag The tag to update.
     * @param bUpdateDatabase If set to true, this event will be persisted in the database.
     * @param bSort If set to false, epg entries will not be sorted after updating; used for mass updates
     * @return True if it was updated successfully, false otherwise.
     */
    bool UpdateEntry(const CEpgInfoTag &tag, bool bUpdateDatabase = false, bool bSort = true);

    /*!
     * @brief Update the EPG from 'start' till 'end'.
     * @param start The start time.
     * @param end The end time.
     * @param iUpdateTime Update the table after the given amount of time has passed.
     * @param bForceUpdate Force update from client even if it's not the time to
     * @return True if the update was successful, false otherwise.
     */
    bool Update(const time_t start, const time_t end, int iUpdateTime, bool bForceUpdate = false);

    /*!
     * @brief Get all EPG entries.
     * @param results The file list to store the results in.
     * @return The amount of entries that were added.
     */
    int Get(CFileItemList &results) const;

    /*!
     * @brief Get all EPG entries that and apply a filter.
     * @param results The file list to store the results in.
     * @param filter The filter to apply.
     * @return The amount of entries that were added.
     */
    int Get(CFileItemList &results, const EpgSearchFilter &filter) const;

    /*!
     * @brief Persist this table in the database.
     * @return True if the table was persisted, false otherwise.
     */
    bool Persist(void);

    /*!
     * @brief Get the start time of the first entry in this table.
     * @return The first date in UTC.
     */
    CDateTime GetFirstDate(void) const;

    /*!
     * @brief Get the end time of the last entry in this table.
     * @return The last date in UTC.
     */
    CDateTime GetLastDate(void) const;

    /*!
     * @return The last time this table was scanned.
     */
    CDateTime GetLastScanTime(void);

    /*!
     * @brief Notify observers when the currently active tag changed.
     */
    bool CheckPlayingEvent(void);

    /*!
     * @brief Convert a genre id and subid to a human readable name.
     * @param iID The genre ID.
     * @param iSubID The genre sub ID.
     * @return A human readable name.
     */
    static const std::string &ConvertGenreIdToString(int iID, int iSubID);

    /*!
     * @brief Update an entry in this EPG.
     * @param data The tag to update.
     * @param bUpdateDatabase If set to true, this event will be persisted in the database.
     * @return True if it was updated successfully, false otherwise.
     */
    bool UpdateEntry(const EPG_TAG *data, bool bUpdateDatabase = false);

    /*!
     * @return True if this is an EPG table for a radio channel, false otherwise.
     */
    bool IsRadio(void) const;

    CEpgInfoTagPtr GetNextEvent(const CEpgInfoTag& tag) const;
    CEpgInfoTagPtr GetPreviousEvent(const CEpgInfoTag& tag) const;

    size_t Size(void) const;

    bool NeedsSave(void) const;

    /*!
     * @return True when this EPG is valid and can be updated, false otherwise.
     */
    bool IsValid(void) const;
  protected:
    CEpg(void);

    /*!
     * @brief Update the EPG from a scraper set in the channel tag.
     * TODO: not implemented yet for non-pvr EPGs
     * @param start Get entries with a start date after this time.
     * @param end Get entries with an end date before this time.
     * @return True if the update was successful, false otherwise.
     */
    bool UpdateFromScraper(time_t start, time_t end);

    /*!
     * @brief Fix overlapping events from the tables.
     * @param bUpdateDb If set to yes, any changes to tags during fixing will be persisted to database
     * @return True if anything changed, false otherwise.
     */
    bool FixOverlappingEvents(bool bUpdateDb = false);

    /*!
     * @brief Add an infotag to this container.
     * @param tag The tag to add.
     */
    void AddEntry(const CEpgInfoTag &tag);

    /*!
     * @brief Load all EPG entries from clients into a temporary table and update this table with the contents of that temporary table.
     * @param start Only get entries after this start time. Use 0 to get all entries before "end".
     * @param end Only get entries before this end time. Use 0 to get all entries after "begin". If both "begin" and "end" are 0, all entries will be updated.
     * @return True if the update was successful, false otherwise.
     */
    bool LoadFromClients(time_t start, time_t end);

    /*!
     * @brief Update the contents of this table with the contents provided in "epg"
     * @param epg The updated contents.
     * @param bStoreInDb True to store the updated contents in the db, false otherwise.
     * @return True if the update was successful, false otherwise.
     */
    bool UpdateEntries(const CEpg &epg, bool bStoreInDb = true);

    bool IsRemovableTag(const EPG::CEpgInfoTag &tag) const;

    std::map<CDateTime, CEpgInfoTagPtr> m_tags;
    std::map<int, CEpgInfoTagPtr>       m_changedTags;
    std::map<int, CEpgInfoTagPtr>       m_deletedTags;
    bool                                m_bChanged;        /*!< true if anything changed that needs to be persisted, false otherwise */
    bool                                m_bTagsChanged;    /*!< true when any tags are changed and not persisted, false otherwise */
    bool                                m_bLoaded;         /*!< true when the initial entries have been loaded */
    bool                                m_bUpdatePending;  /*!< true if manual update is pending */
    int                                 m_iEpgID;          /*!< the database ID of this table */
    std::string                         m_strName;         /*!< the name of this table */
    std::string                         m_strScraperName;  /*!< the name of the scraper to use */
    mutable CDateTime                   m_nowActiveStart;  /*!< the start time of the tag that is currently active */

    CDateTime                           m_lastScanTime;    /*!< the last time the EPG has been updated */

    PVR::CPVRChannelPtr                 m_pvrChannel;      /*!< the channel this EPG belongs to */

    CCriticalSection                    m_critSection;     /*!< critical section for changes in this table */
    bool                                m_bUpdateLastScanTime;
  };
}
