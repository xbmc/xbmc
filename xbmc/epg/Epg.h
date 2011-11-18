#pragma once

/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "FileItem.h"

#include "threads/CriticalSection.h"

#include "EpgInfoTag.h"
#include "EpgSearchFilter.h"
#include "utils/Observer.h"

namespace PVR
{
  class CPVRChannel;
}

/** EPG container for CEpgInfoTag instances */
namespace EPG
{
  class CEpg : public std::vector<CEpgInfoTag*>, public Observable
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
    CEpg(int iEpgID, const CStdString &strName = "", const CStdString &strScraperName = "", bool bLoadedFromDb = false);

    /*!
     * @brief Create a new EPG instance for a channel.
     * @param channel The channel to create the EPG for.
     * @param bLoadedFromDb True if this table was loaded from the database, false otherwise.
     */
    CEpg(PVR::CPVRChannel *channel, bool bLoadedFromDb = false);

    /*!
     * @brief Destroy this EPG instance.
     */
    virtual ~CEpg(void);

    /*!
     * @brief Update this table's info with the given info. Doesn't change the EpgID.
     * @param epg The new info.
     * @param bUpdateDb If true, persist the changes.
     * @return True if the update was successful, false otherwise.
     */
    virtual bool UpdateMetadata(const CEpg &epg, bool bUpdateDb = false);

    /*!
     * @brief Load all entries for this table from the database.
     * @return True if any entries were loaded, false otherwise.
     */
    virtual bool Load(void);

    /*!
     * @brief The channel this EPG belongs to.
     * @return The channel this EPG belongs to
     */
    virtual const PVR::CPVRChannel *Channel(void) const { return m_Channel; }

    /*!
     * @brief Channel the channel tag linked to this EPG table.
     * @param channel The new channel tag.
     */
    virtual void SetChannel(PVR::CPVRChannel *channel) { m_Channel = channel; }

    /*!
     * @brief Get the name of the scraper to use for this table.
     * @return The name of the scraper to use for this table.
     */
    virtual const CStdString &ScraperName(void) const { return m_strScraperName; }

    /*!
     * @brief Change the name of the scraper to use.
     * @param strScraperName The new scraper.
     */
    virtual void SetScraperName(const CStdString &strScraperName);

    /*!
     * @brief Get the name of this table.
     * @return The name of this table.
     */
    virtual const CStdString &Name(void) const { return m_strName; }

    /*!
     * @brief Changed the name of this table.
     * @param strName The new name.
     */
    virtual void SetName(const CStdString &strName);

    /*!
     * @brief Get the database ID of this table.
     * @return The database ID of this table.
     */
    virtual int EpgID(void) const { return m_iEpgID; }

    /*!
     * @brief Check whether this EPG contains valid entries.
     * @return True if it has valid entries, false if not.
     */
    virtual bool HasValidEntries(void) const;

    /*!
     * @return True if this EPG has a PVR channel set, false otherwise.
     */
    virtual bool HasPVRChannel(void) const { return !(m_Channel == NULL); }

    /*!
     * @brief Remove all entries from this EPG that finished before the given time
     *        and that have no timers set.
     * @param Time Delete entries with an end time before this time in UTC.
     */
    virtual void Cleanup(const CDateTime &Time);

    /*!
     * @brief Remove all entries from this EPG that finished before the given time
     *        and that have no timers set.
     */
    virtual void Cleanup(void);

    /*!
     * @brief Remove all entries from this EPG.
     */
    virtual void Clear(void);

    /*!
     * @brief Get the event that is occurring now.
     * @return The current event.
     */
    virtual const CEpgInfoTag *InfoTagNow(void) const;

    /*!
     * @brief Get the event that will occur next.
     * @return The next event.
     */
    virtual const CEpgInfoTag *InfoTagNext(void) const;

    /*!
     * @brief Get the event that occurs at the given time.
     * @param time The time in UTC to find the event for.
     * @return The found tag or NULL if it wasn't found.
     */
    virtual const CEpgInfoTag *GetTagAround(const CDateTime &time) const;

    /*!
     * Get the event that occurs between the given begin and end time.
     * @param beginTime Minimum start time in UTC of the event.
     * @param endTime Maximum end time in UTC of the event.
     * @return The found tag or NULL if it wasn't found.
     */
    virtual const CEpgInfoTag *GetTagBetween(const CDateTime &beginTime, const CDateTime &endTime) const;

    /*!
     * @brief Get the infotag with the given ID.
     *
     * Get the infotag with the given ID.
     * If it wasn't found, try finding the event with the given start time
     *
     * @param uniqueID The unique ID of the event to find.
     * @param beginTime The start time in UTC of the event to find if it wasn't found by it's unique ID.
     * @return The found tag or NULL if it wasn't found.
     */
    virtual CEpgInfoTag *GetTag(int uniqueID, const CDateTime &beginTime) const;

    /*!
     * @brief Update an entry in this EPG.
     * @param tag The tag to update.
     * @param bUpdateDatabase If set to true, this event will be persisted in the database.
     * @return True if it was updated successfully, false otherwise.
     */
    virtual bool UpdateEntry(const CEpgInfoTag &tag, bool bUpdateDatabase = false);

    /*!
     * @brief Update the EPG from 'start' till 'end'.
     * @param start The start time.
     * @param end The end time.
     * @param iUpdateTime Update the table after the given amount of time has passed.
     * @return True if the update was successful, false otherwise.
     */
    virtual bool Update(const time_t start, const time_t end, int iUpdateTime);

    /*!
     * @brief Get all EPG entries.
     * @param results The file list to store the results in.
     * @return The amount of entries that were added.
     */
    virtual int Get(CFileItemList &results) const;

    /*!
     * @brief Get all EPG entries that and apply a filter.
     * @param results The file list to store the results in.
     * @param filter The filter to apply.
     * @return The amount of entries that were added.
     */
    virtual int Get(CFileItemList &results, const EpgSearchFilter &filter) const;

    /*!
     * @brief Persist this table in the database.
     * @param bUpdateLastScanTime True to update the last scan time in the db, false otherwise.
     * @return True if the table was persisted, false otherwise.
     */
    virtual bool Persist(bool bUpdateLastScanTime = false);

    /*!
     * @brief Get the start time of the first entry in this table.
     * @return The first date in UTC.
     */
    virtual const CDateTime &GetFirstDate(void) const;

    /*!
     * @brief Get the end time of the last entry in this table.
     * @return The last date in UTC.
     */
    virtual const CDateTime &GetLastDate(void) const;

    /*!
     * @return The last time this table was scanned.
     */
    virtual const CDateTime &GetLastScanTime(void);

    /*!
     * @brief Notify observers when the currently active tag changed.
     */
    virtual bool CheckPlayingEvent(void);

    /*!
     * @brief Convert a genre id and subid to a human readable name.
     * @param iID The genre ID.
     * @param iSubID The genre sub ID.
     * @return A human readable name.
     */
    static const CStdString &ConvertGenreIdToString(int iID, int iSubID);

    /*!
     * @brief Update an entry in this EPG.
     * @param data The tag to update.
     * @param bUpdateDatabase If set to true, this event will be persisted in the database.
     * @return True if it was updated successfully, false otherwise.
     */
    virtual bool UpdateEntry(const EPG_TAG *data, bool bUpdateDatabase = false);

    /*!
     * @return True if this is an EPG table for a radio channel, false otherwise.
     */
    virtual bool IsRadio(void) const;

  protected:
    /*!
     * @brief Update the EPG from a scraper set in the channel tag.
     * TODO: not implemented yet for non-pvr EPGs
     * @param start Get entries with a start date after this time.
     * @param end Get entries with an end date before this time.
     * @return True if the update was successful, false otherwise.
     */
    virtual bool UpdateFromScraper(time_t start, time_t end);

    /*!
     * @brief Persist all tags in this container.
     * @return True if all tags were persisted, false otherwise.
     */
    virtual bool PersistTags(void) const;

    /*!
     * @brief Fix overlapping events from the tables.
     * @return True if anything changed, false otherwise.
     */
    virtual bool FixOverlappingEvents(void);

    /*!
     * @brief Sort all entries in this EPG by date.
     */
    virtual void Sort(void);

    /*!
     * @brief Add an infotag to this container.
     * @param tag The tag to add.
     */
    virtual void AddEntry(const CEpgInfoTag &tag);

    /*!
     * @brief Load all EPG entries from clients into a temporary table and update this table with the contents of that temporary table.
     * @param start Only get entries after this start time. Use 0 to get all entries before "end".
     * @param end Only get entries before this end time. Use 0 to get all entries after "begin". If both "begin" and "end" are 0, all entries will be updated.
     * @return True if the update was successful, false otherwise.
     */
    virtual bool LoadFromClients(time_t start, time_t end);

    /*!
     * @brief Update the contents of this table with the contents provided in "epg"
     * @param epg The updated contents.
     * @param bStoreInDb True to store the updated contents in the db, false otherwise.
     * @return True if the update was successful, false otherwise.
     */
    virtual bool UpdateEntries(const CEpg &epg, bool bStoreInDb = true);

    virtual void UpdatePreviousAndNextPointers(void);

    /*!
     * @brief Update the cached first and last date.
     */
    virtual void UpdateFirstAndLastDates(void);

    virtual bool IsRemovableTag(const EPG::CEpgInfoTag *tag) const;

    bool                       m_bChanged;        /*!< true if anything changed that needs to be persisted, false otherwise */
    bool                       m_bTagsChanged;    /*!< true when any tags are changed and not persisted, false otherwise */
    bool                       m_bInhibitSorting; /*!< don't sort the table if this is true */
    bool                       m_bLoaded;         /*!< true when the initial entries have been loaded */
    int                        m_iEpgID;          /*!< the database ID of this table */
    CStdString                 m_strName;         /*!< the name of this table */
    CStdString                 m_strScraperName;  /*!< the name of the scraper to use */
    mutable const CEpgInfoTag *m_nowActive;       /*!< the tag that is currently active */

    CDateTime                  m_lastScanTime;    /*!< the last time the EPG has been updated */
    CDateTime                  m_firstDate;       /*!< start time of the first epg event in this table */
    CDateTime                  m_lastDate;        /*!< end time of the last epg event in this table */

    PVR::CPVRChannel *         m_Channel;         /*!< the channel this EPG belongs to */

    mutable CCriticalSection   m_critSection;     /*!< critical section for changes in this table */
  };
}
