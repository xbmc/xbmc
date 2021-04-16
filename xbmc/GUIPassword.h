/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "LockType.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/SettingLevel.h"

#include <string>
#include <vector>

class CFileItem;
class CMediaSource;
class CProfileManager;

typedef std::vector<CMediaSource> VECSOURCES;

class CGUIPassword : public ISettingCallback
{
public:
  CGUIPassword(void);
  ~CGUIPassword(void) override;
  template<typename T>
  bool IsItemUnlocked(T pItem,
                      const std::string& strType,
                      const std::string& strLabel,
                      const std::string& strHeading);
  /*! \brief Tests if the user is allowed to access the share folder
   \param pItem The share folder item to access
   \param strType The type of share being accessed, e.g. "music", "video", etc. See CSettings::UpdateSources()
   \return If access is granted, returns \e true
   */
  bool IsItemUnlocked(CFileItem* pItem, const std::string &strType);
  /*! \brief Tests if the user is allowed to access the Mediasource
   \param pItem The share folder item to access
   \param strType The type of share being accessed, e.g. "music", "video", etc. See CSettings::UpdateSources()
   \return If access is granted, returns \e true
   */
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

  void SetMediaSourcePath(const std::string& strMediaSourcePath)
  {
    m_strMediaSourcePath = strMediaSourcePath;
  }

  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;

  bool bMasterUser;
  int iMasterLockRetriesLeft;

private:
  /*! \brief Helper function to test if the user is allowed to access the path
   by looking up the matching Mediasource. Used internally by CheckMenuLock.
   \param profileManager instance passed by ref. see CGUIPassword::CheckMenuLock
   \param strType The type of share being accessed, e.g. "music", "video", etc.
   \return If access is granted, returns \e true
   */
  bool IsMediaPathUnlocked(const std::shared_ptr<CProfileManager>& profileManager,
                           const std::string& strType);

  std::string m_strMediaSourcePath;
  int VerifyPassword(LockType btnType, const std::string& strPassword, const std::string& strHeading);
};

extern CGUIPassword g_passwordManager;
