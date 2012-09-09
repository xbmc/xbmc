#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "utils/StdString.h"
#include "GUIPassword.h"

#include <vector>

class TiXmlNode;

class CProfile
{
public:
  /*! \brief Class for handling lock status
   */
  class CLock
  {
  public:
    CLock(LockType type = LOCK_MODE_EVERYONE, const CStdString &password = "");
    void Validate();

    LockType mode;
    CStdString code;
    bool addonManager;
    bool settings;
    bool music;
    bool video;
    bool files;
    bool pictures;
    bool programs;
  };

  CProfile(const CStdString &directory = "", const CStdString &name = "", const int id = -1);
  ~CProfile(void);
  
  void Load(const TiXmlNode *node, int nextIdProfile);
  void Save(TiXmlNode *root) const;

  const CStdString& getDate() const { return m_date;}
  const int getId() const { return m_id; }
  const CStdString& getName() const { return m_name;}
  const CStdString& getDirectory() const { return m_directory;}
  const CStdString& getThumb() const { return m_thumb;}
  const CStdString& getLockCode() const { return m_locks.code;}
  LockType getLockMode() const { return m_locks.mode; }

  bool hasDatabases() const { return m_bDatabases; }
  bool canWriteDatabases() const { return m_bCanWrite; }
  bool hasSources() const { return m_bSources; }
  bool canWriteSources() const { return m_bCanWriteSources; }
  bool hasAddons() const { return m_bAddons; }
  bool settingsLocked() const { return m_locks.settings; }
  bool addonmanagerLocked() const { return m_locks.addonManager; }
  bool musicLocked() const { return m_locks.music; }
  bool videoLocked() const { return m_locks.video; }
  bool picturesLocked() const { return m_locks.pictures; }
  bool filesLocked() const { return m_locks.files; }
  bool programsLocked() const { return m_locks.programs; }
  const CLock &GetLocks() const { return m_locks; }

  void setName(const CStdString& name) {m_name = name;}
  void setDirectory(const CStdString& directory) {m_directory = directory;}
  void setDate(const CStdString& strDate) { m_date = strDate;}
  void setDate();
  void setThumb(const CStdString& thumb) {m_thumb = thumb;}
  void setDatabases(bool bHas) { m_bDatabases = bHas; }
  void setWriteDatabases(bool bCan) { m_bCanWrite = bCan; }
  void setSources(bool bHas) { m_bSources = bHas; }
  void setWriteSources(bool bCan) { m_bCanWriteSources = bCan; }
  void SetLocks(const CLock &locks);

private:
  CStdString m_directory;
  int m_id;
  CStdString m_name;
  CStdString m_date;
  CStdString m_thumb;
  bool m_bDatabases;
  bool m_bCanWrite;
  bool m_bSources;
  bool m_bCanWriteSources;
  bool m_bAddons;
  CLock m_locks;
};
