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

#include "DateTime.h"
#include "Thread.h"
#include "CriticalSection.h"
#include "Observer.h"
#include "../addons/include/xbmc_pvr_types.h"
#include "utils/Thread.h"

class CPVREpg;
class CPVREpgInfoTag;
class CPVRChannel;
class CPVRDatabase;
class CFileItemList;
struct PVREpgSearchFilter;

class CPVREpgs : public std::vector<CPVREpg*>,
                 public Observer,
                 private CThread
{
  friend class CPVREpg;

private:
  /** @name Configuration */
  //@{
  bool             m_bIgnoreDbForClient; /*!< don't save the EPG data in the database */
  int              m_iLingerTime;        /*!< hours to keep old EPG data */
  int              m_iDisplayTime;       /*!< hours of EPG data to fetch */
  int              m_iUpdateTime;        /*!< update the full EPG after this period */
  //@}

  /** @name Cached data */
  //@{
  CDateTime        m_RadioFirst;         /*!< the earliest EPG date in our radio channel tables */
  CDateTime        m_RadioLast;          /*!< the latest EPG date in our radio channel tables */
  CDateTime        m_TVFirst;            /*!< the earliest EPG date in our tv channel tables */
  CDateTime        m_TVLast;             /*!< the latest EPG date in our tv channel tables */
  //@}

  /** @name Class state properties */
  //@{
  bool             m_bDatabaseLoaded;    /*!< true if we already loaded the EPG from the database */
  time_t           m_iLastEpgUpdate;     /*!< the time the EPG was updated */
  time_t           m_iLastEpgCleanup;    /*!< the time the EPG was cleaned up */
  time_t           m_iLastPointerUpdate; /*!< the time the now playing pointers were updated */
  //@}

  /*!
   * @brief Load the EPG for a channel using the pvr client.
   * @param channel The channel to get the EPG for.
   * @param epg The table to store the data in.
   * @param start The start time.
   * @param end The end time.
   * @return True if the EPG was loaded successfully, false otherwise.
   */
  bool GrabEPGForChannelFromClient(const CPVRChannel &channel, CPVREpg *epg, time_t start, time_t end);

  /*!
   * @brief Load the EPG for a channel using a scraper.
   * @param channel The channel to get the EPG for.
   * @param epg The table to store the data in.
   * @param start The start time.
   * @param end The end time.
   * @return True if the EPG was loaded successfully, false otherwise.
   */
  bool GrabEPGForChannelFromScraper(const CPVRChannel &channel, CPVREpg *epg, time_t start, time_t end);

  /*!
   * @brief Load the EPG for a channel using the pvr client or scraper.
   * @param channel The channel to get the EPG for.
   * @param epg The table to store the data in.
   * @param start The start time.
   * @param end The end time.
   * @return True if the EPG was loaded successfully, false otherwise.
   */
  bool GrabEPGForChannel(const CPVRChannel &channel, CPVREpg *epg, time_t start, time_t end);

  /*!
   * @brief Load the EPG settings.
   * @return True if the settings were loaded successfully, false otherwise.
   */
  bool LoadSettings();

  /*!
   * @brief Remove old EPG entries.
   * @return True if the old entries were removed successfully, false otherwise.
   */
  bool RemoveOldEntries();

  /**
   * Update the last and first EPG date cache
   */
  /*!
   * @brief Update the last and first EPG date cache after changing or inserting a tag.
   * @param tag The tag that was changed or added.
   */
  void UpdateFirstAndLastEPGDates(const CPVREpgInfoTag &tag);

  /*!
   * @brief Update the EPG "now playing" pointers for all channels.
   * @return True if the pointers were updated successfully, false otherwise.
   */
  bool UpdateAllChannelEPGPointers();

  /*!
   * @brief Load and update the EPG data.
   * @param bShowProgress Show a progress bar if true.
   * @return True if the update was successful, false otherwise.
   */
  bool UpdateEPG(bool bShowProgress = false);

  /*!
   * @brief Load all EPG entries from the database.
   * @param bShowProgress Show a progress bar if true.
   * @return True if the update was successful, false otherwise.
   */
  bool LoadFromDb(bool bShowProgress = false);

  /*!
   * @brief Clear all EPG entries.
   * @param bClearDb Clear the database too if true.
   */
  void Clear(bool bClearDb = false);

  /*!
   * @brief Create an EPG table for each channel.
   * @return True if all tables were created successfully, false otherwise.
   */
  bool CreateChannelEpgs(void);

protected:
  /*!
   * @brief EPG update thread
   */
  virtual void Process();

public:
  /*!
   * @brief Create a new EPG table container.
   */
  CPVREpgs();
  virtual ~CPVREpgs();

  /*!
   * @brief Start the EPG update thread.
   */
  void Start();

  /*!
   * @brief Stop the EPG update thread.
   * @return
   */
  bool Stop();

  /*!
   * @brief Reset all EPG tables.
   * @param bClearDb If true, clear the database too.
   * @return True if all tables were reset, false otherwise.
   */
  bool Reset(bool bClearDb = false);

  /*!
   * @brief Process a notification from an observable.
   * @param obs The observable that sent the update.
   * @param msg The update message.
   */
  void Notify(const Observable &obs, const CStdString& msg);

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
  CDateTime GetFirstEPGDate(bool bRadio = false);

  /*!
    * @brief Get the end time of the last entry.
    * @param bRadio Get radio items if true and TV items if false.
    * @return The end time.
    */
  CDateTime GetLastEPGDate(bool bRadio = false);
};

extern CPVREpgs g_PVREpgs; /*!< The container for all EPG tables */
