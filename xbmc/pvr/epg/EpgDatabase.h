/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dbwrappers/Database.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <vector>

class CDateTime;

namespace PVR
{
  class CPVREpg;
  class CPVREpgInfoTag;

  struct PVREpgSearchData;

  /** The EPG database */

  class CPVREpgDatabase : public CDatabase, public std::enable_shared_from_this<CPVREpgDatabase>
  {
  public:
    /*!
     * @brief Create a new instance of the EPG database.
     */
    CPVREpgDatabase() = default;

    /*!
     * @brief Destroy this instance.
     */
    ~CPVREpgDatabase() override = default;

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
    int GetSchemaVersion() const override { return 13; }

    /*!
     * @brief Get the default sqlite database filename.
     * @return The default filename.
     */
    const char* GetBaseDBName() const override { return "Epg"; }

    /*! @name EPG methods */
    //@{

    /*!
     * @brief Remove all EPG information from the database
     * @return True if the EPG information was erased, false otherwise.
     */
    bool DeleteEpg();

    /*!
     * @brief Delete an EPG table.
     * @param table The table to remove.
     * @return True if the table was removed successfully, false otherwise.
     */
    bool Delete(const CPVREpg& table);

    /*!
     * @brief Remove a single EPG entry.
     * @param tag The entry to remove.
     * @param bSingleDelete If true, this is a single delete and the query will be executed
     * immediately.
     * @return True if it was removed or queued successfully, false otherwise.
     */
    bool Delete(const CPVREpgInfoTag& tag, bool bSingleDelete);

    /*!
     * @brief Get all EPG tables from the database. Does not get the EPG tables' entries.
     * @return The entries.
     */
    std::vector<std::shared_ptr<CPVREpg>> GetAll();

    /*!
     * @brief Get all tags for a given EPG id.
     * @param iEpgID The ID of the EPG.
     * @return The entries.
     */
    std::vector<std::shared_ptr<CPVREpgInfoTag>> GetAllEpgTags(int iEpgID);

    /*!
     * @brief Get the start time of the first tag in this EPG.
     * @param iEpgID The ID of the EPG.
     * @return The time.
     */
    CDateTime GetFirstStartTime(int iEpgID);

    /*!
     * @brief Get the end time of the last tag in this EPG.
     * @param iEpgID The ID of the EPG.
     * @return The time.
     */
    CDateTime GetLastEndTime(int iEpgID);

    /*!
     * @brief Get the start time of the first tag with a start time greater than the given min time.
     * @param iEpgID The ID of the EPG.
     * @param minStart The min start time.
     * @return The time.
     */
    CDateTime GetMinStartTime(int iEpgID, const CDateTime& minStart);

    /*!
     * @brief Get the end time of the first tag with an end time less than the given max time.
     * @param iEpgID The ID of the EPG.
     * @param maxEnd The mx end time.
     * @return The time.
     */
    CDateTime GetMaxEndTime(int iEpgID, const CDateTime& maxEnd);

    /*!
     * @brief Get all EPG tags matching the given search criteria.
     * @param searchData The search criteria.
     * @return The matching tags.
     */
    std::vector<std::shared_ptr<CPVREpgInfoTag>> GetEpgTags(const PVREpgSearchData& searchData);

    /*!
     * @brief Get an EPG tag given its EPG id and unique broadcast ID.
     * @param iEpgID The ID of the EPG for the tag to get.
     * @param iUniqueBroadcastId The unique broadcast ID for the tag to get.
     * @return The tag or nullptr, if not found.
     */
    std::shared_ptr<CPVREpgInfoTag> GetEpgTagByUniqueBroadcastID(int iEpgID,
                                                                 unsigned int iUniqueBroadcastId);

    /*!
     * @brief Get an EPG tag given its EPG ID and start time.
     * @param iEpgID The ID of the EPG for the tag to get.
     * @param startTime The start time for the tag to get.
     * @return The tag or nullptr, if not found.
     */
    std::shared_ptr<CPVREpgInfoTag> GetEpgTagByStartTime(int iEpgID, const CDateTime& startTime);

    /*!
     * @brief Get the next EPG tag matching the given EPG id and min start time.
     * @param iEpgID The ID of the EPG for the tag to get.
     * @param minStartTime The min start time for the tag to get.
     * @return The tag or nullptr, if not found.
     */
    std::shared_ptr<CPVREpgInfoTag> GetEpgTagByMinStartTime(int iEpgID,
                                                            const CDateTime& minStartTime);

    /*!
     * @brief Get the next EPG tag matching the given EPG id and max end time.
     * @param iEpgID The ID of the EPG for the tag to get.
     * @param maxEndTime The max end time for the tag to get.
     * @return The tag or nullptr, if not found.
     */
    std::shared_ptr<CPVREpgInfoTag> GetEpgTagByMaxEndTime(int iEpgID, const CDateTime& maxEndTime);

    /*!
     * @brief Get all EPG tags matching the given EPG id, min start time and max end time.
     * @param iEpgID The ID of the EPG for the tags to get.
     * @param minStartTime The min start time for the tags to get.
     * @param maxEndTime The max end time for the tags to get.
     * @return The tags or empty vector, if no tags were found.
     */
    std::vector<std::shared_ptr<CPVREpgInfoTag>> GetEpgTagsByMinStartMaxEndTime(
        int iEpgID, const CDateTime& minStartTime, const CDateTime& maxEndTime);

    /*!
     * @brief Get all EPG tags matching the given EPG id, min end time and max start time.
     * @param iEpgID The ID of the EPG for the tags to get.
     * @param minEndTime The min end time for the tags to get.
     * @param maxStartTime The max start time for the tags to get.
     * @return The tags or empty vector, if no tags were found.
     */
    std::vector<std::shared_ptr<CPVREpgInfoTag>> GetEpgTagsByMinEndMaxStartTime(
        int iEpgID, const CDateTime& minEndTime, const CDateTime& maxStartTime);

    /*!
     * @brief Delete all EPG tags in range of given EPG id, min end time and max start time.
     * @param iEpgID The ID of the EPG for the tags to delete.
     * @param minEndTime The min end time for the tags to delete.
     * @param maxStartTime The max start time for the tags to delete.
     * @param bSingleDelete If true, this is a single delete and the query will be executed
     * immediately.
     * @return True if it was removed or queued successfully, false otherwise.
     */
    bool DeleteEpgTagsByMinEndMaxStartTime(int iEpgID,
                                           const CDateTime& minEndTime,
                                           const CDateTime& maxStartTime,
                                           bool bSingleDelete);

    /*!
     * @brief Get the last stored EPG scan time.
     * @param iEpgId The table to update the time for. Use 0 for a global value.
     * @param lastScan The last scan time or -1 if it wasn't found.
     * @return True if the time was fetched successfully, false otherwise.
     */
    bool GetLastEpgScanTime(int iEpgId, CDateTime* lastScan);

    /*!
     * @brief Update the last scan time.
     * @param iEpgId The table to update the time for.
     * @param lastScanTime The time to write to the database.
     * @param bQueueWrite Don't execute the query immediately but queue it if true.
     * @return True if it was updated successfully, false otherwise.
     */
    bool PersistLastEpgScanTime(int iEpgId, const CDateTime& lastScanTime, bool bQueueWrite);

    /*!
     * @brief Persist an EPG table. It's entries are not persisted.
     * @param epg The table to persist.
     * @param bQueueWrite Don't execute the query immediately but queue it if true.
     * @return The database ID of this entry or 0 if bQueueWrite is false and the query was queued.
     */
    int Persist(const CPVREpg& epg, bool bQueueWrite);

    /*!
     * @brief Erase all EPG tags with the given epg ID and an end time less than the given time.
     * @param iEpgId The ID of the EPG.
     * @param maxEndTime The maximum allowed end time.
     * @return True if the entries were removed successfully, false otherwise.
     */
    bool DeleteEpgTags(int iEpgId, const CDateTime& maxEndTime);

    /*!
     * @brief Erase all EPG tags with the given epg ID.
     * @param iEpgId The ID of the EPG.
     * @return True if the entries were removed successfully, false otherwise.
     */
    bool DeleteEpgTags(int iEpgId);

    /*!
     * @brief Persist an infotag.
     * @param tag The tag to persist.
     * @param bSingleUpdate If true, this is a single update and the query will be executed immediately.
     * @return The database ID of this entry or 0 if bSingleUpdate is false and the query was queued.
     */
    int Persist(const CPVREpgInfoTag& tag, bool bSingleUpdate = true);

    /*!
     * @return Last EPG id in the database
     */
    int GetLastEPGId();

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

    std::shared_ptr<CPVREpgInfoTag> CreateEpgTag(const std::unique_ptr<dbiplus::Dataset>& pDS);

    CCriticalSection m_critSection;
  };
}
