/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Profile.h"

#include "XBDateTime.h"
#include "utils/XMLUtils.h"

CProfile::CLock::CLock(LockMode type, const std::string& password)
  : mode(type), code(password), settings(SettingsLock::NONE)
{
  programs = false;
  pictures = false;
  files = false;
  video = false;
  music = false;
  addonManager = false;
  games = false;
}

void CProfile::CLock::Validate()
{
  if (mode != LockMode::EVERYONE && (code == "-" || code.empty()))
    mode = LockMode::EVERYONE;

  if (code.empty() || mode == LockMode::EVERYONE)
    code = "-";
}

CProfile::CProfile(const std::string &directory, const std::string &name, const int id):
  m_directory(directory),
  m_name(name)
{
  m_id = id;
  m_bDatabases = true;
  m_bCanWrite = true;
  m_bSources = true;
  m_bCanWriteSources = true;
  m_bAddons = true;
}

CProfile::~CProfile(void) = default;

void CProfile::setDate()
{
  const CDateTime now = CDateTime::GetCurrentDateTime();
  std::string strDate = now.GetAsLocalizedDate(false);
  std::string strTime = now.GetAsLocalizedTime(TIME_FORMAT_GUESS);
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
  int settings = static_cast<int>(m_locks.settings);
  XMLUtils::GetInt(node, "locksettings", settings);
  m_locks.settings = static_cast<SettingsLock>(settings);
  XMLUtils::GetBoolean(node, "lockfiles", m_locks.files);
  XMLUtils::GetBoolean(node, "lockmusic", m_locks.music);
  XMLUtils::GetBoolean(node, "lockvideo", m_locks.video);
  XMLUtils::GetBoolean(node, "lockpictures", m_locks.pictures);
  XMLUtils::GetBoolean(node, "lockprograms", m_locks.programs);
  XMLUtils::GetBoolean(node, "lockgames", m_locks.games);

  int lockMode = static_cast<int>(m_locks.mode);
  XMLUtils::GetInt(node, "lockmode", lockMode);
  m_locks.mode = static_cast<LockMode>(lockMode);
  if (m_locks.mode > LockMode::QWERTY || m_locks.mode < LockMode::EVERYONE)
    m_locks.mode = LockMode::EVERYONE;

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
  XMLUtils::SetInt(node, "locksettings", static_cast<int>(m_locks.settings));
  XMLUtils::SetBoolean(node, "lockfiles", m_locks.files);
  XMLUtils::SetBoolean(node, "lockmusic", m_locks.music);
  XMLUtils::SetBoolean(node, "lockvideo", m_locks.video);
  XMLUtils::SetBoolean(node, "lockpictures", m_locks.pictures);
  XMLUtils::SetBoolean(node, "lockprograms", m_locks.programs);
  XMLUtils::SetBoolean(node, "lockgames", m_locks.games);

  XMLUtils::SetInt(node, "lockmode", static_cast<int>(m_locks.mode));
  XMLUtils::SetString(node,"lockcode", m_locks.code);
  XMLUtils::SetString(node, "lastdate", m_date);
}

void CProfile::SetLocks(const CProfile::CLock &locks)
{
  m_locks = locks;
  m_locks.Validate();
}
