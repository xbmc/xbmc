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

#include "epg/EpgContainer.h"
#include "PVREpg.h"
#include "PVREpgSearchFilter.h"

class CPVREpgContainer : public CEpgContainer
{
  friend class CPVREpg;

private:
  /** @name Cached data */
  //@{
  CDateTime m_RadioFirst;         /*!< the earliest EPG radio date in our tables */
  CDateTime m_RadioLast;          /*!< the latest EPG radio date in our tables */
  CDateTime m_TVFirst;            /*!< the earliest EPG TV date in our tables */
  CDateTime m_TVLast;             /*!< the latest EPG TV date in our tables */
  //@}

  /*!
   * @brief Create an EPG table for each channel.
   * @return True if all tables were created successfully, false otherwise.
   */
  bool CreateChannelEpgs(void);

  /*!
   * @brief Update the last and first EPG date cache after changing or inserting a tag.
   * @param tag The tag that was changed or added.
   */
  void UpdateFirstAndLastEPGDates(const CPVREpgInfoTag &tag);

  /*!
   * @brief A hook that is called after the tables have been loaded from the database.
   * @return True if the hook was executed successfully, false otherwise.
   */
  bool AutoCreateTablesHook(void);

public:
  /*!
   * @brief Start the EPG update thread.
   */
  void Start();

  /*!
   * @brief Clear all EPG entries.
   * @param bClearDb Clear the database too if true.
   */
  void Clear(bool bClearDb = false);

  /*!
   * @brief Get all EPG tables and apply a filter.
   * @param results The fileitem list to store the results in.
   * @param filter The filter to apply.
   * @return The amount of entries that were added.
   */
  int GetEPGSearch(CFileItemList* results, const PVREpgSearchFilter &filter);

  /*!
   * @brief Get all EPG tables.
   * @param results The fileitem list to store the results in.
   * @param bRadio Get radio items if true and TV items if false.
   * @return The amount of entries that were added.
   */
  int GetEPGAll(CFileItemList* results, bool bRadio = false);

  /*!
   * @brief Get all entries that are active now.
   * @param results The fileitem list to store the results in.
   * @param bRadio Get radio items if true and TV items if false.
   * @return The amount of entries that were added.
   */
  int GetEPGNow(CFileItemList* results, bool bRadio = false);

  /*!
   * @brief Get all entries that will be active next.
   * @param results The fileitem list to store the results in.
   * @param bRadio Get radio items if true and TV items if false.
   * @return The amount of entries that were added.
   */
  int GetEPGNext(CFileItemList* results, bool bRadio = false);

  /*!
   * @brief Get the start time of the first entry.
   * @param bRadio Get radio items if true and TV items if false.
   * @return The start time.
   */
  const CDateTime &GetFirstEPGDate(bool bRadio = false);

  /*!
    * @brief Get the end time of the last entry.
    * @param bRadio Get radio items if true and TV items if false.
    * @return The end time.
    */
  const CDateTime &GetLastEPGDate(bool bRadio = false);
};

extern CPVREpgContainer g_PVREpgContainer; /*!< The container for all EPG tables */
