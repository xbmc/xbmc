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

  class CPVREpg : public EPG::CEpg
  {
  public:
    /*!
     * @brief Create a new EPG instance for a channel.
     * @param channel The channel to create the EPG for.
     * @param bLoadedFromDb True if this table was loaded from the database, false otherwise.
     */
    CPVREpg(CPVRChannel *channel, bool bLoadedFromDb = false);

    virtual ~CPVREpg(void);

    /*!
     * @see EPG::CEpg::Update()
     */
    bool Update(const CEpg &epg, bool bUpdateDb = false);

    /*!
     * @see EPG::CEpg::HasValidEntries()
     */
    bool HasValidEntries(void) const;

    /*!
     * @see EPG::CEpg::Clear()
     */
    void Clear(void);

    bool UpdateFromClient(const EPG_TAG *data, bool bUpdateDatabase = false) { return UpdateEntry(data, bUpdateDatabase); };

    /*!
     * @see EPG::CEpg::IsRadio()
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

  protected:
    /*!
     * @see EPG::CEpg::UpdateFromScraper()
     */
    bool UpdateFromScraper(time_t start, time_t end);

    /*!
     * @see EPG::CEpg::CreateTag()
     */
    EPG::CEpgInfoTag *CreateTag(void);

    /*!
     * @see EPG::CEpg::LoadFromClients()
     */
    bool LoadFromClients(time_t start, time_t end);

    /*!
     * @see EPG::CEpg::IsRemovableTag()
     */
    bool IsRemovableTag(const EPG::CEpgInfoTag *tag) const;
  };
}
