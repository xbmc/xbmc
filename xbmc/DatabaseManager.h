/*
 *  Copyright (C) 2012-2018 Team Kodi
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

  /*! \brief Check whether we can open a database.

   Checks whether the database has been updated correctly, if so returns true.
   If the database update failed, returns false immediately.
   If the database update is in progress, returns false.

   \param name the name of the database to check.
   \return true if the database can be opened, false otherwise.
   */
  bool CanOpen(const std::string &name);

  /*! \brief By reference of their database types, get the database name in-use
   with consideration of any custom naming set in advanced settings.
   \param dbType the type of the database, as defined in DatabaseTypes.h
   \return the database name in-use (base name + schema version)
   */
  std::string GetDatabaseNameByType(const std::string& dbType) const;

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

  enum class DBStatus
  {
    CLOSED,
    UPDATING,
    READY,
    FAILED
  };
  void UpdateStatus(const std::string& basename, DBStatus status);
  void UpdateDetails(const std::string& basename, const std::string& type, const std::string& name);
  void UpdateDatabase(CDatabase &db, DatabaseSettings *settings = NULL);
  bool Update(CDatabase &db, const DatabaseSettings &settings);
  bool UpdateVersion(CDatabase &db, const std::string &dbName);

  mutable CCriticalSection m_section; ///< Critical section protecting m_dbDetails.

  using DBBaseName = std::string;
  struct DBDetails
  {
    DBStatus m_status{DBStatus::CLOSED};
    std::string m_type;
    std::string m_name;
  };
  std::map<DBBaseName, DBDetails, std::less<>> m_dbDetails; ///< Our database details map.
};
