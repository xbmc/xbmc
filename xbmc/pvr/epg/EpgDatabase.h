/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "dbwrappers/Database.h"
#include "threads/CriticalSection.h"

#include "pvr/epg/Epg.h"

namespace PVR
{
  class CPVREpgInfoTag;
  class CPVREpgContainer;

  /** The EPG database */

  class CPVREpgDatabase : public CDatabase
  {
  public:
    /*!
     * @brief Create a new instance of the EPG database.
     */
    CPVREpgDatabase(void) = default;

    /*!
     * @brief Destroy this instance.
     */
    ~CPVREpgDatabase(void) override = default;

    /*!
     * @brief Open the database.
     * @return True if it was opened successfully, false otherwise.
     */
    bool Open() override;

    /*!
     * @brief Close the database.
     */
    void Close() override;

    /*!
     * @brief Lock the database.
     */
    void Lock();

    /*!
     * @brief Unlock the database.
     */
    void Unlock();

    /*!
     * @brief Get the minimal database version that is required to operate correctly.
     * @return The minimal database version.
     */
    int GetSchemaVersion(void) const override { return 12; }

    /*!
     * @brief Get the default sqlite database filename.
     * @return The default filename.
     */
    const char *GetBaseDBName(void) const override { return "Epg"; }

    /*! @name EPG methods */
    //@{

    /*!
     * @brief Remove all EPG information from the database
     * @return True if the EPG information was erased, false otherwise.
     */
    bool DeleteEpg(void);

    /*!
     * @brief Delete an EPG table.
     * @param table The table to remove.
     * @return True if the table was removed successfully, false otherwise.
     */
    bool Delete(const CPVREpg &table);

    /*!
     * @brief Erase all EPG entries with an end time less than the given time.
     * @param maxEndTime The maximum allowed end time.
     * @return True if the entries were removed successfully, false otherwise.
     */
    bool DeleteEpgEntries(const CDateTime &maxEndTime);

    /*!
     * @brief Remove a single EPG entry.
     * @param tag The entry to remove.
     * @return True if it was removed successfully, false otherwise.
     */
    bool Delete(const CPVREpgInfoTag &tag);

    /*!
     * @brief Get all EPG tables from the database. Does not get the EPG tables' entries.
     * @param container The container to get the EPG tables for.
     * @return The entries.
     */
    std::vector<CPVREpgPtr> Get(const CPVREpgContainer &container);

    /*!
     * @brief Get all EPG entries for a table.
     * @param epg The EPG table to get the entries for.
     * @return The entries.
     */
    std::vector<CPVREpgInfoTagPtr> Get(const CPVREpg &epg);

    /*!
     * @brief Get the last stored EPG scan time.
     * @param iEpgId The table to update the time for. Use 0 for a global value.
     * @param lastScan The last scan time or -1 if it wasn't found.
     * @return True if the time was fetched successfully, false otherwise.
     */
    bool GetLastEpgScanTime(int iEpgId, CDateTime *lastScan);

    /*!
     * @brief Update the last scan time.
     * @param iEpgId The table to update the time for. Use 0 for a global value.
     * @param bQueueWrite Don't execute the query immediately but queue it if true.
     * @return True if it was updated successfully, false otherwise.
     */
    bool PersistLastEpgScanTime(int iEpgId = 0, bool bQueueWrite = false);

    /*!
     * @brief Persist an EPG table. It's entries are not persisted.
     * @param epg The table to persist.
     * @param bQueueWrite Don't execute the query immediately but queue it if true.
     * @return The database ID of this entry or 0 if bSingleUpdate is false and the query was queued.
     */
    int Persist(const CPVREpg &epg, bool bQueueWrite = false);

    /*!
     * @brief Persist an infotag.
     * @param tag The tag to persist.
     * @param bSingleUpdate If true, this is a single update and the query will be executed immediately.
     * @return The database ID of this entry or 0 if bSingleUpdate is false and the query was queued.
     */
    int Persist(const CPVREpgInfoTag &tag, bool bSingleUpdate = true);

    /*!
     * @return Last EPG id in the database
     */
    int GetLastEPGId(void);

    //@}

  private:
    /*!
     * @brief Create the EPG database tables.
     */
    void CreateTables() override;

    /*!
     * @brief Create the EPG database analytics.
     */
    void CreateAnalytics() override;

    /*!
     * @brief Update an old version of the database.
     * @param version The version to update the database from.
     */
    void UpdateTables(int version) override;

    int GetMinSchemaVersion() const override { return 4; }

    CCriticalSection m_critSection;
  };
}
