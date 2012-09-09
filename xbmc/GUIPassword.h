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

class CFileItem;
class CMediaSource;

#include <vector>
#include <map>

typedef std::vector<CMediaSource> VECSOURCES;

typedef enum
{
  LOCK_MODE_UNKNOWN            = -1,
  LOCK_MODE_EVERYONE           =  0,
  LOCK_MODE_NUMERIC            =  1,
  LOCK_MODE_GAMEPAD            =  2,
  LOCK_MODE_QWERTY             =  3,
  LOCK_MODE_SAMBA              =  4,
  LOCK_MODE_EEPROM_PARENTAL    =  5
} LockType;

class CGUIPassword
{
public:
  CGUIPassword(void);
  virtual ~CGUIPassword(void);
  bool IsItemUnlocked(CFileItem* pItem, const CStdString &strType);
  bool IsItemUnlocked(CMediaSource* pItem, const CStdString &strType);
  bool CheckLock(LockType btnType, const CStdString& strPassword, int iHeading);
  bool CheckLock(LockType btnType, const CStdString& strPassword, int iHeading, bool& bCanceled);
  bool IsProfileLockUnlocked(int iProfile=-1);
  bool IsProfileLockUnlocked(int iProfile, bool& bCanceled, bool prompt = true);
  bool IsMasterLockUnlocked(bool bPromptUser);
  bool IsMasterLockUnlocked(bool bPromptUser, bool& bCanceled);

  void UpdateMasterLockRetryCount(bool bResetCount);
  bool CheckStartUpLock();
  bool CheckMenuLock(int iWindowID);
  bool SetMasterLockMode(bool bDetails=true);
  bool LockSource(const CStdString& strType, const CStdString& strName, bool bState);
  void LockSources(bool lock);
  void RemoveSourceLocks();
  bool IsDatabasePathUnlocked(const CStdString& strPath, VECSOURCES& vecSources);

  bool bMasterUser;
  int iMasterLockRetriesLeft;

private:
  int VerifyPassword(LockType btnType, const CStdString& strPassword, const CStdString& strHeading);
};

extern CGUIPassword g_passwordManager;


