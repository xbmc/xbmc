#include "stdafx.h"
#include "GUIPassword.h"
#include "Application.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogGamepad.h"
#include "util.h"
#include "xbox/xkutils.h"

CGUIPassword g_passwordManager;

CGUIPassword::CGUIPassword(void)
{
  m_bConfirmed = false;
  m_bCanceled = false;
  m_SMBShare = "";
  m_masterLockRetriesRemaining = 0;
}

CGUIPassword::~CGUIPassword(void)
{
}

bool CGUIPassword::IsItemUnlocked(CFileItem* pItem, const CStdString &strType)
{
  // \brief Tests if the user is allowed to access the share folder
  // \param pItem The share folder item to access
  // \param strType The type of share being accessed, e.g. "music", "video", etc. See CSettings::UpdateBookmark
  // \return If access is granted, returns \e true
  while (LOCK_MODE_EVERYONE < pItem->m_iLockMode)
  {
    CStdString strLabel = pItem->GetLabel();
    bool bConfirmed = false;
    bool bCanceled = false;
    int iResult = 0;  // init to user succeeded state, doing this to optimize switch statement below
    char buffer[33]; // holds 32 places plus sign character
    int iRetries = 0;
    // TODO: MasterLock - fix this
    if (MasterUserOverridesPasswords())// Check if we are the MasterUser!
    {
      // we are the master user and have inputted the master pass already
      iResult = 0; 
    }
    else if (!g_passwordManager.MasterLockDisabled())
    {
      // not the master user
      if (!(1 > pItem->m_iBadPwdCount))
        iRetries = g_guiSettings.GetInt("MasterLock.MaxRetries") - pItem->m_iBadPwdCount;
      if (0 != g_guiSettings.GetInt("MasterLock.MaxRetries") && pItem->m_iBadPwdCount >= g_guiSettings.GetInt("MasterLock.MaxRetries"))
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
      
      switch (pItem->m_iLockMode)
      {
      case LOCK_MODE_NUMERIC:
        iResult = CGUIDialogNumeric::ShowAndVerifyPassword(pItem->m_strLockCode, strHeading, iRetries);
        break;
      case LOCK_MODE_GAMEPAD:
        iResult = CGUIDialogGamepad::ShowAndVerifyPassword(pItem->m_strLockCode, strHeading, iRetries);
        break;
      case LOCK_MODE_QWERTY:
        iResult = CGUIDialogKeyboard::ShowAndVerifyPassword(pItem->m_strLockCode, strHeading, iRetries);
        break;
      default:
        // pItem->m_iLockMode isn't set to an implemented lock mode, so treat as unlocked
        return true;
        break;
      }
    //
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
        pItem->m_iLockMode = pItem->m_iLockMode * -1;
        itoa(pItem->m_iLockMode, buffer, 10);
        g_settings.UpdateBookmark(strType, strLabel, "lockmode", itoa(pItem->m_iLockMode, buffer, 10));
        pItem->m_iBadPwdCount = 0;
        g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", itoa(pItem->m_iBadPwdCount, buffer, 10));
        break;
      }
    case 1:
      {
        // password entry failed
        if (0 != g_guiSettings.GetInt("MasterLock.MaxRetries"))
          pItem->m_iBadPwdCount++;
        g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", itoa(pItem->m_iBadPwdCount, buffer, 10));
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

bool CGUIPassword::IsItemUnlocked(CShare* pItem, const CStdString &strType)
{
  // \brief Tests if the user is allowed to access the share folder
  // \param pItem The share folder item to access
  // \param strType The type of share being accessed, e.g. "music", "video", etc. See CSettings::UpdateBookmark
  // \return If access is granted, returns \e true
  while (LOCK_MODE_EVERYONE < pItem->m_iLockMode)
  {
    CStdString strLabel = pItem->strName;
    bool bConfirmed = false;
    bool bCanceled = false;
    int iResult = 0;  // init to user succeeded state, doing this to optimize switch statement below
    char buffer[33]; // holds 32 places plus sign character
    int iRetries = 0;
    if (!(1 > pItem->m_iBadPwdCount)) iRetries = g_guiSettings.GetInt("MasterLock.MaxRetries") - pItem->m_iBadPwdCount;
    if (0 != g_guiSettings.GetInt("MasterLock.MaxRetries") && pItem->m_iBadPwdCount >= g_guiSettings.GetInt("MasterLock.MaxRetries"))
    {
      // user previously exhausted all retries, show access denied error
      CGUIDialogOK::ShowAndGetInput(12345, 12346, 0, 0);
      return false;
    }
    if (MasterUserOverridesPasswords())// Check if we are the MasterUser!
    {
      iResult = 0; 
    }
    else if (!g_passwordManager.MasterLockDisabled())
    {
      // show the appropriate lock dialog
      CStdString strHeading = g_localizeStrings.Get(12348);
      switch (pItem->m_iLockMode)
      {
      case LOCK_MODE_NUMERIC:
        iResult = CGUIDialogNumeric::ShowAndVerifyPassword(pItem->m_strLockCode, strHeading, iRetries);
        break;
      case LOCK_MODE_GAMEPAD:
        iResult = CGUIDialogGamepad::ShowAndVerifyPassword(pItem->m_strLockCode, strHeading, iRetries);
        break;
      case LOCK_MODE_QWERTY:
        iResult = CGUIDialogKeyboard::ShowAndVerifyPassword(pItem->m_strLockCode, strHeading, iRetries);
        break;
      default:
        return true;
        break;
      }
    }
    switch (iResult)
    {
    case -1: // user canceled out
      {
        return false;
        break; 
      }
    case 0: // password entry succeeded
      {
        pItem->m_iLockMode = pItem->m_iLockMode * -1;
        itoa(pItem->m_iLockMode, buffer, 10);
        g_settings.UpdateBookmark(strType, strLabel, "lockmode", itoa(pItem->m_iLockMode, buffer, 10));
        pItem->m_iBadPwdCount = 0;
        g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", itoa(pItem->m_iBadPwdCount, buffer, 10));
        break;
      }
    case 1: // password entry failed
      {
        if (0 != g_guiSettings.GetInt("MasterLock.MaxRetries"))  pItem->m_iBadPwdCount++;
        g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", itoa(pItem->m_iBadPwdCount, buffer, 10));
        break;
      }
    default:
      {
        return false;
        break;
      }
    }
  }
  return true;
}

bool CGUIPassword::IsConfirmed() const
{
  return m_bConfirmed;
}

bool CGUIPassword::IsCanceled() const
{
  return m_bCanceled;
}

void CGUIPassword::SetSMBShare(const CStdString &strPath)
{
  m_SMBShare = strPath;
  m_bCanceled = false;
}

CStdString CGUIPassword::GetSMBShare()
{
  return m_SMBShare;
}

void CGUIPassword::GetSMBShareUserPassword()
{
  CURL url(m_SMBShare);
  CStdString passcode;
  CStdString username = url.GetUserName();
  CStdString outusername = username;
  CStdString share;
  url.GetURLWithoutUserDetails(share);

  CStdString header;
  header.Format("%s %s", g_localizeStrings.Get(14062).c_str(), share.c_str());

  if (!CGUIDialogKeyboard::ShowAndGetInput(outusername, header, false))
  {
    if (outusername.IsEmpty() || outusername != username) // just because the routine returns false when enter is hit and nothing was changed
    {
      m_bCanceled = true;
      return;
    }
  }
  if (!CGUIDialogKeyboard::ShowAndGetInput(passcode, g_localizeStrings.Get(12326), true))
  {
    m_bCanceled = true;
    return;
  }
  url.SetPassword(passcode);
  url.SetUserName(outusername);
  url.GetURL(m_SMBShare);
}

bool CGUIPassword::CheckStartUpLock()   // GeminiServer
{
  if (m_masterLockRetriesRemaining == 0)
    m_masterLockRetriesRemaining = g_guiSettings.GetInt("MasterLock.MaxRetries");
  return CheckMasterLock(true);

  // prompt user for mastercode if the mastercode was set b4 or by xml
  int iVerifyPasswordResult = -1;
  CStdString strHeader = g_localizeStrings.Get(12324);
  CStdString password = g_guiSettings.GetString("MasterLock.MasterCode");
  for (int i=1; i <= m_masterLockRetriesRemaining; i++)
  {
    switch (g_guiSettings.GetInt("MasterUser.LockMode"))
    { // Prompt user for mastercode
      case LOCK_MODE_NUMERIC:
        iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(password, strHeader, 0);
        break;
      case LOCK_MODE_GAMEPAD:
        iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(password, strHeader, 0);
        break;
      case LOCK_MODE_QWERTY:
        iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword(password, strHeader, 0);
        break;
    }
    if (iVerifyPasswordResult != 0 )
    {
      CStdString strLabel,strLabel1;
      strLabel1 = g_localizeStrings.Get(12343);
      int iLeft = m_masterLockRetriesRemaining - i;
      strLabel.Format("%i %s %s",iLeft,strLabel1.c_str(), "before shutdown!");
      
      // PopUp OK and Display: MasterLock mode has changed but no no Mastercode has been set!
      CGUIDialogOK *dlg = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (!dlg) return false;
      dlg->SetHeading( g_localizeStrings.Get(12369) );
      dlg->SetLine( 0, g_localizeStrings.Get(12367) );
      dlg->SetLine( 1, g_localizeStrings.Get(12368) );
      dlg->SetLine( 2, strLabel);
      dlg->DoModal( m_gWindowManager.GetActiveWindow() );
    }
    else
      i = m_masterLockRetriesRemaining;
  }
  if (iVerifyPasswordResult == 0)
  {
    m_masterLockRetriesRemaining = g_guiSettings.GetInt("MasterLock.MaxRetries");
    return true;  // OK The MasterCode Accepted! XBMC Can Run!
  }
  else 
  {
    g_applicationMessenger.Shutdown(); // Turn off the box
    return false;
  }
}

bool CGUIPassword::MasterLockDisabled()
{
  return (g_guiSettings.GetInt("MasterUser.LockMode") == LOCK_MODE_EVERYONE);
}

bool CGUIPassword::MasterUserOverridesPasswords()
{
  if (g_guiSettings.GetInt("MasterUser.LockMode") == LOCK_MODE_EVERYONE)
    return false;
  return IsMasterUser();
}

bool CGUIPassword::IsMasterUser()
{
  if (g_guiSettings.GetInt("MasterUser.LockMode") == LOCK_MODE_EVERYONE)
    g_guiSettings.SetBool("MasterLock.MasterUser", true);
  return g_guiSettings.GetBool("MasterLock.MasterUser");
}

void CGUIPassword::LogoutMasterUser()
{
  g_guiSettings.SetBool("MasterLock.MasterUser", false);
  m_masterLockRetriesRemaining = g_guiSettings.GetInt("MasterLock.MaxRetries");
}

bool CGUIPassword::CheckMasterLock(bool shutdownOnFailure /* = false*/)
{
  if (IsMasterUser())
    return true;

  // prompt for master lock
  int iVerifyPasswordResult = -1;
  CStdString strHeading = g_localizeStrings.Get(12324);

  while (true)
  {
    if (g_guiSettings.GetInt("MasterLock.MaxRetries") > 0)
    {
      if (m_masterLockRetriesRemaining <= 0)
      {
        m_masterLockRetriesRemaining = 0;
        // failed to input correct password
        if (shutdownOnFailure || g_guiSettings.GetBool("MasterLock.EnableShutdown"))
        {
          // Shutdown enabled, tell the user we're shutting off
          CGUIDialogOK::ShowAndGetInput(12345, 12346, 12347, 0);
#ifndef _DEBUG  // don't actually shut off if debug build, it hangs VS for a long time
          g_application.Stop();
          Sleep(200);
          XKUtils::XBOXPowerOff();
#endif
          return false;
        }
        // Tell the user they ran out of retry attempts
        CGUIDialogOK::ShowAndGetInput(12345, 12346, 0, 0);
        return false;
      }
      else
      { // prompt user with how many retries they have
        CStdString retrys;
        retrys.Format("%d %s", m_masterLockRetriesRemaining, g_localizeStrings.Get(12343));
        CGUIDialogOK *dialog = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK); // Tell user they entered a bad password
        if (dialog) 
        {
          dialog->SetHeading(12324);
          dialog->SetLine(0, 12345);
          dialog->SetLine(1, retrys);
          dialog->SetLine(2, 0);
          dialog->DoModal(m_gWindowManager.GetActiveWindow());
        }
      }
      m_masterLockRetriesRemaining--;
    }
    CStdString password = g_guiSettings.GetString("MasterLock.MasterCode");
    switch (g_guiSettings.GetInt("MasterUser.LockMode"))
    {
    case LOCK_MODE_NUMERIC:
      iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(password, strHeading, 0); //g_guiSettings.GetInt("MasterLock.MaxRetries"));
      break;
    case LOCK_MODE_GAMEPAD:
      iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(password, strHeading, 0); //g_guiSettings.GetInt("MasterLock.MaxRetries"));
      break;
    case LOCK_MODE_QWERTY:
      iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword(password, strHeading, 0); //g_guiSettings.GetInt("MasterLock.MaxRetries"));
      break;
    default:   // must not be supported, treat as unlocked
      iVerifyPasswordResult = 0;
      break;
    }
    if (iVerifyPasswordResult == 0)
    {
      g_guiSettings.SetBool("MasterLock.MasterUser", true);
      return true;
    }
    if (iVerifyPasswordResult == -1)
      return false; // cancelled
  }
  return false;
}

bool CGUIPassword::CheckMenuLock(int iWindowID)
{
  bool bCheckPW         = false;
  int iLockFilemanager  = g_guiSettings.GetInt("MasterLock.LockSettingsFileManager") & LOCK_MASK_FILE_MANAGER;
  int iLockSettings     = g_guiSettings.GetInt("MasterLock.LockSettingsFileManager") & LOCK_MASK_SETTINGS;
  int iLockHomeMedia    = g_guiSettings.GetInt("MasterLock.LockHomeMedia");
  switch (iWindowID)
  {
    case WINDOW_SETTINGS_MENU:  // Settings 
      if (iLockSettings == 1) bCheckPW = true;
      break;
    
    // Sub Settings: Todo: needs a new Section Lock SubSettings! Disabling for now
    /*
    case WINDOW_SETTINGS_MYPICTURES:
      if (iLockSettings == 1) bCheckPW = true;
      break;
    case WINDOW_SETTINGS_MYPROGRAMS:
      if (iLockSettings == 1) bCheckPW = true;
      break;
    case WINDOW_SETTINGS_MYWEATHER:
      if (iLockSettings == 1) bCheckPW = true;
      break;
    case WINDOW_SETTINGS_MYMUSIC:
      if (iLockSettings == 1) bCheckPW = true;
      break;
    case WINDOW_SETTINGS_SYSTEM:
      if (iLockSettings == 1) bCheckPW = true;
      break;
    case WINDOW_SETTINGS_MYVIDEOS:
      if (iLockSettings == 1) bCheckPW = true;
      break;
    case WINDOW_SETTINGS_NETWORK:
      if (iLockSettings == 1) bCheckPW = true;
      break;
    case WINDOW_SETTINGS_APPEARANCE:
      if (iLockSettings == 1) bCheckPW = true;
      break;
    */

    case WINDOW_FILES:          // Files
      if (iLockFilemanager == 1) bCheckPW = true;
      break;
    case WINDOW_PROGRAMS:       // Programs
      if (  iLockHomeMedia == LOCK_PROGRAMS     || iLockHomeMedia == LOCK_MU_PROG       || iLockHomeMedia == LOCK_VI_PROG     || 
            iLockHomeMedia == LOCK_PIC_PROG     || iLockHomeMedia == LOCK_PROG_VI_MU    || iLockHomeMedia == LOCK_PROG_PIC_MU || 
            iLockHomeMedia == LOCK_PROG_PIC_VI  || iLockHomeMedia == LOCK_MU_VI_PIC_PROG  )
        bCheckPW = true;
      break;
    case WINDOW_MUSIC_FILES:    // Music
      if (  iLockHomeMedia == LOCK_MUSIC        || iLockHomeMedia == LOCK_MU_VI          || iLockHomeMedia == LOCK_MU_PIC     || 
            iLockHomeMedia == LOCK_MU_PROG      || iLockHomeMedia == LOCK_MU_VI_PIC      || iLockHomeMedia == LOCK_PROG_VI_MU || 
            iLockHomeMedia == LOCK_PROG_PIC_MU  || iLockHomeMedia == LOCK_MU_VI_PIC_PROG   )
      bCheckPW = true;
      break;
    case WINDOW_VIDEOS:         // Video
      if (  iLockHomeMedia == LOCK_VIDEO        || iLockHomeMedia == LOCK_MU_VI          || iLockHomeMedia == LOCK_VI_PIC     || 
            iLockHomeMedia == LOCK_VI_PROG      || iLockHomeMedia == LOCK_MU_VI_PIC      || iLockHomeMedia == LOCK_PROG_VI_MU || 
            iLockHomeMedia == LOCK_PROG_PIC_VI  || iLockHomeMedia == LOCK_MU_VI_PIC_PROG   )
      bCheckPW = true;
      break;
    case WINDOW_PICTURES:       // Pictures
      if (  iLockHomeMedia == LOCK_PICTURES     || iLockHomeMedia == LOCK_MU_PIC         || iLockHomeMedia == LOCK_VI_PIC      || 
            iLockHomeMedia == LOCK_PIC_PROG     || iLockHomeMedia == LOCK_MU_VI_PIC      || iLockHomeMedia == LOCK_PROG_PIC_MU || 
            iLockHomeMedia == LOCK_PROG_PIC_VI  || iLockHomeMedia == LOCK_MU_VI_PIC_PROG   )
      bCheckPW = true;
      break;
    default:
      bCheckPW = false;
      break;
  }
  if (bCheckPW)
    return CheckMasterLock(); //Now let's check the PW if we need!
  else
    return true;
}

/// \brief Gets the path of an authenticated share
/// \param strAuth The SMB style path
/// \return Path to share with proper username/password, or same as imput if none found in db
CStdString CGUIPassword::GetSMBAuthFilename(const CStdString& strAuth)
{
  CURL urlIn(strAuth);
  CStdString strPath(strAuth);

  CStdString strShare = urlIn.GetShareName();	// it's only the server\share we're interested in authenticating
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

bool CGUIPassword::IsDatabasePathUnlocked(CStdString& strPath, VECSHARES& vecShares)
{
  // try to find the best matching bookmark
  bool bName = false;
  int iIndex = CUtil::GetMatchingShare(strPath, vecShares, bName);

  // are lockmodes LOCK_MODE_SAMBA and LOCK_MODE_EEPROM_PARENTAL in use?
  // to be safe, ignore them for now and only test for 1,2,3
  if (iIndex > -1 &&
    /* bookmark is unlocked */
    (g_settings.m_vecMyVideoShares[iIndex].m_iLockMode <= LOCK_MODE_EVERYONE ||
    /* bookmark is locked with LOCK_MODE_SAMBA or LOCK_MODE_EEPROM_PARENTAL */
    g_settings.m_vecMyVideoShares[iIndex].m_iLockMode > LOCK_MODE_QWERTY || 
    /* we're in master user mode */
    g_passwordManager.IsMasterUser())
    )
    return true;

  return false;
}
