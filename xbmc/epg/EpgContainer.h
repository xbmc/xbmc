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

#include "XBDateTime.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/Observer.h"

#include "Epg.h"
#include "EpgDatabase.h"

class CFileItemList;

namespace EPG
{
  #define g_EpgContainer CEpgContainer::Get()

  class CEpgContainer : public std::vector<CEpg *>,
                   public Observer,
                   public Observable,
                   private CThread
  {
    friend class CEpgDatabase;

  protected:
    CEpgDatabase m_database;           /*!< the EPG database */

    /** @name Configuration */
    //@{
    bool         m_bIgnoreDbForClient; /*!< don't save the EPG data in the database */
    int          m_iDisplayTime;       /*!< hours of EPG data to fetch */
    int          m_iUpdateTime;        /*!< update the full EPG after this period */
    //@}

    /** @name Class state properties */
    //@{
    bool         m_bDatabaseLoaded;    /*!< true if we already loaded the EPG from the database */
    time_t       m_iLastEpgCleanup;    /*!< the time the EPG was cleaned up */
    time_t       m_iLastEpgUpdate;     /*!< the time the EPG was updated */
    //@}

    CCriticalSection m_critSection;    /*!< a critical section for changes to this container */

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
     * @brief Load and update the EPG data.
     * @param bShowProgress Show a progress bar if true.
     * @return True if the update was successful, false otherwise.
     */
    virtual bool UpdateEPG(bool bShowProgress = false);

    /*!
     * @return True if a running update should be interrupted, false otherwise.
     */
    virtual bool InterruptUpdate(void) const;

    /*!
     * @brief Load or update a single table.
     * @param epg The table to update.
     * @param start The start time to use.
     * @param end The end time to use.
     * @return True if the load or update was successful, false otherwise.
     */
    virtual bool UpdateSingleTable(CEpg *epg, const time_t start, const time_t end);

    /*!
     * @brief A hook that will be called on every update thread iteration.
     */
    virtual void ProcessHook(const CDateTime &time) {};

    /*!
     * @brief A hook that is called after the tables have been loaded from the database.
     * @return True if the hook was executed successfully, false otherwise.
     */
    virtual bool AutoCreateTablesHook(void) { return true; }

    /*!
     * @brief Create a new EPG table.
     * @param iEpgId The table ID or -1 to create a new one.
     * @return The new table.
     */
    virtual CEpg *CreateEpg(int iEpgId);

  protected:
    /*!
     * @brief EPG update thread
     */
    virtual void Process(void);

    /*!
     * @brief Create a new EPG table container.
     */
    CEpgContainer(void);

  public:
    /*!
     * @brief Destroy this instance.
     */
    virtual ~CEpgContainer(void);

    /*!
     * @return An instance of this singleton.
     */
    static CEpgContainer &Get(void);

    /*!
     * @brief Get a pointer to the database instance.
     * @return A pointer to the database instance.
     */
    CEpgDatabase *GetDatabase(void) { return &m_database; }

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
     * @brief Clear all EPG entries.
     * @param bClearDb Clear the database too if true.
     */
    virtual void Clear(bool bClearDb = false);

    /*!
     * @brief Stop the update thread and unload all data.
     */
    virtual void Unload(void);

    /*!
     * @brief Clear the EPG and all it's database entries.
     */
    virtual void Reset(void) { Clear(true); }

    /*!
     * @brief Delete an EPG table from this container.
     * @param epg The table to delete.
     * @param bDeleteFromDatabase Delete this table from the database too if true.
     * @return
     */
    virtual bool DeleteEpg(const CEpg &epg, bool bDeleteFromDatabase = false);

    /*!
     * @brief Process a notification from an observable.
     * @param obs The observable that sent the update.
     * @param msg The update message.
     */
    virtual void Notify(const Observable &obs, const CStdString& msg);

    /*!
     * @brief Update an entry in this container.
     * @param tag The table to update.
     * @param bUpdateDatabase If set to true, this table will be persisted in the database.
     * @return True if it was updated successfully, false otherwise.
     */
    virtual bool UpdateEntry(const CEpg &entry, bool bUpdateDatabase = false);

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
    virtual const CDateTime GetFirstEPGDate(void) const;

    /*!
      * @brief Get the end time of the last entry.
      * @return The end time.
      */
    virtual const CDateTime GetLastEPGDate(void) const;

    /*!
     * @brief Get an EPG table given it's ID.
     * @param iEpgId The database ID of the table.
     * @return The table or NULL if it wasn't found.
     */
    virtual CEpg *GetById(int iEpgId) const;

    /*!
     * @brief Get an EPG table given it's index in this container.
     * @param iIndex The index.
     * @return The table or NULL if it wasn't found.
     */
    virtual CEpg *GetByIndex(unsigned int iIndex) const;
  };
}
