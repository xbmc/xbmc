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
#include "../addons/include/xbmc_pvr_types.h"

class CPVREpgInfoTag;
class CPVREpgs;

class CPVREpg : public std::vector<CPVREpgInfoTag*>
{
  friend class CPVREpgs;

private:
  CPVRChannel * m_Channel;        /* the channel this EPG belongs to */
  bool          m_bUpdateRunning; /* true if EPG is currently being updated */
  bool          m_bIsSorted;      /* remember if we're sorted or not */

  /**
   * Update the EPG from a scraper set in the channel tag
   * TODO: not implemented yet
   */
  bool UpdateFromScraper(time_t start, time_t end);

  /**
   * Update the EPG from a client
   */
  bool UpdateFromClient(time_t start, time_t end);

public:
  CPVREpg(CPVRChannel *channel);
  virtual ~CPVREpg(void);

  /**
   * Check if this EPG contains valid entries
   */
  bool HasValidEntries(void) const;

  /**
   * The channel this EPG belongs to
   */
  CPVRChannel *Channel(void) const { return m_Channel; }

  /**
   * Delete an infotag from this EPG
   */
  bool DeleteInfoTag(CPVREpgInfoTag *tag);

  /**
   * Remove all entries from this EPG that finished before the given time
   * and that have no timers set
   */
  void Cleanup(const CDateTime Time);

  /**
   * Remove all entries from this EPG that finished before the current time
   * and that have no timers set
   */
  void Cleanup(void);

  /**
   * Remove all entries from this EPG
   */
  void Clear(void);

  /**
   * Sort all entries in this EPG by date
   */
  void Sort(void);

  /**
   * Get the event that is occurring now
   */
  const CPVREpgInfoTag *InfoTagNow(void) const;

  /**
   * Get the event that will occur next
   */
  const CPVREpgInfoTag *InfoTagNext(void) const;

  /**
   * Get the infotag with the given ID
   * If it wasn't'found, try finding the event with the given start time
   */
  const CPVREpgInfoTag *InfoTag(long uniqueID, CDateTime StartTime) const;

  /**
   * Get the event that occurs at the given time
   */
  const CPVREpgInfoTag *InfoTagAround(CDateTime Time) const;

  /**
   * Get the event that occurs between the given begin and end time
   */
  const CPVREpgInfoTag *InfoTagBetween(CDateTime BeginTime, CDateTime EndTime) const;

  /**
   * True if this EPG is currently being updated, false otherwise
   */
  bool IsUpdateRunning(void) const { return m_bUpdateRunning; }

  /**
   * Mark the EPG as being update or no longer being updated
   */
  void SetUpdateRunning(bool OnOff) { m_bUpdateRunning = OnOff; }

  /**
   * Update an entry in this EPG
   * If bUpdateDatabase is set to true, this event will be persisted in the database
   */
  bool UpdateEntry(const CPVREpgInfoTag &tag, bool bUpdateDatabase = false);

  /**
   * Update an entry in this EPG
   * If bUpdateDatabase is set to true, this event will be persisted in the database
   */
  bool UpdateEntry(const PVR_PROGINFO *data, bool bUpdateDatabase = false);

  /**
   * Fix overlapping events from the tables
   */
  bool FixOverlappingEvents(bool bStore = true);

  /**
   * Update the EPG from 'start' till 'end'
   */
  bool Update(time_t start, time_t end, bool bStoreInDb = true);

  bool LoadFromDb();
};
