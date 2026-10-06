/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "media/MediaSection.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/SettingLevel.h"

#include <string>
#include <vector>

class CFileItem;
class CMediaSource;
class CProfileManager;
enum class LockMode;

class CGUIPassword : public ISettingCallback
{
public:
  CGUIPassword(void);
  ~CGUIPassword(void) override;
  template<typename T>
  bool IsItemUnlocked(T pItem,
                      KODI::MEDIA::MediaSection section,
                      const std::string& strLabel,
                      const std::string& strHeading);
  /*! \brief Tests if the user is allowed to access the share folder
   \param pItem The share folder item to access
   \param section The section the share belongs to
   \return If access is granted, returns \e true
   */
  bool IsItemUnlocked(CFileItem* pItem, KODI::MEDIA::MediaSection section);
  /*! \brief Tests if the user is allowed to access the Mediasource
   \param pItem The share folder item to access
   \param section The section the share belongs to
   \return If access is granted, returns \e true
   */
  bool IsItemUnlocked(CMediaSource* pItem, KODI::MEDIA::MediaSection section);
  bool CheckLock(LockMode btnType, const std::string& strPassword, int iHeading);
  bool CheckLock(LockMode btnType, const std::string& strPassword, int iHeading, bool& bCanceled);
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
  bool IsVideoUnlocked();
  bool IsMusicUnlocked();
  bool SetMasterLockMode(bool bDetails=true);
  bool LockSource(KODI::MEDIA::MediaSection section, const std::string& strName, bool bState);
  void LockSources(bool lock);
  void RemoveSourceLocks();
  bool IsDatabasePathUnlocked(const std::string& strPath, std::vector<CMediaSource>& sources);

  /*! \brief Helper function to test if a matching mediasource is currently unlocked
   for a given media file
   \note this function only returns the lock state. it does not provide unlock functionality
   \param section The section the share belongs to
   \param file The file to check lock state for
   \return If access is granted, returns \e true
   */
  bool IsMediaFileUnlocked(KODI::MEDIA::MediaSection section, const std::string& file) const;

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
   \param section The section the share belongs to
   \return If access is granted, returns \e true
   */
  bool IsMediaPathUnlocked(const std::shared_ptr<CProfileManager>& profileManager,
                           KODI::MEDIA::MediaSection section) const;

  std::string m_strMediaSourcePath;
  int VerifyPassword(LockMode btnType,
                     const std::string& strPassword,
                     const std::string& strHeading);
};

extern CGUIPassword g_passwordManager;
