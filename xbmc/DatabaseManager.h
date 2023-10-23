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
   */
  void Initialize();

  /*! \brief Check whether we can open a database.

   Checks whether the database has been updated correctly, if so returns true.
   If the database update failed, returns false immediately.
   If the database update is in progress, returns false.

   \param name the name of the database to check.
   \return true if the database can be opened, false otherwise.
   */
  bool CanOpen(const std::string &name);

  bool IsUpgrading() const { return m_bIsUpgrading; }

  void LocalizationChanged();

private:
  std::atomic<bool> m_bIsUpgrading;

  enum DB_STATUS { DB_CLOSED, DB_UPDATING, DB_READY, DB_FAILED };
  void UpdateStatus(const std::string &name, DB_STATUS status);
  void UpdateDatabase(CDatabase &db, DatabaseSettings *settings = NULL);
  bool Update(CDatabase &db, const DatabaseSettings &settings);
  bool UpdateVersion(CDatabase &db, const std::string &dbName);

  CCriticalSection            m_section;     ///< Critical section protecting m_dbStatus.
  std::map<std::string, DB_STATUS> m_dbStatus;    ///< Our database status map.
};
