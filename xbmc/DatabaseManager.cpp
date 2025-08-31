/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DatabaseManager.h"

#include "ServiceBroker.h"
#include "TextureDatabase.h"
#include "addons/AddonDatabase.h"
#include "music/MusicDatabase.h"
#include "pvr/PVRDatabase.h"
#include "pvr/epg/EpgDatabase.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "view/ViewDatabase.h"

#include <algorithm>
#include <mutex>

using namespace ADDON;
using namespace PVR;

CDatabaseManager::CDatabaseManager() :
  m_bIsUpgrading(false)
{
  // Initialize the addon database (must be before the addon manager is init'd)
  ADDON::CAddonDatabase db;
  UpdateDatabase(db);
}

CDatabaseManager::~CDatabaseManager() = default;

bool CDatabaseManager::Initialize()
{
  std::unique_lock lock(m_section);

  m_dbStatus.clear();

  CLog::Log(LOGDEBUG, "{}, updating databases...", __FUNCTION__);

  const std::shared_ptr<CAdvancedSettings> advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  // NOTE: Order here is important. In particular, CTextureDatabase has to be updated
  //       before CVideoDatabase.
  {
    ADDON::CAddonDatabase db;
    UpdateDatabase(db);
  }
  { CViewDatabase db; UpdateDatabase(db); }
  { CTextureDatabase db; UpdateDatabase(db); }
  { CMusicDatabase db; UpdateDatabase(db, &advancedSettings->m_databaseMusic); }
  { CVideoDatabase db; UpdateDatabase(db, &advancedSettings->m_databaseVideo); }
  { CPVRDatabase db; UpdateDatabase(db, &advancedSettings->m_databaseTV); }
  { CPVREpgDatabase db; UpdateDatabase(db, &advancedSettings->m_databaseEpg); }

  CLog::Log(LOGDEBUG, "{}, updating databases... DONE", __FUNCTION__);

  m_bIsUpgrading = false;
  m_connecting = false;

  return std::ranges::all_of(m_dbStatus,
                             [](const auto& db) { return db.second.dbStatus == DBStatus::READY; });
}

bool CDatabaseManager::CanOpen(const std::string &name)
{
  std::unique_lock lock(m_section);
  const auto i = m_dbStatus.find(name);
  if (i != m_dbStatus.end())
    return i->second.dbStatus == DBStatus::READY;
  return false; // db isn't even attempted to update yet
}

std::string CDatabaseManager::GetFullDatabaseNameByType(const std::string& dbType) const
{
  std::string defaultDbName;

  if (dbType == DBTYPE_ADDON)
    defaultDbName = CAddonDatabase::GetDefaultBaseDBName();
  else if (dbType == DBTYPE_EPG)
    defaultDbName = CPVREpgDatabase::GetDefaultBaseDBName();
  else if (dbType == DBTYPE_MUSIC)
    defaultDbName = CMusicDatabase::GetDefaultBaseDBName();
  else if (dbType == DBTYPE_PVR)
    defaultDbName = CPVRDatabase::GetDefaultBaseDBName();
  else if (dbType == DBTYPE_TEXTURE)
    defaultDbName = CTextureDatabase::GetDefaultBaseDBName();
  else if (dbType == DBTYPE_VIDEO)
    defaultDbName = CVideoDatabase::GetDefaultBaseDBName();
  else if (dbType == DBTYPE_VIEWMODE)
    defaultDbName = CViewDatabase::GetDefaultBaseDBName();
  else
    CLog::LogF(LOGERROR, "Unknown dbType '{}'", dbType);

  return GetFullDatabaseName(defaultDbName);
}

std::string CDatabaseManager::GetFullDatabaseName(const std::string& name) const
{
  std::unique_lock lock(m_section);
  const auto i = m_dbStatus.find(name);
  if (i != m_dbStatus.cend())
    return i->second.dbFullName;
  return {};
}

void CDatabaseManager::UpdateDatabase(CDatabase &db, DatabaseSettings *settings)
{
  std::string name = db.GetBaseDBName();
  UpdateStatus(name, DBStatus::UPDATING);
  if (Update(db, settings ? *settings : DatabaseSettings()))
    UpdateStatus(name, DBStatus::READY);
  else
    UpdateStatus(name, DBStatus::FAILED);
}

bool CDatabaseManager::Update(CDatabase &db, const DatabaseSettings &settings)
{
  DatabaseSettings dbSettings = settings;
  db.InitSettings(dbSettings);

  int version = db.GetSchemaVersion();
  std::string latestDb = dbSettings.name;
  latestDb += std::to_string(version);

  while (version >= db.GetMinSchemaVersion())
  {
    std::string dbName = dbSettings.name;
    if (version)
      dbName += std::to_string(version);

    m_connecting = true;
    const CDatabase::ConnectionState connectionState{db.Connect(dbName, dbSettings, false)};
    m_connecting = false;

    if (connectionState == CDatabase::ConnectionState::STATE_ERROR)
    {
      return false; // unable to connect
    }
    else if (connectionState == CDatabase::ConnectionState::STATE_CONNECTED)
    {
      // Database exists, take a copy for our current version (if needed) and reopen that one
      if (version < db.GetSchemaVersion())
      {
        CLog::Log(LOGINFO, "Old database found - updating from version {} to {}", version,
                  db.GetSchemaVersion());
        m_bIsUpgrading = true;

        bool copy_fail = false;

        try
        {
          db.CopyDB(latestDb);
        }
        catch (...)
        {
          CLog::Log(LOGERROR, "Unable to copy old database {} to new version {}", dbName, latestDb);
          copy_fail = true;
        }

        db.Close();

        if (copy_fail)
          return false;

        if (db.Connect(latestDb, dbSettings, false) != CDatabase::ConnectionState::STATE_CONNECTED)
        {
          CLog::Log(LOGERROR, "Unable to open freshly copied database {}", latestDb);
          return false;
        }
      }

      // yay - we have a copy of our db, now do our worst with it
      if (UpdateVersion(db, latestDb))
      {
        UpdateFullName(db.GetBaseDBName(), latestDb);
        return true;
      }

      // update failed - loop around and see if we have another one available
      db.Close();
    }

    // drop back to the previous version and try that
    version--;
  }
  // try creating a new one
  if (db.Connect(latestDb, dbSettings, true) == CDatabase::ConnectionState::STATE_CONNECTED)
    return true;

  // failed to update or open the database
  db.Close();
  CLog::Log(LOGERROR, "Unable to create new database");
  return false;
}

bool CDatabaseManager::UpdateVersion(CDatabase &db, const std::string &dbName)
{
  int version = db.GetDBVersion();
  bool bReturn = false;

  if (version < db.GetMinSchemaVersion())
  {
    CLog::Log(LOGERROR, "Can't update database {} from version {} - it's too old", dbName, version);
    return false;
  }
  else if (version < db.GetSchemaVersion())
  {
    CLog::Log(LOGINFO, "Attempting to update the database {} from version {} to {}", dbName,
              version, db.GetSchemaVersion());
    bool success = true;
    db.BeginTransaction();

    try
    {
      // drop old analytics
      db.DropAnalytics();
    }
    catch (...)
    {
      success = false;
    }
    if (!success)
    {
      CLog::Log(LOGERROR, "Exception dropping old analytics from {}", dbName);
      db.RollbackTransaction();
      return false;
    }

    db.CommitTransaction();
    db.BeginTransaction();

    try
    {
      // update table(s), recreate analytics, update version
      db.UpdateTables(version);
      db.CreateAnalytics();
      db.UpdateVersionNumber();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Exception updating database {} from version {} to {}", dbName, version,
                db.GetSchemaVersion());
      success = false;
    }
    if (!success)
    {
      CLog::Log(LOGERROR, "Error updating database {} from version {} to {}", dbName, version,
                db.GetSchemaVersion());
      db.RollbackTransaction();
      return false;
    }
    bReturn = db.CommitTransaction();
    CLog::Log(LOGINFO, "Update to version {} successful", db.GetSchemaVersion());
  }
  else if (version > db.GetSchemaVersion())
  {
    bReturn = false;
    CLog::Log(LOGERROR,
              "Can't open the database {} as it is a NEWER version than what we were expecting?",
              dbName);
  }
  else
  {
    bReturn = true;
    CLog::Log(LOGINFO, "Running database version {}", dbName);
  }

  return bReturn;
}

void CDatabaseManager::UpdateFullName(const std::string& name, const std::string& fullName)
{
  std::unique_lock lock(m_section);
  auto& entry{m_dbStatus[name]};
  entry.dbFullName = fullName;
}

void CDatabaseManager::UpdateStatus(const std::string& name, DBStatus status)
{
  std::unique_lock lock(m_section);
  auto& entry{m_dbStatus[name]};
  entry.dbStatus = status;
}

void CDatabaseManager::LocalizationChanged()
{
  std::unique_lock lock(m_section);

  // update video version type table after language changed
  CVideoDatabase videodb;
  if (videodb.Open())
  {
    videodb.UpdateVideoVersionTypeTable();
    CLog::Log(LOGDEBUG, "{}, Video version type table updated for new language settings",
              __FUNCTION__);
  }
}
