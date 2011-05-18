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

namespace PVR
{
  struct PVREpgSearchFilter;
  class CPVREpgContainer;

  /** PVR EPG class */

  class CPVREpg : public EPG::CEpg
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
    EPG::CEpgInfoTag *CreateTag(void);

    bool LoadFromClients(time_t start, time_t end);

  protected:
    /*!
     * @brief Update this table's info with the given info. Doesn't change the EpgID.
     * @param epg The new info.
     * @param bUpdateDb If true, persist the changes.
     * @return True if the update was successful, false otherwise.
     */
    bool Update(const CEpg &epg, bool bUpdateDb = false);

    bool IsRemovableTag(const EPG::CEpgInfoTag *tag) const;

  public:
    /*!
     * @brief Create a new EPG instance for a channel.
     * @param channel The channel to create the EPG for.
     * @param bLoadedFromDb True if this table was loaded from the database, false otherwise.
     */
    CPVREpg(CPVRChannel *channel, bool bLoadedFromDb = false);

    /*!
     * @brief Check whether this EPG contains valid entries.
     * @return True if it has valid entries, false if not.
     */
    bool HasValidEntries(void) const;

    /*!
     * @brief Remove all entries from this EPG.
     */
    void Clear(void);

    /*!
     * @brief Update an entry in this EPG.
     * @param data The tag to update.
     * @param bUpdateDatabase If set to true, this event will be persisted in the database.
     * @return True if it was updated successfully, false otherwise.
     */
    bool UpdateEntry(const EPG_TAG *data, bool bUpdateDatabase = false);
    bool UpdateFromClient(const EPG_TAG *data, bool bUpdateDatabase = false) { return UpdateEntry(data, bUpdateDatabase); };

    /*!
     * @brief True if this is a table for a radio channel, false if it's for TV.
     * @return True if this is a table for a radio channel, false if it's for TV.
     */
    bool IsRadio(void) const;

    /*!
     * @brief Get all EPG entries that and apply a filter.
     * @param results The file list to store the results in.
     * @param filter The filter to apply.
     * @return The amount of entries that were added.
     */
    int Get(CFileItemList *results, const PVREpgSearchFilter &filter) const;

    int Get(CFileItemList *results) const { return CEpg::Get(results); };
  };
}
