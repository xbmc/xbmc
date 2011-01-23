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

#include "PVRChannel.h"
#include "PVREpgSearchFilter.h"
#include "../addons/include/xbmc_pvr_types.h"

class CPVREpgInfoTag;
class CPVREpgs;

/** PVR EPG class */

class CPVREpg : public std::vector<CPVREpgInfoTag*>
{
  friend class CPVREpgs;

private:
  CPVRChannel * m_Channel;        /*!< the channel this EPG belongs to */
  bool          m_bUpdateRunning; /*!< true if EPG is currently being updated */
  bool          m_bIsSorted;      /*!< remember if we're sorted or not */

  /*!
   * @brief Update the EPG from a scraper set in the channel tag.
   * TODO: not implemented yet
   * @param start Get entries with a start date after this time.
   * @param end Get entries with an end date before this time.
   * @return True if the update was successful, false otherwise.
   */
  bool UpdateFromScraper(time_t start, time_t end);

  /*!
   * @brief Update the EPG from a client.
   * @param start Get entries with a start date after this time.
   * @param end Get entries with an end date before this time.
   * @return True if the update was successful, false otherwise.
   */
  bool UpdateFromClient(time_t start, time_t end);

public:
  /*!
   * @brief Create a new EPG instance for a channel.
   * @param channel The channel to create the EPG for.
   */
  CPVREpg(CPVRChannel *channel);
  virtual ~CPVREpg(void);

  /*!
   * @brief Check whether this EPG contains valid entries.
   * @return True if it has valid entries, false if not.
   */
  bool HasValidEntries(void) const;

  /*!
   * @brief The channel this EPG belongs to.
   * @return The channel this EPG belongs to
   */
  CPVRChannel *Channel(void) const { return m_Channel; }

  /*!
   * @brief Delete an infotag from this EPG.
   * @param tag The tag to delete.
   * @return True if it was deleted successfully, false if not.
   */
  bool DeleteInfoTag(CPVREpgInfoTag *tag);

  /*!
   * @brief Remove all entries from this EPG that finished before the given time
   *        and that have no timers set.
   * @param Time Delete entries with an end time before this time.
   */
  void Cleanup(const CDateTime Time);

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
   * @brief Sort all entries in this EPG by date.
   */
  void Sort(void);

  /*!
   * @brief Get the event that is occurring now.
   * @return The current event.
   */
  const CPVREpgInfoTag *InfoTagNow(void) const;

  /*!
   * @brief Get the event that will occur next.
   * @return The next event.
   */
  const CPVREpgInfoTag *InfoTagNext(void) const;

  /*!
   * @brief Get the infotag with the given ID.
   *
   * Get the infotag with the given ID.
   * If it wasn't'found, try finding the event with the given start time
   *
   * @param uniqueID The unique ID of the event to find.
   * @param StartTime The start time of the event to find if it wasn't found by it's unique ID.
   * @return The found tag or NULL if it wasn't found.
   */
  const CPVREpgInfoTag *InfoTag(long uniqueID, CDateTime StartTime) const;

  /*!
   * @brief Get the event that occurs at the given time.
   * @param Time The time to find the event for.
   * @return The found tag or NULL if it wasn't found.
   */
  const CPVREpgInfoTag *InfoTagAround(CDateTime Time) const;

  /*!
   * Get the event that occurs between the given begin and end time.
   * @param BeginTime Minimum start time of the event.
   * @param EndTime Maximum end time of the event.
   * @return The found tag or NULL if it wasn't found.
   */
  const CPVREpgInfoTag *InfoTagBetween(CDateTime BeginTime, CDateTime EndTime) const;

  /*!
   * @brief True if this EPG is currently being updated, false otherwise.
   * @return True if this EPG is currently being updated, false otherwise.
   */
  bool IsUpdateRunning(void) const { return m_bUpdateRunning; }

  /*!
   * @brief Mark the EPG as being update or no longer being updated.
   * @param bUpdateRunning The new value.
   */
  void SetUpdateRunning(bool bUpdateRunning) { m_bUpdateRunning = bUpdateRunning; }

  /*!
   * @brief Update an entry in this EPG.
   * @param tag The tag to update.
   * @param bUpdateDatabase If set to true, this event will be persisted in the database.
   * @return True if it was updated successfully, false otherwise.
   */
  bool UpdateEntry(const CPVREpgInfoTag &tag, bool bUpdateDatabase = false);

  /*!
   * @brief Update an entry in this EPG.
   * @param data The tag to update.
   * @param bUpdateDatabase If set to true, this event will be persisted in the database.
   * @return True if it was updated successfully, false otherwise.
   */
  bool UpdateEntry(const PVR_PROGINFO *data, bool bUpdateDatabase = false);

  /*!
   * @brief Fix overlapping events from the tables.
   * @param bStore Store in the database if true.
   * @return True if the events were fixed successfully, false otherwise.
   */
  bool FixOverlappingEvents(bool bStore = true);

  /*!
   * @brief Update the EPG from 'start' till 'end'.
   * @param start The start time.
   * @param end The end time.
   * @param bStoreInDb Store in the database if true.
   * @return True if the update was successful, false otherwise.
   */
  bool Update(time_t start, time_t end, bool bStoreInDb = true);

  /*!
   * @brief Load all entries from the database.
   * @return True if the entries were loaded successfully, false otherwise.
   */
  bool LoadFromDb();

  /*!
   * @brief Get all EPG entries.
   * @param results The file list to store the results in.
   * @return The amount of entries that were added.
   */
  int Get(CFileItemList *results);

  /*!
   * @brief Get all EPG entries that and apply a filter.
   * @param results The file list to store the results in.
   * @param filter The filter to apply.
   * @return The amount of entries that were added.
   */
  int Get(CFileItemList *results, const PVREpgSearchFilter &filter);
};
