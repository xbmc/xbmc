/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Profile.h"
#include "utils/GUIInfoManager.h"
#include "guilib/XMLUtils.h"

CProfile::CProfile(const CStdString &directory, const CStdString &name)
{
  _directory = directory;
  _name = name;
  _bDatabases = true;
  _bCanWrite = true;
  _bSources = true;
  _bCanWriteSources = true;
  _bAddons = true;
  _bLockPrograms = false;
  _bLockPictures = false;
  _bLockFiles = false;
  _bLockVideo = false;
  _bLockMusic = false;
  _bLockSettings = false;
  _bLockAddonManager = false;
  _iLockMode = LOCK_MODE_EVERYONE;
}

CProfile::~CProfile(void)
{}

void CProfile::setDate()
{
  CStdString strDate = g_infoManager.GetDate(true);
  CStdString strTime = g_infoManager.GetTime();
  if (strDate.IsEmpty() || strTime.IsEmpty())
    setDate("-");
  else
    setDate(strDate+" - "+strTime);
}

void CProfile::Load(const TiXmlNode *node)
{
  XMLUtils::GetString(node, "name", _name);
  XMLUtils::GetPath(node, "directory", _directory);
  XMLUtils::GetPath(node, "thumbnail", _thumb);
  XMLUtils::GetBoolean(node, "hasdatabases", _bDatabases);
  XMLUtils::GetBoolean(node, "canwritedatabases", _bCanWrite);
  XMLUtils::GetBoolean(node, "hassources", _bSources);
  XMLUtils::GetBoolean(node, "canwritesources", _bCanWriteSources);
  XMLUtils::GetBoolean(node, "lockaddonmanager", _bLockAddonManager);
  XMLUtils::GetBoolean(node, "locksettings", _bLockSettings);
  XMLUtils::GetBoolean(node, "lockfiles", _bLockFiles);
  XMLUtils::GetBoolean(node, "lockmusic", _bLockMusic);
  XMLUtils::GetBoolean(node, "lockvideo", _bLockVideo);
  XMLUtils::GetBoolean(node, "lockpictures", _bLockPictures);
  XMLUtils::GetBoolean(node, "lockprograms", _bLockPrograms);
  
  int lockMode = _iLockMode;
  XMLUtils::GetInt(node, "lockmode", lockMode);
  _iLockMode = (LockType)lockMode;
  if (_iLockMode > LOCK_MODE_QWERTY || _iLockMode < LOCK_MODE_EVERYONE)
    _iLockMode = LOCK_MODE_EVERYONE;
  
  XMLUtils::GetString(node, "lockcode", _strLockCode);
  XMLUtils::GetString(node, "lastdate", _date);
}

void CProfile::Save(TiXmlNode *root) const
{
  TiXmlElement profileNode("profile");
  TiXmlNode *node = root->InsertEndChild(profileNode);

  XMLUtils::SetString(node, "name", _name);
  XMLUtils::SetPath(node, "directory", _directory);
  XMLUtils::SetPath(node, "thumbnail", _thumb);
  XMLUtils::SetBoolean(node, "hasdatabases", _bDatabases);
  XMLUtils::SetBoolean(node, "canwritedatabases", _bCanWrite);
  XMLUtils::SetBoolean(node, "hassources", _bSources);
  XMLUtils::SetBoolean(node, "canwritesources", _bCanWriteSources);
  XMLUtils::SetBoolean(node, "lockaddonmanager", _bLockAddonManager);
  XMLUtils::SetBoolean(node, "locksettings", _bLockSettings);
  XMLUtils::SetBoolean(node, "lockfiles", _bLockFiles);
  XMLUtils::SetBoolean(node, "lockmusic", _bLockMusic);
  XMLUtils::SetBoolean(node, "lockvideo", _bLockVideo);
  XMLUtils::SetBoolean(node, "lockpictures", _bLockPictures);
  XMLUtils::SetBoolean(node, "lockprograms", _bLockPrograms);

  XMLUtils::SetInt(node, "lockmode", _iLockMode);
  XMLUtils::SetString(node,"lockcode", _strLockCode);
  XMLUtils::SetString(node, "lastdate", _date);
}
