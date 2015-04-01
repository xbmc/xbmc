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
#include <stdexcept>
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "addons/AddonDatabase.h"
#include "view/ViewDatabase.h"
#include "TextureDatabase.h"
#include "music/MusicDatabase.h"
#include "video/VideoDatabase.h"
#include "pvr/PVRDatabase.h"
#include "epg/EpgDatabase.h"
#include "settings/AdvancedSettings.h"
#include "threads/ThreadLocal.h"

using namespace std;
using namespace EPG;
using namespace PVR;

static XbmcThreads::ThreadLocal<CVideoDatabase> videoDatabase;
static XbmcThreads::ThreadLocal<CViewDatabase> viewDatabase;
static XbmcThreads::ThreadLocal<CAddonDatabase> addonDatabase;
static XbmcThreads::ThreadLocal<CTextureDatabase> textureDatabase;
static XbmcThreads::ThreadLocal<CEpgDatabase> epgDatabase;
static XbmcThreads::ThreadLocal<CPVRDatabase> pvrDatabase;

CDatabaseManager &CDatabaseManager::Get()
{
  static CDatabaseManager s_manager;
  return s_manager;
}

CDatabaseManager::CDatabaseManager()
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
  { CEpgDatabase db; UpdateDatabase(db, &g_advancedSettings.m_databaseEpg); }
  CLog::Log(LOGDEBUG, "%s, updating databases... DONE", __FUNCTION__);
}

void CDatabaseManager::Deinitialize()
{
  CSingleLock lock(m_section);
  m_dbStatus.clear();
}

bool CDatabaseManager::CanOpen(const std::string &name)
{
  CSingleLock lock(m_section);
  map<string, DB_STATUS>::const_iterator i = m_dbStatus.find(name);
  if (i != m_dbStatus.end())
    return i->second == DB_READY;
  return false; // db isn't even attempted to update yet
}

void CDatabaseManager::UpdateDatabase(CDatabase &db, DatabaseSettings *settings)
{
  std::string name = db.GetBaseDBName();
  UpdateStatus(name, DB_UPDATING);
  if (db.Update(settings ? *settings : DatabaseSettings()))
    UpdateStatus(name, DB_READY);
  else
    UpdateStatus(name, DB_FAILED);
}

void CDatabaseManager::UpdateStatus(const std::string &name, DB_STATUS status)
{
  CSingleLock lock(m_section);
  m_dbStatus[name] = status;
}

void CDatabaseManager::OpenDatabase(CDatabase &database) const
{
  if (!database.Open())
    throw std::runtime_error(StringUtils::Format("Can't connect to database '%s'", database.GetBaseDBName()));
}

CVideoDatabase* CDatabaseManager::GetVideoDatabase()
{
  CSingleLock lock(m_section);
  CVideoDatabase *database = videoDatabase.get();
  if (database == NULL)
  {
    database = new CVideoDatabase();
    OpenDatabase(*database);
    videoDatabase.set(database);
  }
  return database;
}

CViewDatabase* CDatabaseManager::GetViewDatabase()
{
  CSingleLock lock(m_section);
  CViewDatabase *database = viewDatabase.get();
  if (database == NULL)
  {
    database = new CViewDatabase();
    OpenDatabase(*database);
    viewDatabase.set(database);
  }
  return database;
}

CAddonDatabase* CDatabaseManager::GetAddonDatabase()
{
  CSingleLock lock(m_section);
  CAddonDatabase *database = addonDatabase.get();
  if (database == NULL)
  {
    database = new CAddonDatabase();
    OpenDatabase(*database);
    addonDatabase.set(database);
  }
  return database;
}

CTextureDatabase* CDatabaseManager::GetTextureDatabase()
{
  CSingleLock lock(m_section);
  CTextureDatabase *database = textureDatabase.get();
  if (database == NULL)
  {
    database = new CTextureDatabase();
    OpenDatabase(*database);
    textureDatabase.set(database);
  }
  return database;
}

EPG::CEpgDatabase* CDatabaseManager::GetEpgDatabase()
{
  CSingleLock lock(m_section);
  CEpgDatabase *database = epgDatabase.get();
  if (database == NULL)
  {
    database = new CEpgDatabase();
    OpenDatabase(*database);
    epgDatabase.set(database);
  }
  return database;
}

PVR::CPVRDatabase* CDatabaseManager::GetPVRDatabase()
{
  CSingleLock lock(m_section);
  CPVRDatabase *database = pvrDatabase.get();
  if (database == NULL)
  {
    database = new CPVRDatabase();
    OpenDatabase(*database);
    pvrDatabase.set(database);
  }
  return database;
}

void CDatabaseManager::CloseDatabases()
{
  CSingleLock lock(m_section);
  
  if(videoDatabase.get() != NULL)
  {
    delete videoDatabase.get();
    videoDatabase.set(NULL);
  }
  if(viewDatabase.get() != NULL)
  {
    delete viewDatabase.get();
    viewDatabase.set(NULL);
  }
  if(addonDatabase.get() != NULL)
  {
    delete addonDatabase.get();
    addonDatabase.set(NULL);
  }
  if(textureDatabase.get() != NULL)
  {
    delete textureDatabase.get();
    textureDatabase.set(NULL);
  }
  if(epgDatabase.get() != NULL)
  {
    delete epgDatabase.get();
    epgDatabase.set(NULL);
  }
  if(pvrDatabase.get() != NULL)
  {
    delete pvrDatabase.get();
    pvrDatabase.set(NULL);
  }
}