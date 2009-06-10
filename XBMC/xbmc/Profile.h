#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "StdString.h"
#include "GUIPassword.h"

#include <vector>

class CProfile
{
public:
  CProfile(void);
  ~CProfile(void);

  const CStdString& getDate() const { return _date;}
  const CStdString& getName() const { return _name;}
  const CStdString& getDirectory() const { return _directory;}
  const CStdString& getThumb() const { return _thumb;}
  const CStdString& getLockCode() const { return _strLockCode;}
  LockType getLockMode() const { return _iLockMode; }

  bool hasDatabases() const { return _bDatabases; }
  bool canWriteDatabases() const { return _bCanWrite; }
  bool hasSources() const { return _bSources; }
  bool canWriteSources() const { return _bCanWriteSources; }
  bool hasAddons() const { return _bAddons; }
  bool settingsLocked() const { return _bLockSettings; }
  bool musicLocked() const { return _bLockMusic; }
  bool videoLocked() const { return _bLockVideo; }
  bool picturesLocked() const { return _bLockPictures; }
  bool filesLocked() const { return _bLockFiles; }
  bool programsLocked() const { return _bLockPrograms; }

  void setName(const CStdString& name) {_name = name;}
  void setDirectory(const CStdString& directory) {_directory = directory;}
  void setDate(const CStdString& strDate) { _date = strDate;}
  void setDate();
  void setLockMode(LockType iLockMode) { _iLockMode = iLockMode;}
  void setLockCode(const CStdString& strLockCode) { _strLockCode = strLockCode; }
  void setThumb(const CStdString& thumb) {_thumb = thumb;}
  void setDatabases(bool bHas) { _bDatabases = bHas; }
  void setWriteDatabases(bool bCan) { _bCanWrite = bCan; }
  void setSources(bool bHas) { _bSources = bHas; }
  void setWriteSources(bool bCan) { _bCanWriteSources = bCan; }

  void setSettingsLocked(bool bLocked) { _bLockSettings = bLocked; }
  void setFilesLocked(bool bLocked) { _bLockFiles = bLocked; }
  void setMusicLocked(bool bLocked) { _bLockMusic = bLocked; }
  void setVideoLocked(bool bLocked) { _bLockVideo = bLocked; }
  void setPicturesLocked(bool bLocked) { _bLockPictures = bLocked; }
  void setProgramsLocked(bool bLocked) { _bLockPrograms = bLocked; }

  CStdString _directory;
  CStdString _name;
  CStdString _date;
  CStdString _thumb;
  bool _bDatabases;
  bool _bCanWrite;
  bool _bSources;
  bool _bCanWriteSources;
  bool _bAddons;

  // lock stuff
  LockType _iLockMode;
  CStdString _strLockCode;
  bool _bLockSettings;
  bool _bLockMusic;
  bool _bLockVideo;
  bool _bLockFiles;
  bool _bLockPictures;
  bool _bLockPrograms;
};

typedef std::vector<CProfile> VECPROFILES;
typedef std::vector<CProfile>::iterator IVECPROFILES;

