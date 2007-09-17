/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIPassword.h"
#include "Application.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogGamepad.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogLockSettings.h"
#include "GUIDialogProfileSettings.h"
#include "util.h"
#include "settings.h"

CGUIPassword g_passwordManager;

CGUIPassword::CGUIPassword(void)
{
  iMasterLockRetriesLeft = -1;
  bMasterUser = false;
  m_SMBShare = "";
}
CGUIPassword::~CGUIPassword(void)
{}

bool CGUIPassword::IsItemUnlocked(CFileItem* pItem, const CStdString &strType)
{
  // \brief Tests if the user is allowed to access the share folder
  // \param pItem The share folder item to access
  // \param strType The type of share being accessed, e.g. "music", "video", etc. See CSettings::UpdateSources()
  // \return If access is granted, returns \e true
  if (g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE)
    return true;

  while (pItem->m_iHasLock > 1)
  {
    int iMode = pItem->m_iLockMode;
    CStdString strLockCode = pItem->m_strLockCode;
    CStdString strLabel = pItem->GetLabel();
    bool bConfirmed = false;
    bool bCanceled = false;
    int iResult = 0;  // init to user succeeded state, doing this to optimize switch statement below
    char buffer[33]; // holds 32 places plus sign character
    int iRetries = 0;
    if(g_passwordManager.bMasterUser)// Check if we are the MasterUser!
    {
      iResult = 0;
    }
    else
    {
      if (!(1 > pItem->m_iBadPwdCount))
        iRetries = g_guiSettings.GetInt("masterlock.maxretries") - pItem->m_iBadPwdCount;
      if (0 != g_guiSettings.GetInt("masterlock.maxretries") && pItem->m_iBadPwdCount >= g_guiSettings.GetInt("masterlock.maxretries"))
      { // user previously exhausted all retries, show access denied error
        CGUIDialogOK::ShowAndGetInput(12345, 12346, 0, 0);
        return false;
      }
      // show the appropriate lock dialog
      CStdString strHeading = "";
      if (pItem->m_bIsFolder)
        strHeading = g_localizeStrings.Get(12325);
      else
        strHeading = g_localizeStrings.Get(12348);

      switch (iMode)
      {
      case LOCK_MODE_NUMERIC:
        iResult = CGUIDialogNumeric::ShowAndVerifyPassword(strLockCode, strHeading, iRetries);
        break;
      case LOCK_MODE_GAMEPAD:
        iResult = CGUIDialogGamepad::ShowAndVerifyPassword(strLockCode, strHeading, iRetries);
        break;
      case LOCK_MODE_QWERTY:
        iResult = CGUIDialogKeyboard::ShowAndVerifyPassword(strLockCode, strHeading, iRetries);
        break;
      default:
        // pItem->m_iLockMode isn't set to an implemented lock mode, so treat as unlocked
        return true;
        break;
      }
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
        g_settings.UpdateSource(strType, strLabel, "badpwdcount", itoa(pItem->m_iBadPwdCount, buffer, 10));
        g_settings.SaveSources();
        break;
      }
    case 1:
      {
        // password entry failed
        if (0 != g_guiSettings.GetInt("masterlock.maxretries"))
          pItem->m_iBadPwdCount++;
        g_settings.UpdateSource(strType, strLabel, "badpwdcount", itoa(pItem->m_iBadPwdCount, buffer, 10));
        g_settings.SaveSources();
        break;
      }
    default:
      {
        // this should never happen, but if it does, do nothing
        return false; break;
      }
    }
  }
  return true;
}

void CGUIPassword::SetSMBShare(const CStdString &strPath)
{
  m_SMBShare = strPath;
}

CStdString CGUIPassword::GetSMBShare()
{
  return m_SMBShare;
}

bool CGUIPassword::GetSMBShareUserPassword()
{
  CURL url(m_SMBShare);
  CStdString passcode;
  CStdString username = url.GetUserName();
  CStdString outusername = username;
  CStdString share;
  url.GetURLWithoutUserDetails(share);

  if (!CGUIDialogLockSettings::ShowAndGetUserAndPassword(outusername,passcode,share))
    return false;

  url.SetPassword(passcode);
  url.SetUserName(outusername);
  url.GetURL(m_SMBShare);

  return true;
}

bool CGUIPassword::CheckStartUpLock()
{
  // prompt user for mastercode if the mastercode was set b4 or by xml
  int iVerifyPasswordResult = -1;
  CStdString strHeader = g_localizeStrings.Get(20075);
  if (iMasterLockRetriesLeft == -1)
    iMasterLockRetriesLeft = g_guiSettings.GetInt("masterlock.maxretries");
  if (g_passwordManager.iMasterLockRetriesLeft == 0) g_passwordManager.iMasterLockRetriesLeft = 1;
  CStdString strPassword = g_settings.m_vecProfiles[0].getLockCode();
  for (int i=1; i <= g_passwordManager.iMasterLockRetriesLeft; i++)
  {
    switch (g_settings.m_vecProfiles[0].getLockMode())
    { // Prompt user for mastercode
      case LOCK_MODE_NUMERIC:
        iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(strPassword, strHeader, 0);
        break;
      case LOCK_MODE_GAMEPAD:
        iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(strPassword, strHeader, 0);
        break;
      case LOCK_MODE_QWERTY:
        iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword(strPassword, strHeader, 0);
        break;
    }
    if (iVerifyPasswordResult != 0 )
    {
      CStdString strLabel,strLabel1;
      strLabel1 = g_localizeStrings.Get(12343);
      int iLeft = g_passwordManager.iMasterLockRetriesLeft-i;
      strLabel.Format("%i %s",iLeft,strLabel1.c_str());

      // PopUp OK and Display: MasterLock mode has changed but no no Mastercode has been set!
      CGUIDialogOK *dlg = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (!dlg) return false;
      dlg->SetHeading( g_localizeStrings.Get(20076) );
      dlg->SetLine( 0, g_localizeStrings.Get(12367) );
      dlg->SetLine( 1, g_localizeStrings.Get(12368) );
      dlg->SetLine( 2, strLabel);
      dlg->DoModal();
    }
    else
      i=g_passwordManager.iMasterLockRetriesLeft;
  }

  if (iVerifyPasswordResult == 0)
  {
    g_passwordManager.iMasterLockRetriesLeft = g_guiSettings.GetInt("masterlock.maxretries");
    return true;  // OK The MasterCode Accepted! XBMC Can Run!
  }
  else
  {
    g_applicationMessenger.Shutdown(); // Turn off the box
    return false;
  }
}

bool CGUIPassword::SetMasterLockMode(bool bDetails)
{
  CGUIDialogLockSettings* pDialog = (CGUIDialogLockSettings*)m_gWindowManager.GetWindow(WINDOW_DIALOG_LOCK_SETTINGS);
  if (pDialog)
  {
    CProfile& profile=g_settings.m_vecProfiles.at(0);
    if (pDialog->ShowAndGetLock(profile._iLockMode,profile._strLockCode,profile._bLockMusic,profile._bLockVideo,profile._bLockPictures,profile._bLockPrograms,profile._bLockFiles,profile._bLockSettings,12360,true,bDetails))
      return true;

    return false;
  }

  return true;
}

bool CGUIPassword::IsProfileLockUnlocked(int iProfile)
{
  bool bDummy;
  return IsProfileLockUnlocked(iProfile,bDummy);
}

bool CGUIPassword::IsProfileLockUnlocked(int iProfile, bool& bCanceled)
{
  if (g_passwordManager.bMasterUser)
    return true;
  int iProfileToCheck=iProfile;
  if (iProfile == -1)
    iProfileToCheck = g_settings.m_iLastLoadedProfileIndex;
  if (iProfileToCheck == 0)
    return IsMasterLockUnlocked(true,bCanceled);
  else
  {
    if (g_settings.m_vecProfiles[iProfileToCheck].getDate().IsEmpty() && (g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE || g_settings.m_vecProfiles[iProfileToCheck].getLockMode() == LOCK_MODE_EVERYONE))
    {
      if (CGUIDialogProfileSettings::ShowForProfile(iProfileToCheck,false))
        return true;
    }
    else
       if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
        return CheckLock(g_settings.m_vecProfiles[iProfileToCheck].getLockMode(),g_settings.m_vecProfiles[iProfileToCheck].getLockCode(),20095,bCanceled);
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
    iMasterLockRetriesLeft = g_guiSettings.GetInt("masterlock.maxretries");
  if ((LOCK_MODE_EVERYONE < g_settings.m_vecProfiles[0].getLockMode() && !bMasterUser) && !bPromptUser)
    // not unlocked, but calling code doesn't want to prompt user
    return false;

  if (g_passwordManager.bMasterUser || g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE)
    return true;

  if (iMasterLockRetriesLeft == 0)
  {
    UpdateMasterLockRetryCount(false);
    return false;
  }

  // no, unlock since we are allowed to prompt
  int iVerifyPasswordResult = -1;
  CStdString strHeading = g_localizeStrings.Get(20075);
  CStdString strPassword = g_settings.m_vecProfiles[0].getLockCode();
  switch (g_settings.m_vecProfiles[0].getLockMode())
  {
  case LOCK_MODE_NUMERIC:
    iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(strPassword, strHeading, 0);
    break;
  case LOCK_MODE_GAMEPAD:
    iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(strPassword, strHeading, 0);
    break;
  case LOCK_MODE_QWERTY:
    iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword(strPassword, strHeading, 0);
    break;
  default:   // must not be supported, treat as unlocked
    iVerifyPasswordResult = 0;
    break;
  }
  if (1 == iVerifyPasswordResult)
    UpdateMasterLockRetryCount(false);

  if (0 != iVerifyPasswordResult)
  {
    bCanceled = true;
    return false;
  }

  // user successfully entered mastercode
  UpdateMasterLockRetryCount(true);
  if (g_guiSettings.GetBool("masterlock.automastermode") && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
  {
      LockSources(false);
      bMasterUser = true;
      g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(20052),g_localizeStrings.Get(20054));
  }
  return true;
}

void CGUIPassword::UpdateMasterLockRetryCount(bool bResetCount)
{
  // \brief Updates Master Lock status.
  // \param bResetCount masterlock retry counter is zeroed if true, or incremented and displays an Access Denied dialog if false.
  if (!bResetCount)
  {
    // Bad mastercode entered
    if (0 < g_guiSettings.GetInt("masterlock.maxretries"))
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
        if (g_guiSettings.GetBool("masterlock.enableshutdown"))
        {
          // Shutdown enabled, tell the user we're shutting off
          CGUIDialogOK::ShowAndGetInput(12345, 12346, 12347, 0);
          g_applicationMessenger.Shutdown();
          return ;
        }
        // Tell the user they ran out of retry attempts
        CGUIDialogOK::ShowAndGetInput(12345, 12346, 0, 0);
        return ;
      }
    }
    CStdString dlgLine1 = "";
    if (0 < g_passwordManager.iMasterLockRetriesLeft)
      dlgLine1.Format("%d %s", g_passwordManager.iMasterLockRetriesLeft, g_localizeStrings.Get(12343));
    CGUIDialogOK *dialog = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK); // Tell user they entered a bad password
    if (dialog)
    {
      dialog->SetHeading(20075);
      dialog->SetLine(0, 12345);
      dialog->SetLine(1, dlgLine1);
      dialog->SetLine(2, 0);
      dialog->DoModal();
    }
  }
  else
    g_passwordManager.iMasterLockRetriesLeft = g_guiSettings.GetInt("masterlock.maxretries"); // user entered correct mastercode, reset retries to max allowed
}

bool CGUIPassword::CheckLock(int btnType, const CStdString& strPassword, int iHeading)
{
  bool bDummy;
  return CheckLock(btnType,strPassword,iHeading,bDummy);
}

bool CGUIPassword::CheckLock(int btnType, const CStdString& strPassword, int iHeading, bool& bCanceled)
{
  bCanceled = false;
  if (btnType == 0 || strPassword.Equals("-") || g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser)
    return true;

  int iVerifyPasswordResult = -1;
  CStdString strHeading = g_localizeStrings.Get(iHeading);
  switch (btnType)
  {
  case LOCK_MODE_NUMERIC:
    iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(const_cast<CStdString&>(strPassword), strHeading, 0);
    break;
  case LOCK_MODE_GAMEPAD:
    iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(const_cast<CStdString&>(strPassword), strHeading, 0);
    break;
  case LOCK_MODE_QWERTY:
    iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword(const_cast<CStdString&>(strPassword), strHeading, 0);
    break;
  default:   // must not be supported, treat as unlocked
    iVerifyPasswordResult = 0;
    break;
  }

  if (iVerifyPasswordResult == -1)
    bCanceled = true;

  return (iVerifyPasswordResult==0);
}

bool CGUIPassword::CheckMenuLock(int iWindowID)
{
  bool bCheckPW         = false;
  int iSwitch = iWindowID;

  // check if a settings subcategory was called from other than settings window
  if (iWindowID >= WINDOW_SCREEN_CALIBRATION && iWindowID <= WINDOW_SETTINGS_APPEARANCE)
  {
    int iCWindowID = m_gWindowManager.GetActiveWindow();
    if (iCWindowID != WINDOW_SETTINGS_MENU && (iCWindowID < WINDOW_SCREEN_CALIBRATION || iCWindowID > WINDOW_SETTINGS_APPEARANCE))
      iSwitch = WINDOW_SETTINGS_MENU;
  }

  if (iWindowID == WINDOW_MUSIC_FILES)
    if (m_gWindowManager.GetActiveWindow() == WINDOW_MUSIC_NAV)
      iSwitch = WINDOW_HOME;

  if (iWindowID == WINDOW_MUSIC_NAV)
    if (m_gWindowManager.GetActiveWindow() == WINDOW_HOME)
      iSwitch = WINDOW_MUSIC_FILES;

  if (iWindowID == WINDOW_VIDEO_NAV)
    if (m_gWindowManager.GetActiveWindow() == WINDOW_HOME)
      iSwitch = WINDOW_VIDEO_FILES;

  if (iWindowID == WINDOW_VIDEO_FILES)
    if (m_gWindowManager.GetActiveWindow() == WINDOW_VIDEO_NAV)
      iSwitch = WINDOW_HOME;

  CLog::Log(LOGDEBUG, "Checking if window ID %i is locked.", iSwitch);

  switch (iSwitch)
  {
    case WINDOW_SETTINGS_MENU:  // Settings
      bCheckPW = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].settingsLocked();
      break;
    case WINDOW_FILES:          // Files
      bCheckPW = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].filesLocked();
      break;
    case WINDOW_PROGRAMS:       // Programs
    case WINDOW_SCRIPTS:
      bCheckPW = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].programsLocked();
      break;
    case WINDOW_GAMESAVES:
      bCheckPW = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].programsLocked();
      break;
    case WINDOW_MUSIC_FILES:    // Music
      bCheckPW = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].musicLocked();
      break;
    case WINDOW_VIDEO_FILES:    // Video
      bCheckPW = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].videoLocked();
      break;
    case WINDOW_PICTURES:       // Pictures
      bCheckPW = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].picturesLocked();
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
CStdString CGUIPassword::GetSMBAuthFilename(const CStdString& strAuth)
{
  /// \brief Gets the path of an authenticated share
  /// \param strAuth The SMB style path
  /// \return Path to share with proper username/password, or same as imput if none found in db
  CURL urlIn(strAuth);
  CStdString strPath(strAuth);

  CStdString strShare;  // it's only the server\share we're interested in authenticating
  strShare  = urlIn.GetHostName();
  strShare += "/";
  strShare += urlIn.GetShareName();

  IMAPPASSWORDS it = m_mapSMBPasswordCache.find(strShare);
  if(it != m_mapSMBPasswordCache.end())
  {
    // if share found in cache use it to supply username and password
    CURL url(it->second);		// map value contains the full url of the originally authenticated share. map key is just the share
    CStdString strPassword = url.GetPassWord();

    CStdString strUserName = url.GetUserName();
    urlIn.SetPassword(strPassword);
    urlIn.SetUserName(strUserName);
    urlIn.GetURL(strPath);
  }
  return strPath;
}

bool CGUIPassword::LockSource(const CStdString& strType, const CStdString& strName, bool bState)
{
  VECSHARES* pShares = g_settings.GetSharesFromType(strType);
  bool bResult = false;
  for (IVECSHARES it=pShares->begin();it != pShares->end();++it)
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
  m_gWindowManager.SendThreadMessage(msg);

  return bResult;
}

void CGUIPassword::LockSources(bool lock)
{
  // lock or unlock all sources (those with locks)
  const char* strType[5] = {"programs","music","video","pictures","files"};
  for (int i=0;i<5;++i)
  {
    VECSHARES *shares = g_settings.GetSharesFromType(strType[i]);
    for (IVECSHARES it=shares->begin();it != shares->end();++it)
      if (it->m_iLockMode != LOCK_MODE_EVERYONE)
        it->m_iHasLock = lock ? 2 : 1;
  }
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
  m_gWindowManager.SendThreadMessage(msg);
}

void CGUIPassword::RemoveSourceLocks()
{
  // remove lock from all sources
  const char* strType[5] = {"programs","music","video","pictures","files"};
  for (int i=0;i<5;++i)
  {
    VECSHARES *shares = g_settings.GetSharesFromType(strType[i]);
    for (IVECSHARES it=shares->begin();it != shares->end();++it)
      if (it->m_iLockMode != LOCK_MODE_EVERYONE) // remove old info
      {
        it->m_iHasLock = 0;
        it->m_iLockMode = LOCK_MODE_EVERYONE;
        g_settings.UpdateSource(strType[i],it->strName,"lockmode","0"); // removes locks from xml
      }
  }
  g_settings.SaveSources();
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0, GUI_MSG_UPDATE_SOURCES);
  m_gWindowManager.SendThreadMessage(msg);
}

bool CGUIPassword::IsDatabasePathUnlocked(CStdString& strPath, VECSHARES& vecShares)
{
  if (g_passwordManager.bMasterUser || g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE)
    return true;

  // try to find the best matching source
  bool bName = false;
  int iIndex = CUtil::GetMatchingShare(strPath, vecShares, bName);

  if (iIndex > -1)
    if (vecShares[iIndex].m_iHasLock < 2)
      return true;

  return false;
}