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

#include "Database.h"
#include "DateTime.h"

class CEpg;
class CEpgInfoTag;

/** The EPG database */

class CEpgDatabase : public CDatabase
{
private:
  CDateTime lastScanTime; /*!< the last time the EPG has been updated */

public:
  /*!
   * @brief Create a new instance of the EPG database.
   */
  CEpgDatabase(void);

  /*!
   * @brief Destroy this instance.
   */
  virtual ~CEpgDatabase(void);

  /*!
   * @brief Open the database.
   * @return True if it was opened successfully, false otherwise.
   */
  virtual bool Open();

  /*!
   * @brief Get the minimal database version that is required to operate correctly.
   * @return The minimal database version.
   */
  virtual int GetMinVersion() const { return 1; };

  /*!
   * @brief Get the default sqlite database filename.
   * @return The default filename.
   */
  const char *GetDefaultDBName() const { return "MyEpg4.db"; };

  /*! @name EPG methods */
  //@{

  /*!
   * @brief Remove all EPG information from the database
   * @return True if the EPG information was erased, false otherwise.
   */
  bool DeleteEpg();

  /*!
   * @brief Erase all EPG entries for a table.
   * @param table The table to remove the EPG entries for.
   * @param start Remove entries after this time if set.
   * @param end Remove entries before this time if set.
   * @return True if the entries were removed successfully, false otherwise.
   */
  bool Delete(const CEpg &table, const CDateTime &start = NULL, const CDateTime &end = NULL);

  /*!
   * @brief Erase all EPG entries older than 1 day.
   * @return True if the entries were removed successfully, false otherwise.
   */
  bool DeleteOldEpgEntries();

  /*!
   * @brief Remove a single EPG entry.
   * @param tag The entry to remove.
   * @return True if it was removed successfully, false otherwise.
   */
  bool Delete(const CEpgInfoTag &tag);

  /*!
   * @brief Get all EPG entries for a table.
   * @param epg The EPG table to get the entries for.
   * @param start Get entries after this time if set.
   * @param end Get entries before this time if set.
   * @return The amount of entries that was added.
   */
  int Get(CEpg *epg, const CDateTime &start = NULL, const CDateTime &end = NULL);

  /**
   * Get the start time of the first entry for a channel.
   * If iChannelId is <= 0, then all entries will be searched.
   */
  /*!
   * @brief Get the start time of the first entry for a channel.
   * @param iEpgId The ID of the EPG table to get the entries for. If iEpgId is <= 0, then all entries will be searched.
   * @return The start time.
   */
  CDateTime GetEpgDataStart(long iEpgId = -1);

  /*!
   * @brief Get the end time of the last entry for a channel.
   * @param iEpgId The ID of the EPG table to get the entries for. If iEpgId is <= 0, then all entries will be searched.
   * @return The end time.
   */
  CDateTime GetEpgDataEnd(long iEpgId = -1);

  /*!
   * @brief Get the last stored EPG scan time.
   * @return The last scan time or -1 if it wasn't found.
   */
  CDateTime GetLastEpgScanTime();

  /*!
   * @brief Update the last scan time.
   * @return True if it was updated successfully, false otherwise.
   */
  bool PersistLastEpgScanTime(void);

  /*!
   * @brief Persist an EPG table. It's entries are not persisted.
   * @param epg The table to persist.
   * @param bSingleUpdate If true, this is a single update and the query will be executed immediately.
   * @param bLastUpdate If multiple updates were sent, set this to true on the last update to execute the queries.
   * @return The database ID of this entry or 0 if bSingleUpdate is false and the query was queued.
   */
  int Persist(const CEpg &epg, bool bSingleUpdate = true, bool bLastUpdate = false);

  /*!
   * @brief Persist an infotag.
   * @param tag The tag to persist.
   * @param bSingleUpdate If true, this is a single update and the query will be executed immediately.
   * @param bLastUpdate If multiple updates were sent, set this to true on the last update to execute the queries.
   * @return True if the query or queries were executed successfully, false otherwise.
   */
  bool Persist(const CEpgInfoTag &tag, bool bSingleUpdate = true, bool bLastUpdate = false);

  //@}

private:
  /*!
   * @brief Create the EPG database tables.
   * @return True if the tables were created successfully, false otherwise.
   */
  virtual bool CreateTables();

  /*!
   * @brief Update an old version of the database.
   * @param version The version to update the database from.
   * @return True if it was updated successfully, false otherwise.
   */
  virtual bool UpdateOldVersion(int version);
};
