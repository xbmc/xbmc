/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIPassword.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "dialogs/GUIDialogGamepad.h"
#include "dialogs/GUIDialogNumeric.h"
#include "favourites/FavouritesService.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "media/MediaLockState.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "profiles/ProfileManager.h"
#include "profiles/dialogs/GUIDialogLockSettings.h"
#include "profiles/dialogs/GUIDialogProfileSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "view/ViewStateSettings.h"

#include <utility>

using namespace KODI::MESSAGING;

CGUIPassword::CGUIPassword(void)
{
  iMasterLockRetriesLeft = -1;
  bMasterUser = false;
}
CGUIPassword::~CGUIPassword(void) = default;

template<typename T>
bool CGUIPassword::IsItemUnlocked(T pItem,
                                  const std::string& strType,
                                  const std::string& strLabel,
                                  const std::string& strHeading)
{
  const std::shared_ptr<CProfileManager> profileManager =
      CServiceBroker::GetSettingsComponent()->GetProfileManager();
  if (profileManager->GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE)
    return true;

  while (pItem->m_iHasLock > LOCK_STATE_LOCK_BUT_UNLOCKED)
  {
    const std::string strLockCode = pItem->m_strLockCode;
    int iResult = 0; // init to user succeeded state, doing this to optimize switch statement below
    if (!g_passwordManager.bMasterUser) // Check if we are the MasterUser!
    {
      if (0 != CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                   CSettings::SETTING_MASTERLOCK_MAXRETRIES) &&
          pItem->m_iBadPwdCount >= CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                                       CSettings::SETTING_MASTERLOCK_MAXRETRIES))
      {
        // user previously exhausted all retries, show access denied error
        HELPERS::ShowOKDialogText(CVariant{12345}, CVariant{12346});
        return false;
      }
      // show the appropriate lock dialog
      iResult = VerifyPassword(pItem->m_iLockMode, strLockCode, strHeading);
    }
    switch (iResult)
    {
    case -1:
      { // user canceled out
        return false;
        break;
      }
    case 0:
      {
        // password entry succeeded
        pItem->m_iBadPwdCount = 0;
        pItem->m_iHasLock = LOCK_STATE_LOCK_BUT_UNLOCKED;
        g_passwordManager.LockSource(strType, strLabel, false);
        CMediaSourceSettings::GetInstance().UpdateSource(strType, strLabel, "badpwdcount",
                                                         std::to_string(pItem->m_iBadPwdCount));
        CMediaSourceSettings::GetInstance().Save();

        // a mediasource has been unlocked successfully
        // => refresh favourites due to possible visibility changes
        CServiceBroker::GetFavouritesService().RefreshFavourites();
        break;
      }
    case 1:
      {
        // password entry failed
        if (0 != CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                     CSettings::SETTING_MASTERLOCK_MAXRETRIES))
          pItem->m_iBadPwdCount++;
        CMediaSourceSettings::GetInstance().UpdateSource(strType, strLabel, "badpwdcount",
                                                         std::to_string(pItem->m_iBadPwdCount));
        CMediaSourceSettings::GetInstance().Save();
        break;
      }
    default:
      {
        // this should never happen, but if it does, do nothing
        return false;
        break;
      }
    }
  }
  return true;
}

bool CGUIPassword::IsItemUnlocked(CFileItem* pItem, const std::string& strType)
{
  const std::string strLabel = pItem->GetLabel();
  std::string strHeading;
  if (pItem->m_bIsFolder)
    strHeading = g_localizeStrings.Get(12325); // "Locked! Enter code..."
  else
    strHeading = g_localizeStrings.Get(12348); // "Item locked"

  return IsItemUnlocked<CFileItem*>(pItem, strType, strLabel, strHeading);
}

bool CGUIPassword::IsItemUnlocked(CMediaSource* pItem, const std::string& strType)
{
  const std::string strLabel = pItem->strName;
  const std::string& strHeading = g_localizeStrings.Get(12325); // "Locked! Enter code..."

  return IsItemUnlocked<CMediaSource*>(pItem, strType, strLabel, strHeading);
}

bool CGUIPassword::CheckStartUpLock()
{
  // prompt user for mastercode if the mastercode was set b4 or by xml
  int iVerifyPasswordResult = -1;

  const std::string& strHeader = g_localizeStrings.Get(20075); // "Enter master lock code"

  if (iMasterLockRetriesLeft == -1)
    iMasterLockRetriesLeft = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_MASTERLOCK_MAXRETRIES);

  if (g_passwordManager.iMasterLockRetriesLeft == 0)
    g_passwordManager.iMasterLockRetriesLeft = 1;

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  std::string strPassword = profileManager->GetMasterProfile().getLockCode();

  if (profileManager->GetMasterProfile().getLockMode() == 0)
    iVerifyPasswordResult = 0;
  else
  {
    for (int i=1; i <= g_passwordManager.iMasterLockRetriesLeft; i++)
    {
      iVerifyPasswordResult = VerifyPassword(profileManager->GetMasterProfile().getLockMode(), strPassword, strHeader);
      if (iVerifyPasswordResult != 0 )
      {
        std::string strLabel1;
        strLabel1 = g_localizeStrings.Get(12343); // "retries left"
        int iLeft = g_passwordManager.iMasterLockRetriesLeft-i;
        std::string strLabel = StringUtils::Format("{} {}", iLeft, strLabel1);

        // PopUp OK and Display: MasterLock mode has changed but no new Mastercode has been set!
        HELPERS::ShowOKDialogLines(CVariant{12360}, CVariant{12367}, CVariant{strLabel}, CVariant{""});
      }
      else
        i=g_passwordManager.iMasterLockRetriesLeft;
    }
  }

  if (iVerifyPasswordResult == 0)
  {
    g_passwordManager.iMasterLockRetriesLeft = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_MASTERLOCK_MAXRETRIES);
    return true;  // OK The MasterCode Accepted! XBMC Can Run!
  }
  else
  {
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SHUTDOWN); // Turn off the box
    return false;
  }
}

bool CGUIPassword::SetMasterLockMode(bool bDetails)
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  CProfile* profile = profileManager->GetProfile(0);
  if (profile)
  {
    CProfile::CLock locks = profile->GetLocks();
    // prompt user for master lock
    if (CGUIDialogLockSettings::ShowAndGetLock(locks, 12360, true, bDetails))
    {
      profile->SetLocks(locks);
      return true;
    }
  }
  return false;
}

bool CGUIPassword::IsProfileLockUnlocked(int iProfile)
{
  bool bDummy;
  return IsProfileLockUnlocked(iProfile,bDummy,true);
}

bool CGUIPassword::IsProfileLockUnlocked(int iProfile, bool& bCanceled, bool prompt)
{
  if (g_passwordManager.bMasterUser)
    return true;

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  int iProfileToCheck = iProfile;
  if (iProfile == -1)
    iProfileToCheck = profileManager->GetCurrentProfileIndex();

  if (iProfileToCheck == 0)
    return IsMasterLockUnlocked(prompt,bCanceled);
  else
  {
    const CProfile *profile = profileManager->GetProfile(iProfileToCheck);
    if (!profile)
      return false;

    if (!prompt)
      return (profile->getLockMode() == LOCK_MODE_EVERYONE);

    if (profile->getDate().empty() &&
       (profileManager->GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE ||
        profile->getLockMode() == LOCK_MODE_EVERYONE))
    {
      // user hasn't set a password and this is the first time they've used this account
      // so prompt for password/settings
      if (CGUIDialogProfileSettings::ShowForProfile(iProfileToCheck, true))
        return true;
    }
    else
    {
      if (profileManager->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
        // prompt user for profile lock code
        return CheckLock(profile->getLockMode(),profile->getLockCode(),20095,bCanceled);
    }
  }

  return true;
}

bool CGUIPassword::IsMasterLockUnlocked(bool bPromptUser)
{
  bool bDummy;
  return IsMasterLockUnlocked(bPromptUser,bDummy);
}

bool CGUIPassword::IsMasterLockUnlocked(bool bPromptUser, bool& bCanceled)
{
  bCanceled = false;

  if (iMasterLockRetriesLeft == -1)
    iMasterLockRetriesLeft = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_MASTERLOCK_MAXRETRIES);

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  if ((LOCK_MODE_EVERYONE < profileManager->GetMasterProfile().getLockMode() && !bMasterUser) && !bPromptUser)
  {
    // not unlocked, but calling code doesn't want to prompt user
    return false;
  }

  if (g_passwordManager.bMasterUser || profileManager->GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE)
    return true;

  if (iMasterLockRetriesLeft == 0)
  {
    UpdateMasterLockRetryCount(false);
    return false;
  }

  // no, unlock since we are allowed to prompt
  const std::string& strHeading = g_localizeStrings.Get(20075);
  std::string strPassword = profileManager->GetMasterProfile().getLockCode();

  int iVerifyPasswordResult = VerifyPassword(profileManager->GetMasterProfile().getLockMode(), strPassword, strHeading);
  if (1 == iVerifyPasswordResult)
    UpdateMasterLockRetryCount(false);

  if (0 != iVerifyPasswordResult)
  {
    bCanceled = true;
    return false;
  }

  // user successfully entered mastercode
  UpdateMasterLockRetryCount(true);
  return true;
}

void CGUIPassword::UpdateMasterLockRetryCount(bool bResetCount)
{
  // \brief Updates Master Lock status.
  // \param bResetCount masterlock retry counter is zeroed if true, or incremented and displays an Access Denied dialog if false.
  if (!bResetCount)
  {
    // Bad mastercode entered
    if (0 < CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_MASTERLOCK_MAXRETRIES))
    {
      // We're keeping track of how many bad passwords are entered
      if (1 < g_passwordManager.iMasterLockRetriesLeft)
      {
        // user still has at least one retry after decrementing
        g_passwordManager.iMasterLockRetriesLeft--;
      }
      else
      {
        // user has run out of retry attempts
        g_passwordManager.iMasterLockRetriesLeft = 0;
        // Tell the user they ran out of retry attempts
        HELPERS::ShowOKDialogText(CVariant{12345}, CVariant{12346});
        return;
      }
    }
    std::string dlgLine1 = "";
    if (0 < g_passwordManager.iMasterLockRetriesLeft)
      dlgLine1 = StringUtils::Format("{} {}", g_passwordManager.iMasterLockRetriesLeft,
                                     g_localizeStrings.Get(12343)); // "retries left"
    // prompt user for master lock code
    HELPERS::ShowOKDialogLines(CVariant{20075}, CVariant{12345}, CVariant{std::move(dlgLine1)}, CVariant{0});
  }
  else
    g_passwordManager.iMasterLockRetriesLeft = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_MASTERLOCK_MAXRETRIES); // user entered correct mastercode, reset retries to max allowed
}

bool CGUIPassword::CheckLock(LockType btnType, const std::string& strPassword, int iHeading)
{
  bool bDummy;
  return CheckLock(btnType,strPassword,iHeading,bDummy);
}

bool CGUIPassword::CheckLock(LockType btnType, const std::string& strPassword, int iHeading, bool& bCanceled)
{
  bCanceled = false;

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  if (btnType == LOCK_MODE_EVERYONE ||
      strPassword == "-" ||
      profileManager->GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE ||
      g_passwordManager.bMasterUser)
  {
    return true;
  }

  const std::string& strHeading = g_localizeStrings.Get(iHeading);
  int iVerifyPasswordResult = VerifyPassword(btnType, strPassword, strHeading);

  if (iVerifyPasswordResult == -1)
    bCanceled = true;

  return (iVerifyPasswordResult==0);
}

bool CGUIPassword::CheckSettingLevelLock(const SettingLevel& level, bool enforce /*=false*/)
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  LOCK_LEVEL::SETTINGS_LOCK lockLevel = profileManager->GetCurrentProfile().settingsLockLevel();

  if (lockLevel == LOCK_LEVEL::NONE)
    return true;

    //check if we are already in settings and in an level that needs unlocking
  int windowID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if ((int)lockLevel-1 <= (short)CViewStateSettings::GetInstance().GetSettingLevel() &&
     (windowID == WINDOW_SETTINGS_MENU ||
         (windowID >= WINDOW_SCREEN_CALIBRATION &&
          windowID <= WINDOW_SETTINGS_MYPVR)))
    return true; //Already unlocked

  else if (lockLevel == LOCK_LEVEL::ALL)
    return IsMasterLockUnlocked(true);
  else if ((int)lockLevel-1 <= (short)level)
  {
    if (enforce)
      return IsMasterLockUnlocked(true);
    else if (!IsMasterLockUnlocked(false))
    {
      //Current Setting level is higher than our permission... so lower the viewing level
      SettingLevel newLevel = (SettingLevel)(short)(lockLevel-2);
      CViewStateSettings::GetInstance().SetSettingLevel(newLevel);
    }
  }
  return true;

}

bool IsSettingsWindow(int iWindowID)
{
  return (iWindowID >= WINDOW_SCREEN_CALIBRATION && iWindowID <= WINDOW_SETTINGS_MYPVR)
       || iWindowID == WINDOW_SKIN_SETTINGS;
}

bool CGUIPassword::CheckMenuLock(int iWindowID)
{
  bool bCheckPW = false;
  int iSwitch = iWindowID;

  // check if a settings subcategory was called from other than settings window
  if (IsSettingsWindow(iWindowID))
  {
    int iCWindowID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
    if (iCWindowID != WINDOW_SETTINGS_MENU && !IsSettingsWindow(iCWindowID))
      iSwitch = WINDOW_SETTINGS_MENU;
  }

  if (iWindowID == WINDOW_MUSIC_NAV)
  {
    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_HOME)
      iSwitch = WINDOW_MUSIC_NAV;
  }

  if (iWindowID == WINDOW_VIDEO_NAV)
  {
    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_HOME)
      iSwitch = WINDOW_VIDEO_NAV;
  }

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  switch (iSwitch)
  {
    case WINDOW_SETTINGS_MENU:  // Settings
      return CheckSettingLevelLock(CViewStateSettings::GetInstance().GetSettingLevel());
      break;
    case WINDOW_ADDON_BROWSER:  // Addons
      bCheckPW = profileManager->GetCurrentProfile().addonmanagerLocked();
      break;
    case WINDOW_FILES:          // Files
      bCheckPW = profileManager->GetCurrentProfile().filesLocked();
      break;
    case WINDOW_PROGRAMS:       // Programs
      bCheckPW = profileManager->GetCurrentProfile().programsLocked();
      break;
    case WINDOW_MUSIC_NAV:      // Music
      bCheckPW = profileManager->GetCurrentProfile().musicLocked();
      if (!bCheckPW && !m_strMediaSourcePath.empty()) // check mediasource by path
        return g_passwordManager.IsMediaPathUnlocked(profileManager, "music");
      break;
    case WINDOW_VIDEO_NAV:      // Video
      bCheckPW = profileManager->GetCurrentProfile().videoLocked();
      if (!bCheckPW && !m_strMediaSourcePath.empty()) // check mediasource by path
        return g_passwordManager.IsMediaPathUnlocked(profileManager, "video");
      break;
    case WINDOW_PICTURES:       // Pictures
      bCheckPW = profileManager->GetCurrentProfile().picturesLocked();
      break;
    case WINDOW_GAMES:          // Games
      bCheckPW = profileManager->GetCurrentProfile().gamesLocked();
      break;
    case WINDOW_SETTINGS_PROFILES:
      bCheckPW = true;
      break;
    default:
      bCheckPW = false;
      break;
  }
  if (bCheckPW)
    return IsMasterLockUnlocked(true); //Now let's check the PW if we need!
  else
    return true;
}

bool CGUIPassword::LockSource(const std::string& strType, const std::string& strName, bool bState)
{
  VECSOURCES* pShares = CMediaSourceSettings::GetInstance().GetSources(strType);
  bool bResult = false;
  for (IVECSOURCES it=pShares->begin();it != pShares->end();++it)
  {
    if (it->strName == strName)
    {
      if (it->m_iHasLock > LOCK_STATE_NO_LOCK)
      {
        it->m_iHasLock = bState ? LOCK_STATE_LOCKED : LOCK_STATE_LOCK_BUT_UNLOCKED;
        bResult = true;
      }
      break;
    }
  }
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);

  return bResult;
}

void CGUIPassword::LockSources(bool lock)
{
  // lock or unlock all sources (those with locks)
  const char* strTypes[] = {"programs", "music", "video", "pictures", "files", "games"};
  for (const char* const strType : strTypes)
  {
    VECSOURCES *shares = CMediaSourceSettings::GetInstance().GetSources(strType);
    for (IVECSOURCES it=shares->begin();it != shares->end();++it)
      if (it->m_iLockMode != LOCK_MODE_EVERYONE)
        it->m_iHasLock = lock ? LOCK_STATE_LOCKED : LOCK_STATE_LOCK_BUT_UNLOCKED;
  }
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CGUIPassword::RemoveSourceLocks()
{
  // remove lock from all sources
  const char* strTypes[] = {"programs", "music", "video", "pictures", "files", "games"};
  for (const char* const strType : strTypes)
  {
    VECSOURCES *shares = CMediaSourceSettings::GetInstance().GetSources(strType);
    for (IVECSOURCES it=shares->begin();it != shares->end();++it)
      if (it->m_iLockMode != LOCK_MODE_EVERYONE) // remove old info
      {
        it->m_iHasLock = LOCK_STATE_NO_LOCK;
        it->m_iLockMode = LOCK_MODE_EVERYONE;

        // remove locks from xml
        CMediaSourceSettings::GetInstance().UpdateSource(strType, it->strName, "lockmode", "0");
      }
  }
  CMediaSourceSettings::GetInstance().Save();
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0, GUI_MSG_UPDATE_SOURCES);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

bool CGUIPassword::IsDatabasePathUnlocked(const std::string& strPath, VECSOURCES& vecSources)
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  if (g_passwordManager.bMasterUser || profileManager->GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE)
    return true;

  // Don't check plugin paths as they don't have an entry in sources.xml
  // Base locked status on videos being locked and being in the video
  // navigation screen (must have passed lock by then)
  CURL url{strPath};
  if (url.IsProtocol("plugin"))
    return !(profileManager->GetCurrentProfile().videoLocked() &&
             CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_VIDEO_NAV);

  // try to find the best matching source
  bool bName = false;
  int iIndex = CUtil::GetMatchingSource(strPath, vecSources, bName);

  if (iIndex > -1 && iIndex < static_cast<int>(vecSources.size()))
    if (vecSources[iIndex].m_iHasLock < LOCK_STATE_LOCKED)
      return true;

  return false;
}

bool CGUIPassword::IsMediaPathUnlocked(const std::shared_ptr<CProfileManager>& profileManager,
                                       const std::string& strType) const
{
  if (!StringUtils::StartsWithNoCase(m_strMediaSourcePath, "root") &&
      !StringUtils::StartsWithNoCase(m_strMediaSourcePath, "library://"))
  {
    if (!g_passwordManager.bMasterUser &&
        profileManager->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
    {
      VECSOURCES& vecSources = *CMediaSourceSettings::GetInstance().GetSources(strType);
      bool bName = false;
      int iIndex = CUtil::GetMatchingSource(m_strMediaSourcePath, vecSources, bName);
      if (iIndex > -1 && iIndex < static_cast<int>(vecSources.size()))
      {
        return g_passwordManager.IsItemUnlocked(&vecSources[iIndex], strType);
      }
    }
  }

  return true;
}

bool CGUIPassword::IsMediaFileUnlocked(const std::string& type, const std::string& file) const
{
  std::vector<CMediaSource>* vecSources = CMediaSourceSettings::GetInstance().GetSources(type);

  if (!vecSources)
  {
    CLog::Log(LOGERROR,
              "{}: CMediaSourceSettings::GetInstance().GetSources(\"{}\") returned nullptr.",
              __func__, type);
    return true;
  }

  // try to find the best matching source for this file

  bool isSourceName{false};
  const std::string fileBasePath = URIUtils::GetBasePath(file);

  int iIndex = CUtil::GetMatchingSource(fileBasePath, *vecSources, isSourceName);

  if (iIndex > -1 && iIndex < static_cast<int>(vecSources->size()))
    return (*vecSources)[iIndex].m_iHasLock < LOCK_STATE_LOCKED;

  return true;
}

void CGUIPassword::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_MASTERLOCK_LOCKCODE)
    SetMasterLockMode();
}

int CGUIPassword::VerifyPassword(LockType btnType, const std::string& strPassword, const std::string& strHeading)
{
  int iVerifyPasswordResult;
  switch (btnType)
  {
  case LOCK_MODE_NUMERIC:
    iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(const_cast<std::string&>(strPassword), strHeading, 0);
    break;
  case LOCK_MODE_GAMEPAD:
    iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(const_cast<std::string&>(strPassword), strHeading, 0);
    break;
  case LOCK_MODE_QWERTY:
    iVerifyPasswordResult = CGUIKeyboardFactory::ShowAndVerifyPassword(const_cast<std::string&>(strPassword), strHeading, 0);
    break;
  default:   // must not be supported, treat as unlocked
    iVerifyPasswordResult = 0;
    break;
  }

  return iVerifyPasswordResult;
}

