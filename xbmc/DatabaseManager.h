/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <atomic>
#include <map>
#include <string>
#include "threads/CriticalSection.h"

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
