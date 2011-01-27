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
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/Observer.h"

#include "Epg.h"
#include "EpgDatabase.h"

class CFileItemList;
class CPVREpg;
class CPVREpgContainer;

class CEpgContainer : public std::vector<CEpg *>,
                 public Observer,
                 private CThread
{
  friend class CEpg;
  friend class CEpgDatabase;

  friend class CPVREpg;
  friend class CPVREpgContainer;

protected:
  CEpgDatabase m_database;           /*!< the EPG database */

  /** @name Configuration */
  //@{
  bool         m_bIgnoreDbForClient; /*!< don't save the EPG data in the database */
  int          m_iLingerTime;        /*!< hours to keep old EPG data */
  int          m_iDisplayTime;       /*!< hours of EPG data to fetch */
  int          m_iUpdateTime;        /*!< update the full EPG after this period */
  //@}

  /** @name Cached data */
  //@{
  CDateTime    m_First;              /*!< the earliest EPG date in our tables */
  CDateTime    m_Last;               /*!< the latest EPG date in our tables */
  //@}

  /** @name Class state properties */
  //@{
  bool         m_bDatabaseLoaded;    /*!< true if we already loaded the EPG from the database */
  time_t       m_iLastEpgCleanup;    /*!< the time the EPG was cleaned up */
  time_t       m_iLastEpgUpdate;     /*!< the time the EPG was updated */
  //@}

  /*!
   * @brief Load the EPG settings.
   * @return True if the settings were loaded successfully, false otherwise.
   */
  virtual bool LoadSettings(void);

  /*!
   * @brief Remove old EPG entries.
   * @return True if the old entries were removed successfully, false otherwise.
   */
  virtual bool RemoveOldEntries(void);

  /*!
   * @brief Update the last and first EPG date cache after changing or inserting a tag.
   * @param tag The tag that was changed or added.
   */
  virtual void UpdateFirstAndLastEPGDates(const CEpgInfoTag &tag);

  /*!
   * @brief Load and update the EPG data.
   * @param bShowProgress Show a progress bar if true.
   * @return True if the update was successful, false otherwise.
   */
  virtual bool UpdateEPG(bool bShowProgress = false);

  /*!
   * @brief Clear all EPG entries.
   * @param bClearDb Clear the database too if true.
   */
  virtual void Clear(bool bClearDb = false);

protected:
  /*!
   * @brief EPG update thread
   */
  virtual void Process(void);

  /*!
   * @brief Get an EPG table given it's ID.
   * @param iEpgId The database ID of the table.
   * @return The table or NULL if it wasn't found.
   */
  CEpg *GetById(int iEpgId);

  /*!
   * @brief Update an entry in this container.
   * @param tag The table to update.
   * @param bUpdateDatabase If set to true, this table will be persisted in the database.
   * @return True if it was updated successfully, false otherwise.
   */
  virtual bool UpdateEntry(const CEpg &entry, bool bUpdateDatabase = false);

public:
  /*!
   * @brief Load all EPG entries from the database.
   * @param bShowProgress Show a progress bar if true.
   * @return True if the update was successful, false otherwise.
   */
  virtual bool Load(bool bShowProgress = false);

  /*!
   * @brief A hook that will be called on every update thread iteration.
   */
  virtual void ProcessHook(const CDateTime &time) {};

  /*!
   * @brief Get a pointer to the database instance.
   * @return A pointer to the database instance.
   */
  CEpgDatabase *GetDatabase() { return &m_database; }

  /*!
   * @brief Create a new EPG table container.
   */
  CEpgContainer(void);

  /*!
   * @brief Destroy this instance.
   */
  virtual ~CEpgContainer(void);

  /*!
   * @brief Start the EPG update thread.
   */
  virtual void Start(void);

  /*!
   * @brief Stop the EPG update thread.
   * @return
   */
  virtual bool Stop(void);

  /*!
   * @brief Reset all EPG tables.
   * @param bClearDb If true, clear the database too.
   * @return True if all tables were reset, false otherwise.
   */
  virtual bool Reset(bool bClearDb = false);

  /*!
   * @brief Erase this EPG table.
   * @return True if it was erased, false otherwise.
   */
  virtual bool Erase(void);

  /*!
   * @brief Process a notification from an observable.
   * @param obs The observable that sent the update.
   * @param msg The update message.
   */
  virtual void Notify(const Observable &obs, const CStdString& msg);

  /*!
   * @brief Get all EPG tables and apply a filter.
   * @param results The fileitem list to store the results in.
   * @param filter The filter to apply.
   * @return The amount of entries that were added.
   */
  virtual int GetEPGSearch(CFileItemList* results, const EpgSearchFilter &filter);

  /*!
   * @brief Get all EPG tables.
   * @param results The fileitem list to store the results in.
   * @return The amount of entries that were added.
   */
  virtual int GetEPGAll(CFileItemList* results);

  /*!
   * @brief Get the start time of the first entry.
   * @return The start time.
   */
  virtual CDateTime GetFirstEPGDate(void) { return m_First; }

  /*!
    * @brief Get the end time of the last entry.
    * @return The end time.
    */
  virtual CDateTime GetLastEPGDate(void) { return m_Last; }
};

extern CEpgContainer g_EpgContainer; /*!< The container for all EPG tables */
