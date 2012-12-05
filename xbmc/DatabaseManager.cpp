/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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
#include "ViewDatabase.h"
#include "TextureDatabase.h"
#include "music/MusicDatabase.h"
#include "video/VideoDatabase.h"
#include "pvr/PVRDatabase.h"
#include "epg/EpgDatabase.h"
#include "settings/AdvancedSettings.h"

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
