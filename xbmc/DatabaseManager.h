/*
 *  Copyright (C) 2012-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <atomic>
#include <map>
#include <string>
#include <string_view>

class CDatabase;
class DatabaseSettings;

/*!
 \ingroup database
 \brief Database manager class for handling database updating

 Ensures that databases used in XBMC are up to date, and if a database can't be
 opened, ensures we don't continuously try it.

 */
class CDatabaseManager
{
public:
  CDatabaseManager();
  CDatabaseManager(const CDatabaseManager&) = delete;
  CDatabaseManager const& operator=(CDatabaseManager const&) = delete;
  ~CDatabaseManager();

  /*! \brief Initialize the database manager
   Checks that all databases are up to date, otherwise updates them.
   \return true if all databases are initialized successfully, false otherwise.
   */
  bool Initialize();

  /*! \brief Reset the database manager state.
   Must be called on profile changes (LoadProfile / LogOff) so that the
   next call to Initialize() re-runs the schema version check and migration
   for all databases under the new profile's database folder.
   */
  void Deinitialize();

  /*! \brief Check whether we can open a database.

   Checks whether the database has been updated correctly, if so returns true.
   If the database update failed, returns false immediately.
   If the database update is in progress, returns false.

   \param name the name of the database to check.
   \return true if the database can be opened, false otherwise.
   */
  bool CanOpen(const std::string &name);

  /*!
   * \brief Get the in-use database name for the database of type \p dbType, taking into account
   *        any custom naming set in advanced settings.
   * \param[in] dbType the type of the database, as defined in DatabaseTypes.h
   * \return the database name in-use (base name + schema version)
   */
  std::string GetDatabaseNameByType(std::string_view dbType) const;

  /*! \brief Check whether manager is connecting to the databases currently.
   \return true if connecting, false otherwise.
   */
  bool IsConnecting() const { return m_connecting; }

  /*! \brief Check whether manager is upgrading the databases currently.
   \return true if upgrading, false otherwise.
   */
  bool IsUpgrading() const { return m_bIsUpgrading; }

  void LocalizationChanged();

private:
  std::atomic<bool> m_bIsUpgrading;
  std::atomic<bool> m_connecting{false};
  bool m_initialized{false};

  enum class DBStatus
  {
    CLOSED,
    UPDATING,
    READY,
    FAILED
  };
  void UpdateStatus(const std::string& basename, DBStatus status);
  void UpdateDetails(const std::string& basename, const std::string& type, const std::string& name);
  bool UpdateDatabase(CDatabase& db, DatabaseSettings* settings = nullptr);
  bool Update(CDatabase &db, const DatabaseSettings &settings);
  bool UpdateVersion(CDatabase &db, const std::string &dbName);
  bool InitializeInternal();

  mutable CCriticalSection m_section; ///< Critical section protecting m_dbDetails.

  struct StringHash
  {
    using is_transparent = void; // Enables heterogeneous operations.
    std::size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
  };

  struct DBDetails
  {
    DBStatus m_status{DBStatus::CLOSED};
    std::string m_type;
    std::string m_name;
  };

  std::map<std::string, DBDetails, std::less<>> m_dbDetails;
};
