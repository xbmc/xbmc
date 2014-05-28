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

#ifndef ROOT_DATABASEMANAGER_H_INCLUDED
#define ROOT_DATABASEMANAGER_H_INCLUDED
#include "DatabaseManager.h"
#endif

#ifndef ROOT_UTILS_LOG_H_INCLUDED
#define ROOT_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif

#ifndef ROOT_ADDONS_ADDONDATABASE_H_INCLUDED
#define ROOT_ADDONS_ADDONDATABASE_H_INCLUDED
#include "addons/AddonDatabase.h"
#endif

#ifndef ROOT_VIEW_VIEWDATABASE_H_INCLUDED
#define ROOT_VIEW_VIEWDATABASE_H_INCLUDED
#include "view/ViewDatabase.h"
#endif

#ifndef ROOT_TEXTUREDATABASE_H_INCLUDED
#define ROOT_TEXTUREDATABASE_H_INCLUDED
#include "TextureDatabase.h"
#endif

#ifndef ROOT_MUSIC_MUSICDATABASE_H_INCLUDED
#define ROOT_MUSIC_MUSICDATABASE_H_INCLUDED
#include "music/MusicDatabase.h"
#endif

#ifndef ROOT_VIDEO_VIDEODATABASE_H_INCLUDED
#define ROOT_VIDEO_VIDEODATABASE_H_INCLUDED
#include "video/VideoDatabase.h"
#endif

#ifndef ROOT_PVR_PVRDATABASE_H_INCLUDED
#define ROOT_PVR_PVRDATABASE_H_INCLUDED
#include "pvr/PVRDatabase.h"
#endif

#ifndef ROOT_EPG_EPGDATABASE_H_INCLUDED
#define ROOT_EPG_EPGDATABASE_H_INCLUDED
#include "epg/EpgDatabase.h"
#endif

#ifndef ROOT_SETTINGS_ADVANCEDSETTINGS_H_INCLUDED
#define ROOT_SETTINGS_ADVANCEDSETTINGS_H_INCLUDED
#include "settings/AdvancedSettings.h"
#endif


using namespace std;
using namespace EPG;
using namespace PVR;

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
