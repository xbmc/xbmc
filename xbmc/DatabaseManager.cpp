/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#include "DatabaseManager.h"
#include "utils/log.h"
#include "addons/AddonDatabase.h"
#include "view/ViewDatabase.h"
#include "TextureDatabase.h"
#include "music/MusicDatabase.h"
#include "video/VideoDatabase.h"
#include "pvr/PVRDatabase.h"
#include "epg/EpgDatabase.h"
#include "games/addons/savestates/SavestateDatabase.h"
#include "settings/AdvancedSettings.h"
#include "cores/AudioEngine/Engines/ActiveAE/AudioDSPAddons/ActiveAEDSP.h"

using namespace PVR;
using namespace ActiveAE;

CDatabaseManager &CDatabaseManager::GetInstance()
{
  static CDatabaseManager s_manager;
  return s_manager;
}

CDatabaseManager::CDatabaseManager(): m_bIsUpgrading(false)
{
}

CDatabaseManager::~CDatabaseManager()
{
}

void CDatabaseManager::Initialize(bool addonsOnly)
{
  Deinitialize();
  { CAddonDatabase db; UpdateDatabase(db); }
  if (addonsOnly)
    return;
  CLog::Log(LOGDEBUG, "%s, updating databases...", __FUNCTION__);

  // NOTE: Order here is important. In particular, CTextureDatabase has to be updated
  //       before CVideoDatabase.
  { CViewDatabase db; UpdateDatabase(db); }
  { CTextureDatabase db; UpdateDatabase(db); }
  { CMusicDatabase db; UpdateDatabase(db, &g_advancedSettings.m_databaseMusic); }
  { CVideoDatabase db; UpdateDatabase(db, &g_advancedSettings.m_databaseVideo); }
  { CPVRDatabase db; UpdateDatabase(db, &g_advancedSettings.m_databaseTV); }
  { CPVREpgDatabase db; UpdateDatabase(db, &g_advancedSettings.m_databaseEpg); }
  { CActiveAEDSPDatabase db; UpdateDatabase(db, &g_advancedSettings.m_databaseADSP); }
  CLog::Log(LOGDEBUG, "%s, updating databases... DONE", __FUNCTION__);
  m_bIsUpgrading = false;
}

void CDatabaseManager::Deinitialize()
{
  CSingleLock lock(m_section);
  m_dbStatus.clear();
}

bool CDatabaseManager::CanOpen(const std::string &name)
{
  CSingleLock lock(m_section);
  std::map<std::string, DB_STATUS>::const_iterator i = m_dbStatus.find(name);
  if (i != m_dbStatus.end())
    return i->second == DB_READY;
  return false; // db isn't even attempted to update yet
}

void CDatabaseManager::UpdateDatabase(CDatabase &db, DatabaseSettings *settings)
{
  std::string name = db.GetBaseDBName();
  UpdateStatus(name, DB_UPDATING);
  if (Update(db, settings ? *settings : DatabaseSettings()))
    UpdateStatus(name, DB_READY);
  else
    UpdateStatus(name, DB_FAILED);
}

bool CDatabaseManager::Update(CDatabase &db, const DatabaseSettings &settings)
{
  DatabaseSettings dbSettings = settings;
  db.InitSettings(dbSettings);

  int version = db.GetSchemaVersion();
  std::string latestDb = dbSettings.name;
  latestDb += StringUtils::Format("%d", version);

  while (version >= db.GetMinSchemaVersion())
  {
    std::string dbName = dbSettings.name;
    if (version)
      dbName += StringUtils::Format("%d", version);

    if (db.Connect(dbName, dbSettings, false))
    {
      // Database exists, take a copy for our current version (if needed) and reopen that one
      if (version < db.GetSchemaVersion())
      {
        CLog::Log(LOGNOTICE, "Old database found - updating from version %i to %i", version, db.GetSchemaVersion());
        m_bIsUpgrading = true;

        bool copy_fail = false;

        try
        {
          db.CopyDB(latestDb);
        }
        catch (...)
        {
          CLog::Log(LOGERROR, "Unable to copy old database %s to new version %s", dbName.c_str(), latestDb.c_str());
          copy_fail = true;
        }

        db.Close();

        if (copy_fail)
          return false;

        if (!db.Connect(latestDb, dbSettings, false))
        {
          CLog::Log(LOGERROR, "Unable to open freshly copied database %s", latestDb.c_str());
          return false;
        }
      }

      // yay - we have a copy of our db, now do our worst with it
      if (UpdateVersion(db, latestDb))
        return true;

      // update failed - loop around and see if we have another one available
      db.Close();
    }

    // drop back to the previous version and try that
    version--;
  }
  // try creating a new one
  if (db.Connect(latestDb, dbSettings, true))
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
    CLog::Log(LOGERROR, "Can't update database %s from version %i - it's too old", dbName.c_str(), version);
    return false;
  }
  else if (version < db.GetSchemaVersion())
  {
    CLog::Log(LOGNOTICE, "Attempting to update the database %s from version %i to %i", dbName.c_str(), version, db.GetSchemaVersion());
    bool success = true;
    db.BeginTransaction();
    try
    {
      // drop old analytics, update table(s), recreate analytics, update version
      db.DropAnalytics();
      db.UpdateTables(version);
      db.CreateAnalytics();
      db.UpdateVersionNumber();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Exception updating database %s from version %i to %i", dbName.c_str(), version, db.GetSchemaVersion());
      success = false;
    }
    if (!success)
    {
      CLog::Log(LOGERROR, "Error updating database %s from version %i to %i", dbName.c_str(), version, db.GetSchemaVersion());
      db.RollbackTransaction();
      return false;
    }
    bReturn = db.CommitTransaction();
    CLog::Log(LOGINFO, "Update to version %i successful", db.GetSchemaVersion());
  }
  else if (version > db.GetSchemaVersion())
  {
    bReturn = false;
    CLog::Log(LOGERROR, "Can't open the database %s as it is a NEWER version than what we were expecting?", dbName.c_str());
  }
  else
  {
    bReturn = true;
    CLog::Log(LOGNOTICE, "Running database version %s", dbName.c_str());
  }

  return bReturn;
}

void CDatabaseManager::UpdateStatus(const std::string &name, DB_STATUS status)
{
  CSingleLock lock(m_section);
  m_dbStatus[name] = status;
}
