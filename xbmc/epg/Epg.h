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

#include "FileItem.h"

#include "threads/CriticalSection.h"

#include "EpgInfoTag.h"
#include "EpgSearchFilter.h"

class CEpgContainer;
class CPVREpgContainer;
class CPVREpg;

/** EPG container for CEpgInfoTag instances */

class CEpg : public std::vector<CEpgInfoTag*>
{
  friend class CPVREpgContainer;
  friend class CEpgContainer;
  friend class CPVREpg;

private:
  bool                       m_bUpdateRunning; /*!< true if EPG is currently being updated */
  CStdString                 m_strName;        /*!< the name of this table */
  CStdString                 m_strScraperName; /*!< the name of the scraper to use */
  int                        m_iEpgID;         /*!< the database ID of this table */
  mutable const CEpgInfoTag *m_nowActive;      /*!< the tag that is currently active */

  mutable CCriticalSection   m_critSection;    /*!< critical section for changes in this table */

  CPVRChannel *              m_Channel;        /*!< the channel this EPG belongs to */

  /*!
   * @brief Update the EPG from a scraper set in the channel tag.
   * TODO: not implemented yet
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
   * @param bStore Store in the database if true.
   * @return True if the events were fixed successfully, false otherwise.
   */
  virtual bool FixOverlappingEvents(bool bStore = true);

  /*!
   * @brief Create a new tag.
   * @return The new tag.
   */
  virtual CEpgInfoTag *CreateTag(void);

  /*!
   * @brief Sort all entries in this EPG by date.
   */
  virtual void Sort(void);

  /*!
   * @brief Get the infotag with the given ID.
   *
   * Get the infotag with the given ID.
   * If it wasn't found, try finding the event with the given start time
   *
   * @param uniqueID The unique ID of the event to find.
   * @param StartTime The start time of the event to find if it wasn't found by it's unique ID.
   * @return The found tag or NULL if it wasn't found.
   */
  virtual const CEpgInfoTag *InfoTag(int uniqueID, const CDateTime &StartTime) const;

protected:
  /*!
   * @brief Update this table's info with the given info. Doesn't change the EpgID.
   * @param epg The new info.
   * @param bUpdateDb If true, persist the changes.
   * @return True if the update was successful, false otherwise.
   */
  virtual bool Update(const CEpg &epg, bool bUpdateDb = false);

  /*!
   * @brief Load all entries for this table from the database.
   * @return True if any entries were loaded, false otherwise.
   */
  bool Load(void);

public:
  /*!
   * @brief Create a new EPG instance.
   * @param iEpgID The ID of this table or <= 0 to create a new ID.
   * @param strName The name of this table.
   * @param strScraperName The name of the scraper to use.
   */
  CEpg(int iEpgID, const CStdString &strName = CStdString(), const CStdString &strScraperName = CStdString());

  /*!
   * @brief Destroy this EPG instance.
   */
  virtual ~CEpg(void);

  /*!
   * @brief Delete this EPG table from the database.
   * @return True if it was deleted successfully, false otherwise.
   */
  virtual bool Delete(void);

  /*!
   * @brief The channel this EPG belongs to.
   * @return The channel this EPG belongs to
   */
  const CPVRChannel *Channel(void) const { return m_Channel; }

  /*!
   * @brief Get the name of the scraper to use for this table.
   * @return The name of the scraper to use for this table.
   */
  const CStdString &ScraperName(void) const { return m_strScraperName; }

  /*!
   * @brief Get the name of this table.
   * @return The name of this table.
   */
  const CStdString &Name(void) const { return m_strName; }

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
   * @brief Delete an infotag from this EPG.
   * @param tag The tag to delete.
   * @return True if it was deleted successfully, false if not.
   */
  virtual bool DeleteInfoTag(CEpgInfoTag *tag);

  /*!
   * @brief Remove all entries from this EPG that finished before the given time
   *        and that have no timers set.
   * @param Time Delete entries with an end time before this time.
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
   * @param Time The time to find the event for.
   * @return The found tag or NULL if it wasn't found.
   */
  virtual const CEpgInfoTag *InfoTagAround(CDateTime Time) const;

  /*!
   * Get the event that occurs between the given begin and end time.
   * @param BeginTime Minimum start time of the event.
   * @param EndTime Maximum end time of the event.
   * @return The found tag or NULL if it wasn't found.
   */
  virtual const CEpgInfoTag *InfoTagBetween(CDateTime BeginTime, CDateTime EndTime) const;

  /*!
   * @brief True if this EPG is currently being updated, false otherwise.
   * @return True if this EPG is currently being updated, false otherwise.
   */
  virtual bool IsUpdateRunning(void) const { return m_bUpdateRunning; }

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
   * @param bStoreInDb Store in the database if true.
   * @return True if the update was successful, false otherwise.
   */
  virtual bool Update(time_t start, time_t end, bool bStoreInDb = true);

  /*!
   * @brief Get all EPG entries.
   * @param results The file list to store the results in.
   * @return The amount of entries that were added.
   */
  virtual int Get(CFileItemList *results) const;

  /*!
   * @brief Get all EPG entries that and apply a filter.
   * @param results The file list to store the results in.
   * @param filter The filter to apply.
   * @return The amount of entries that were added.
   */
  virtual int Get(CFileItemList *results, const EpgSearchFilter &filter) const;

  /*!
   * @brief Persist this table in the database.
   * @param bPersistTags Set to true to persist all changed tags in this container.
   * @return True if the table was persisted, false otherwise.
   */
  virtual bool Persist(bool bPersistTags = false);
};
