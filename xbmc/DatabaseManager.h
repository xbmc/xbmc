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

  /*! \brief By reference of their database types, get the actual full database name in-use
   with consideration of any custom naming set in advanced settings.
   \param dbType the default type of the database: addon, epg, music, pvr, texture, video, viewmode
   \return the full db name in-use (real db name + schema version)
   */
  static constexpr const char* DBTYPE_ADDON = "addon";
  static constexpr const char* DBTYPE_EPG = "epg";
  static constexpr const char* DBTYPE_MUSIC = "music";
  static constexpr const char* DBTYPE_PVR = "pvr";
  static constexpr const char* DBTYPE_TEXTURE = "texture";
  static constexpr const char* DBTYPE_VIDEO = "video";
  static constexpr const char* DBTYPE_VIEWMODE = "viewmode";
  std::string GetFullDatabaseNameByType(const std::string& dbType) const;

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
  void UpdateStatus(const std::string& name, DBStatus status);
  void UpdateFullName(const std::string& name, const std::string& fullName);
  void UpdateDatabase(CDatabase &db, DatabaseSettings *settings = NULL);
  bool Update(CDatabase &db, const DatabaseSettings &settings);
  bool UpdateVersion(CDatabase &db, const std::string &dbName);

  /*! \brief By reference of their default names, get the actual full database name in-use
   with consideration of any custom naming set in advanced settings.
   \param name the default name of the database (e.g. "MyVideos").
   \return the full db name (real db name + schema version)
   */
  std::string GetFullDatabaseName(const std::string& name) const;

  mutable CCriticalSection m_section; ///< Critical section protecting m_dbStatus.

  struct DBDetails
  {
    DBStatus dbStatus{DBStatus::CLOSED};
    std::string dbFullName;
  };
  std::map<std::string, DBDetails> m_dbStatus; ///< Our database status map.
};
