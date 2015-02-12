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

#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "ApplicationMessenger.h"
#include "dialogs/GUIDialogGamepad.h"
#include "guilib/GUIKeyboardFactory.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogOK.h"
#include "profiles/ProfilesManager.h"
#include "profiles/dialogs/GUIDialogLockSettings.h"
#include "profiles/dialogs/GUIDialogProfileSettings.h"
#include "Util.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "guilib/GUIWindowManager.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "view/ViewStateSettings.h"

CGUIPassword::CGUIPassword(void)
{
  iMasterLockRetriesLeft = -1;
  bMasterUser = false;
}
CGUIPassword::~CGUIPassword(void)
{}

bool CGUIPassword::IsItemUnlocked(CFileItem* pItem, const std::string &strType)
{
  // \brief Tests if the user is allowed to access the share folder
  // \param pItem The share folder item to access
  // \param strType The type of share being accessed, e.g. "music", "video", etc. See CSettings::UpdateSources()
  // \return If access is granted, returns \e true
  if (CProfilesManager::Get().GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE)
    return true;

  while (pItem->m_iHasLock > 1)
  {
    std::string strLockCode = pItem->m_strLockCode;
    std::string strLabel = pItem->GetLabel();
    int iResult = 0;  // init to user succeeded state, doing this to optimize switch statement below
    char buffer[33]; // holds 32 places plus sign character
    if(g_passwordManager.bMasterUser)// Check if we are the MasterUser!
    {
      iResult = 0;
    }
    else
    {
      if (0 != CSettings::Get().GetInt("masterlock.maxretries") && pItem->m_iBadPwdCount >= CSettings::Get().GetInt("masterlock.maxretries"))
      { // user previously exhausted all retries, show access denied error
        CGUIDialogOK::ShowAndGetInput(12345, 12346, 0, 0);
        return false;
      }
      // show the appropriate lock dialog
      std::string strHeading = "";
      if (pItem->m_bIsFolder)
        strHeading = g_localizeStrings.Get(12325);
      else
        strHeading = g_localizeStrings.Get(12348);

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
        pItem->m_iHasLock = 1;
        g_passwordManager.LockSource(strType,strLabel,false);
        sprintf(buffer,"%i",pItem->m_iBadPwdCount);
        CMediaSourceSettings::Get().UpdateSource(strType, strLabel, "badpwdcount", buffer);
        CMediaSourceSettings::Get().Save();
        break;
      }
    case 1:
      {
        // password entry failed
        if (0 != CSettings::Get().GetInt("masterlock.maxretries"))
          pItem->m_iBadPwdCount++;
        sprintf(buffer,"%i",pItem->m_iBadPwdCount);
        CMediaSourceSettings::Get().UpdateSource(strType, strLabel, "badpwdcount", buffer);
        CMediaSourceSettings::Get().Save();
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

bool CGUIPassword::CheckStartUpLock()
{
  // prompt user for mastercode if the mastercode was set b4 or by xml
  int iVerifyPasswordResult = -1;
  std::string strHeader = g_localizeStrings.Get(20075);
  if (iMasterLockRetriesLeft == -1)
    iMasterLockRetriesLeft = CSettings::Get().GetInt("masterlock.maxretries");
  if (g_passwordManager.iMasterLockRetriesLeft == 0) g_passwordManager.iMasterLockRetriesLeft = 1;
  std::string strPassword = CProfilesManager::Get().GetMasterProfile().getLockCode();
  if (CProfilesManager::Get().GetMasterProfile().getLockMode() == 0)
    iVerifyPasswordResult = 0;
  else
  {
    for (int i=1; i <= g_passwordManager.iMasterLockRetriesLeft; i++)
    {
      iVerifyPasswordResult = VerifyPassword(CProfilesManager::Get().GetMasterProfile().getLockMode(), strPassword, strHeader);
      if (iVerifyPasswordResult != 0 )
      {
        std::string strLabel1;
        strLabel1 = g_localizeStrings.Get(12343);
        int iLeft = g_passwordManager.iMasterLockRetriesLeft-i;
        std::string strLabel = StringUtils::Format("%i %s", iLeft, strLabel1.c_str());

        // PopUp OK and Display: MasterLock mode has changed but no new Mastercode has been set!
        CGUIDialogOK::ShowAndGetInput(12360, 12367, 12368, strLabel);
      }
      else
        i=g_passwordManager.iMasterLockRetriesLeft;
    }
  }

  if (iVerifyPasswordResult == 0)
  {
    g_passwordManager.iMasterLockRetriesLeft = CSettings::Get().GetInt("masterlock.maxretries");
    return true;  // OK The MasterCode Accepted! XBMC Can Run!
  }
  else
  {
    CApplicationMessenger::Get().Shutdown(); // Turn off the box
    return false;
  }
}

bool CGUIPassword::SetMasterLockMode(bool bDetails)
{
  CProfile* profile = CProfilesManager::Get().GetProfile(0);
  if (profile)
  {
    CProfile::CLock locks = profile->GetLocks();
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
  int iProfileToCheck=iProfile;
  if (iProfile == -1)
    iProfileToCheck = CProfilesManager::Get().GetCurrentProfileIndex();
  if (iProfileToCheck == 0)
    return IsMasterLockUnlocked(prompt,bCanceled);
  else
  {
    CProfile *profile = CProfilesManager::Get().GetProfile(iProfileToCheck);
    if (!profile)
      return false;

    if (!prompt)
      return (profile->getLockMode() == LOCK_MODE_EVERYONE);

    if (profile->getDate().empty() &&
       (CProfilesManager::Get().GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE ||
        profile->getLockMode() == LOCK_MODE_EVERYONE))
    {
      // user hasn't set a password and this is the first time they've used this account
      // so prompt for password/settings
      if (CGUIDialogProfileSettings::ShowForProfile(iProfileToCheck, true))
        return true;
    }
    else
       if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
        return CheckLock(profile->getLockMode(),profile->getLockCode(),20095,bCanceled);
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
    iMasterLockRetriesLeft = CSettings::Get().GetInt("masterlock.maxretries");
  if ((LOCK_MODE_EVERYONE < CProfilesManager::Get().GetMasterProfile().getLockMode() && !bMasterUser) && !bPromptUser)
    // not unlocked, but calling code doesn't want to prompt user
    return false;

  if (g_passwordManager.bMasterUser || CProfilesManager::Get().GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE)
    return true;

  if (iMasterLockRetriesLeft == 0)
  {
    UpdateMasterLockRetryCount(false);
    return false;
  }

  // no, unlock since we are allowed to prompt
  int iVerifyPasswordResult = -1;
  std::string strHeading = g_localizeStrings.Get(20075);
  std::string strPassword = CProfilesManager::Get().GetMasterProfile().getLockCode();
  iVerifyPasswordResult = VerifyPassword(CProfilesManager::Get().GetMasterProfile().getLockMode(), strPassword, strHeading);
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
    if (0 < CSettings::Get().GetInt("masterlock.maxretries"))
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
        CGUIDialogOK::ShowAndGetInput(12345, 12346, 0, 0);
        return ;
      }
    }
    std::string dlgLine1 = "";
    if (0 < g_passwordManager.iMasterLockRetriesLeft)
      dlgLine1 = StringUtils::Format("%d %s",
                                     g_passwordManager.iMasterLockRetriesLeft,
                                     g_localizeStrings.Get(12343).c_str());
    CGUIDialogOK::ShowAndGetInput(20075, 12345, dlgLine1, 0);
  }
  else
    g_passwordManager.iMasterLockRetriesLeft = CSettings::Get().GetInt("masterlock.maxretries"); // user entered correct mastercode, reset retries to max allowed
}

bool CGUIPassword::CheckLock(LockType btnType, const std::string& strPassword, int iHeading)
{
  bool bDummy;
  return CheckLock(btnType,strPassword,iHeading,bDummy);
}

bool CGUIPassword::CheckLock(LockType btnType, const std::string& strPassword, int iHeading, bool& bCanceled)
{
  bCanceled = false;
  if (btnType == LOCK_MODE_EVERYONE || strPassword == "-"        ||
      CProfilesManager::Get().GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser)
    return true;

  int iVerifyPasswordResult = -1;
  std::string strHeading = g_localizeStrings.Get(iHeading);
  iVerifyPasswordResult = VerifyPassword(btnType, strPassword, strHeading);

  if (iVerifyPasswordResult == -1)
    bCanceled = true;

  return (iVerifyPasswordResult==0);
}

bool CGUIPassword::CheckSettingLevelLock(const SettingLevel& level, bool enforce /*=false*/)
{
  LOCK_LEVEL::SETTINGS_LOCK lockLevel = CProfilesManager::Get().GetCurrentProfile().settingsLockLevel();
  
  if (lockLevel == LOCK_LEVEL::NONE)
    return true;
  
    //check if we are already in settings and in an level that needs unlocking
  int windowID = g_windowManager.GetActiveWindow();
  if ((int)lockLevel-1 <= (short)CViewStateSettings::Get().GetSettingLevel() && 
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
      CViewStateSettings::Get().SetSettingLevel(newLevel);
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
  bool bCheckPW         = false;
  int iSwitch = iWindowID;

  // check if a settings subcategory was called from other than settings window
  if (IsSettingsWindow(iWindowID))
  {
    int iCWindowID = g_windowManager.GetActiveWindow();
    if (iCWindowID != WINDOW_SETTINGS_MENU && !IsSettingsWindow(iCWindowID))
      iSwitch = WINDOW_SETTINGS_MENU;
  }

  if (iWindowID == WINDOW_MUSIC_FILES)
    if (g_windowManager.GetActiveWindow() == WINDOW_MUSIC_NAV)
      iSwitch = WINDOW_HOME;

  if (iWindowID == WINDOW_MUSIC_NAV)
    if (g_windowManager.GetActiveWindow() == WINDOW_HOME)
      iSwitch = WINDOW_MUSIC_FILES;

  if (iWindowID == WINDOW_VIDEO_NAV)
    if (g_windowManager.GetActiveWindow() == WINDOW_HOME)
      iSwitch = WINDOW_VIDEO_FILES;

  if (iWindowID == WINDOW_VIDEO_FILES)
    if (g_windowManager.GetActiveWindow() == WINDOW_VIDEO_NAV)
      iSwitch = WINDOW_HOME;

  switch (iSwitch)
  {
    case WINDOW_SETTINGS_MENU:  // Settings
      return CheckSettingLevelLock(CViewStateSettings::Get().GetSettingLevel());
      break;
    case WINDOW_ADDON_BROWSER:  // Addons
      bCheckPW = CProfilesManager::Get().GetCurrentProfile().addonmanagerLocked();
      break;
    case WINDOW_FILES:          // Files
      bCheckPW = CProfilesManager::Get().GetCurrentProfile().filesLocked();
      break;
    case WINDOW_PROGRAMS:       // Programs
      bCheckPW = CProfilesManager::Get().GetCurrentProfile().programsLocked();
      break;
    case WINDOW_MUSIC_FILES:    // Music
      bCheckPW = CProfilesManager::Get().GetCurrentProfile().musicLocked();
      break;
    case WINDOW_VIDEO_FILES:    // Video
      bCheckPW = CProfilesManager::Get().GetCurrentProfile().videoLocked();
      break;
    case WINDOW_PICTURES:       // Pictures
      bCheckPW = CProfilesManager::Get().GetCurrentProfile().picturesLocked();
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
  VECSOURCES* pShares = CMediaSourceSettings::Get().GetSources(strType);
  bool bResult = false;
  for (IVECSOURCES it=pShares->begin();it != pShares->end();++it)
  {
    if (it->strName == strName)
    {
      if (it->m_iHasLock > 0)
      {
        it->m_iHasLock = bState?2:1;
        bResult = true;
      }
      break;
    }
  }
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
  g_windowManager.SendThreadMessage(msg);

  return bResult;
}

void CGUIPassword::LockSources(bool lock)
{
  // lock or unlock all sources (those with locks)
  const char* strType[5] = {"programs","music","video","pictures","files"};
  for (int i=0;i<5;++i)
  {
    VECSOURCES *shares = CMediaSourceSettings::Get().GetSources(strType[i]);
    for (IVECSOURCES it=shares->begin();it != shares->end();++it)
      if (it->m_iLockMode != LOCK_MODE_EVERYONE)
        it->m_iHasLock = lock ? 2 : 1;
  }
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
  g_windowManager.SendThreadMessage(msg);
}

void CGUIPassword::RemoveSourceLocks()
{
  // remove lock from all sources
  const char* strType[5] = {"programs","music","video","pictures","files"};
  for (int i=0;i<5;++i)
  {
    VECSOURCES *shares = CMediaSourceSettings::Get().GetSources(strType[i]);
    for (IVECSOURCES it=shares->begin();it != shares->end();++it)
      if (it->m_iLockMode != LOCK_MODE_EVERYONE) // remove old info
      {
        it->m_iHasLock = 0;
        it->m_iLockMode = LOCK_MODE_EVERYONE;
        CMediaSourceSettings::Get().UpdateSource(strType[i], it->strName, "lockmode", "0"); // removes locks from xml
      }
  }
  CMediaSourceSettings::Get().Save();
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0, GUI_MSG_UPDATE_SOURCES);
  g_windowManager.SendThreadMessage(msg);
}

bool CGUIPassword::IsDatabasePathUnlocked(const std::string& strPath, VECSOURCES& vecSources)
{
  if (g_passwordManager.bMasterUser || CProfilesManager::Get().GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE)
    return true;

  // try to find the best matching source
  bool bName = false;
  int iIndex = CUtil::GetMatchingSource(strPath, vecSources, bName);

  if (iIndex > -1 && iIndex < (int)vecSources.size())
    if (vecSources[iIndex].m_iHasLock < 2)
      return true;

  return false;
}

void CGUIPassword::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "masterlock.lockcode")
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

