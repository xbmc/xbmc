#pragma once

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

#include <map>
#include <string>
#include <vector>

#include "settings/lib/ISettingCallback.h"
#include "settings/lib/Setting.h"

class CFileItem;
class CMediaSource;

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

namespace LOCK_LEVEL {
  /**
   Specifies, what Settings levels are locked for the user
   **/
  enum SETTINGS_LOCK
  {
    NONE,     //settings are unlocked => user can access all settings levels
    ALL,      //all settings are locked => user always has to enter password, when entering the settings screen
    STANDARD, //settings level standard and up are locked => user can still access the beginner levels
    ADVANCED, 
    EXPERT
  };
}

class CGUIPassword : public ISettingCallback
{
public:
  CGUIPassword(void);
  virtual ~CGUIPassword(void);
  bool IsItemUnlocked(CFileItem* pItem, const std::string &strType);
  bool IsItemUnlocked(CMediaSource* pItem, const std::string &strType);
  bool CheckLock(LockType btnType, const std::string& strPassword, int iHeading);
  bool CheckLock(LockType btnType, const std::string& strPassword, int iHeading, bool& bCanceled);
  bool IsProfileLockUnlocked(int iProfile=-1);
  bool IsProfileLockUnlocked(int iProfile, bool& bCanceled, bool prompt = true);
  bool IsMasterLockUnlocked(bool bPromptUser);
  bool IsMasterLockUnlocked(bool bPromptUser, bool& bCanceled);

  void UpdateMasterLockRetryCount(bool bResetCount);
  bool CheckStartUpLock();
  /*! \brief Checks if the current profile is allowed to access the given settings level
   \param level - The level to check
   \param enforce - If false, CheckSettingLevelLock is allowed to lower the current settings level
                    to a level we're allowed to access
   \returns true if we're allowed to access the settings
   */
  bool CheckSettingLevelLock(const SettingLevel& level, bool enforce = false);
  bool CheckMenuLock(int iWindowID);
  bool SetMasterLockMode(bool bDetails=true);
  bool LockSource(const std::string& strType, const std::string& strName, bool bState);
  void LockSources(bool lock);
  void RemoveSourceLocks();
  bool IsDatabasePathUnlocked(const std::string& strPath, VECSOURCES& vecSources);

  virtual void OnSettingAction(const CSetting *setting);

  bool bMasterUser;
  int iMasterLockRetriesLeft;

private:
  int VerifyPassword(LockType btnType, const std::string& strPassword, const std::string& strHeading);
};

extern CGUIPassword g_passwordManager;


