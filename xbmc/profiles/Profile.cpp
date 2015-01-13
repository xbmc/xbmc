/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "Profile.h"
#include "GUIInfoManager.h"
#include "utils/XMLUtils.h"

CProfile::CLock::CLock(LockType type, const std::string &password)
{
  programs = false;
  pictures = false;
  files = false;
  video = false;
  music = false;
  settings = LOCK_LEVEL::NONE;
  addonManager = false;
  mode = type;
  code = password;
}

void CProfile::CLock::Validate()
{
  if (mode != LOCK_MODE_EVERYONE && (code == "-" || code.empty()))
    mode = LOCK_MODE_EVERYONE;
  
  if (code.empty() || mode == LOCK_MODE_EVERYONE)
    code = "-";
}

CProfile::CProfile(const std::string &directory, const std::string &name, const int id)
{
  m_id = id;
  m_directory = directory;
  m_name = name;
  m_bDatabases = true;
  m_bCanWrite = true;
  m_bSources = true;
  m_bCanWriteSources = true;
  m_bAddons = true;
}

CProfile::~CProfile(void)
{}

void CProfile::setDate()
{
  std::string strDate = g_infoManager.GetDate(true);
  std::string strTime = g_infoManager.GetTime();
  if (strDate.empty() || strTime.empty())
    setDate("-");
  else
    setDate(strDate+" - "+strTime);
}

void CProfile::Load(const TiXmlNode *node, int nextIdProfile)
{
  if (!XMLUtils::GetInt(node, "id", m_id))
    m_id = nextIdProfile; 

  XMLUtils::GetString(node, "name", m_name);
  XMLUtils::GetPath(node, "directory", m_directory);
  XMLUtils::GetPath(node, "thumbnail", m_thumb);
  XMLUtils::GetBoolean(node, "hasdatabases", m_bDatabases);
  XMLUtils::GetBoolean(node, "canwritedatabases", m_bCanWrite);
  XMLUtils::GetBoolean(node, "hassources", m_bSources);
  XMLUtils::GetBoolean(node, "canwritesources", m_bCanWriteSources);
  XMLUtils::GetBoolean(node, "lockaddonmanager", m_locks.addonManager);
  XMLUtils::GetInt(node, "locksettings", (int&)m_locks.settings);
  XMLUtils::GetBoolean(node, "lockfiles", m_locks.files);
  XMLUtils::GetBoolean(node, "lockmusic", m_locks.music);
  XMLUtils::GetBoolean(node, "lockvideo", m_locks.video);
  XMLUtils::GetBoolean(node, "lockpictures", m_locks.pictures);
  XMLUtils::GetBoolean(node, "lockprograms", m_locks.programs);
  
  int lockMode = m_locks.mode;
  XMLUtils::GetInt(node, "lockmode", lockMode);
  m_locks.mode = (LockType)lockMode;
  if (m_locks.mode > LOCK_MODE_QWERTY || m_locks.mode < LOCK_MODE_EVERYONE)
    m_locks.mode = LOCK_MODE_EVERYONE;
  
  XMLUtils::GetString(node, "lockcode", m_locks.code);
  XMLUtils::GetString(node, "lastdate", m_date);
}

void CProfile::Save(TiXmlNode *root) const
{
  TiXmlElement profileNode("profile");
  TiXmlNode *node = root->InsertEndChild(profileNode);

  XMLUtils::SetInt(node, "id", m_id);
  XMLUtils::SetString(node, "name", m_name);
  XMLUtils::SetPath(node, "directory", m_directory);
  XMLUtils::SetPath(node, "thumbnail", m_thumb);
  XMLUtils::SetBoolean(node, "hasdatabases", m_bDatabases);
  XMLUtils::SetBoolean(node, "canwritedatabases", m_bCanWrite);
  XMLUtils::SetBoolean(node, "hassources", m_bSources);
  XMLUtils::SetBoolean(node, "canwritesources", m_bCanWriteSources);
  XMLUtils::SetBoolean(node, "lockaddonmanager", m_locks.addonManager);
  XMLUtils::SetInt(node, "locksettings", m_locks.settings);
  XMLUtils::SetBoolean(node, "lockfiles", m_locks.files);
  XMLUtils::SetBoolean(node, "lockmusic", m_locks.music);
  XMLUtils::SetBoolean(node, "lockvideo", m_locks.video);
  XMLUtils::SetBoolean(node, "lockpictures", m_locks.pictures);
  XMLUtils::SetBoolean(node, "lockprograms", m_locks.programs);

  XMLUtils::SetInt(node, "lockmode", m_locks.mode);
  XMLUtils::SetString(node,"lockcode", m_locks.code);
  XMLUtils::SetString(node, "lastdate", m_date);
}

void CProfile::SetLocks(const CProfile::CLock &locks)
{
  m_locks = locks;
  m_locks.Validate();
}
