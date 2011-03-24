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

#include "epg/Epg.h"
#include "PVREpgInfoTag.h"
#include "addons/include/xbmc_pvr_types.h"

class CPVREpgContainer;

/** PVR EPG class */

class CPVREpg : public CEpg
{
private:
  /*!
   * @brief Update the EPG from a scraper set in the channel tag.
   * @param start Get entries with a start date after this time.
   * @param end Get entries with an end date before this time.
   * @return True if the update was successful, false otherwise.
   */
  bool UpdateFromScraper(time_t start, time_t end);

  /*!
   * @brief Create a new tag.
   * @return The new tag.
   */
  CEpgInfoTag *CreateTag(void);

protected:
  /*!
   * @brief Update this table's info with the given info. Doesn't change the EpgID.
   * @param epg The new info.
   * @param bUpdateDb If true, persist the changes.
   * @return True if the update was successful, false otherwise.
   */
  bool Update(const CEpg &epg, bool bUpdateDb = false);

public:
  /*!
   * @brief Create a new EPG instance for a channel.
   * @param channel The channel to create the EPG for.
   */
  CPVREpg(CPVRChannel *channel);

  /*!
   * @brief Check whether this EPG contains valid entries.
   * @return True if it has valid entries, false if not.
   */
  bool HasValidEntries(void) const;

  /*!
   * @brief Remove all entries from this EPG that finished before the given time
   *        and that have no timers set.
   * @param Time Delete entries with an end time before this time.
   */
  void Cleanup(const CDateTime &Time);

  /*!
   * @brief Update an entry in this EPG.
   * @param data The tag to update.
   * @param bUpdateDatabase If set to true, this event will be persisted in the database.
   * @return True if it was updated successfully, false otherwise.
   */
  bool UpdateEntry(const PVR_PROGINFO *data, bool bUpdateDatabase = false);
  bool UpdateFromClient(const PVR_PROGINFO *data, bool bUpdateDatabase = false) { return UpdateEntry(data, bUpdateDatabase); };

  /*!
   * @brief True if this is a table for a radio channel, false if it's for TV.
   * @return True if this is a table for a radio channel, false if it's for TV.
   */
  bool IsRadio(void) const;
};
