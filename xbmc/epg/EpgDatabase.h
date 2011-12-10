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

#include "dbwrappers/Database.h"
#include "XBDateTime.h"
#include "threads/CriticalSection.h"

namespace EPG
{
  class CEpg;
  class CEpgInfoTag;
  class CEpgContainer;

  /** The EPG database */

  class CEpgDatabase : public CDatabase
  {
  public:
    /*!
     * @brief Create a new instance of the EPG database.
     */
    CEpgDatabase(void) {};

    /*!
     * @brief Destroy this instance.
     */
    virtual ~CEpgDatabase(void) {};

    /*!
     * @brief Open the database.
     * @return True if it was opened successfully, false otherwise.
     */
    virtual bool Open(void);

    /*!
     * @brief Get the minimal database version that is required to operate correctly.
     * @return The minimal database version.
     */
    virtual int GetMinVersion(void) const { return 7; };

    /*!
     * @brief Get the default sqlite database filename.
     * @return The default filename.
     */
    const char *GetBaseDBName(void) const { return "Epg"; };

    /*! @name EPG methods */
    //@{

    /*!
     * @brief Remove all EPG information from the database
     * @return True if the EPG information was erased, false otherwise.
     */
    virtual bool DeleteEpg(void);

    /*!
     * @brief Erase all EPG entries for a table.
     * @param table The table to remove the EPG entries for.
     * @param start Remove entries after this time if set.
     * @param end Remove entries before this time if set.
     * @return True if the entries were removed successfully, false otherwise.
     */
    virtual bool Delete(const CEpg &table, const time_t start = 0, const time_t end = 0);

    /*!
     * @brief Erase all EPG entries older than 1 day.
     * @return True if the entries were removed successfully, false otherwise.
     */
    virtual bool DeleteOldEpgEntries(void);

    /*!
     * @brief Remove a single EPG entry.
     * @param tag The entry to remove.
     * @return True if it was removed successfully, false otherwise.
     */
    virtual bool Delete(const CEpgInfoTag &tag);

    /*!
     * @brief Get all EPG tables from the database. Does not get the EPG tables' entries.
     * @param container The container to fill.
     * @return The amount of entries that was added.
     */
    virtual int Get(CEpgContainer &container);

    /*!
     * @brief Get all EPG entries for a table.
     * @param epg The EPG table to get the entries for.
     * @return The amount of entries that was added.
     */
    virtual int Get(CEpg &epg);

    /*!
     * @brief Get the last stored EPG scan time.
     * @param iEpgId The table to update the time for. Use 0 for a global value.
     * @param lastScan The last scan time or -1 if it wasn't found.
     * @return True if the time was fetched successfully, false otherwise.
     */
    virtual bool GetLastEpgScanTime(int iEpgId, CDateTime *lastScan);

    /*!
     * @brief Update the last scan time.
     * @param iEpgId The table to update the time for. Use 0 for a global value.
     * @param bQueueWrite Don't execute the query immediately but queue it if true.
     * @return True if it was updated successfully, false otherwise.
     */
    virtual bool PersistLastEpgScanTime(int iEpgId = 0, bool bQueueWrite = false);

    /*!
     * @brief Persist an EPG table. It's entries are not persisted.
     * @param epg The table to persist.
     * @param bQueueWrite Don't execute the query immediately but queue it if true.
     * @return The database ID of this entry or 0 if bSingleUpdate is false and the query was queued.
     */
    virtual int Persist(const CEpg &epg, bool bQueueWrite = false);

    /*!
     * @brief Persist an infotag.
     * @param tag The tag to persist.
     * @param bSingleUpdate If true, this is a single update and the query will be executed immediately.
     * @return The database ID of this entry or 0 if bSingleUpdate is false and the query was queued.
     */
    virtual int Persist(const CEpgInfoTag &tag, bool bSingleUpdate = true);

    //@}

  protected:
    /*!
     * @brief Create the EPG database tables.
     * @return True if the tables were created successfully, false otherwise.
     */
    virtual bool CreateTables(void);

    /*!
     * @brief Update an old version of the database.
     * @param version The version to update the database from.
     * @return True if it was updated successfully, false otherwise.
     */
    virtual bool UpdateOldVersion(int version);

    CCriticalSection m_critSection;
  };
}
