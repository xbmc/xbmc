/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "LockMode.h"
#include "SettingsLock.h"

#include <string>

class TiXmlNode;

constexpr int INVALID_PROFILE_ID = -1;
constexpr int MASTER_PROFILE_ID = 0; // id of the master profile

class CProfile
{
public:
  /*! \brief Class for handling lock status
   */
  class CLock
  {
  public:
    CLock(LockMode type = LockMode::EVERYONE, const std::string& password = "");
    void Validate();

    LockMode mode;
    std::string code;
    bool addonManager;
    SettingsLock settings = SettingsLock::NONE;
    bool music;
    bool video;
    bool files;
    bool pictures;
    bool programs;
    bool games;
  };

  explicit CProfile(const std::string& directory = "",
                    const std::string& name = "",
                    const int id = INVALID_PROFILE_ID);
  ~CProfile(void);

  void Load(const TiXmlNode *node, int nextIdProfile);
  void Save(TiXmlNode *root) const;

  const std::string& getDate() const { return m_date;}
  int getId() const { return m_id; }
  const std::string& getName() const { return m_name;}
  const std::string& getDirectory() const { return m_directory;}
  const std::string& getThumb() const { return m_thumb;}
  const std::string& getLockCode() const { return m_locks.code;}
  LockMode getLockMode() const { return m_locks.mode; }

  bool hasDatabases() const { return m_bDatabases; }
  bool canWriteDatabases() const { return m_bCanWrite; }
  bool hasSources() const { return m_bSources; }
  bool canWriteSources() const { return m_bCanWriteSources; }
  bool hasAddons() const { return m_bAddons; }
  /**
   \brief Returns which settings levels are locked for the current profile
   \sa LOCK_LEVEL::SETTINGS_LOCK
   */
  SettingsLock settingsLockLevel() const { return m_locks.settings; }
  bool addonmanagerLocked() const { return m_locks.addonManager; }
  bool musicLocked() const { return m_locks.music; }
  bool videoLocked() const { return m_locks.video; }
  bool picturesLocked() const { return m_locks.pictures; }
  bool filesLocked() const { return m_locks.files; }
  bool programsLocked() const { return m_locks.programs; }
  bool gamesLocked() const { return m_locks.games; }
  const CLock &GetLocks() const { return m_locks; }
  bool needsRefresh() const { return m_bNeedsRefresh; }

  void setName(const std::string& name) {m_name = name;}
  void setDirectory(const std::string& directory) {m_directory = directory;}
  void setDate(const std::string& strDate) { m_date = strDate;}
  void setDate();
  void setThumb(const std::string& thumb) {m_thumb = thumb;}
  void setDatabases(bool bHas) { m_bDatabases = bHas; }
  void setWriteDatabases(bool bCan) { m_bCanWrite = bCan; }
  void setSources(bool bHas) { m_bSources = bHas; }
  void setWriteSources(bool bCan) { m_bCanWriteSources = bCan; }
  void SetLocks(const CLock &locks);
  void SetNeedsRefresh(const bool needsRefresh) { m_bNeedsRefresh = needsRefresh; }

private:
  std::string m_directory;
  int m_id;
  std::string m_name;
  std::string m_date;
  std::string m_thumb;
  bool m_bDatabases;
  bool m_bCanWrite;
  bool m_bSources;
  bool m_bCanWriteSources;
  bool m_bAddons;
  bool m_bNeedsRefresh;
  CLock m_locks;
};
