#include "stdafx.h"
#include "GUIPassword.h"
#include "Application.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogGamepad.h"
#include "util.h"
#include "settings.h"
#include "xbox/xkutils.h"

CGUIPassword g_passwordManager;

CGUIPassword::CGUIPassword(void)
{
  m_bConfirmed = false;
  m_bCanceled = false;
  m_SMBShare = "";
}
CGUIPassword::~CGUIPassword(void)
{}
bool CGUIPassword::GetSettings()
{
  // TODO: detect and set g_advancedSettings 
  //       --> Then Same conditions for CheckMasterLock()!
  if (!g_advancedSettings.bUseMasterLockAdvancedXml)
  {
    iMasterLockMode                 = g_guiSettings.GetInt("masterlock.mastermode");
    strMasterLockCode               = g_guiSettings.GetString("masterlock.mastercode");
    bMasterNormalUserMode           = g_guiSettings.GetString("masterlock.usermode").Equals("0"); //true 0:Normal false 1:Advanced 
    iMasterLockHomeMedia            = g_guiSettings.GetInt("masterlock.homemedia");
    iMasterLockSetFile              = g_guiSettings.GetInt("masterlock.settingsfilemanager"); 
    bMasterLockEnableShutdown       = g_guiSettings.GetBool("masterlock.enableshutdown");
    bMasterLockProtectShares        = g_guiSettings.GetBool("masterlock.protectshares");
    bMasterUser                     = g_guiSettings.GetBool("masterlock.masteruser");
    bMasterLockStartupLock          = g_guiSettings.GetBool("masterlock.startuplock");
    iMasterLockMaxRetry             = 3; //default value 3!
  }
  else
  {
    iMasterLockMode                 = g_advancedSettings.iMasterLockMode;
    strMasterLockCode               = g_advancedSettings.strMasterLockCode;
    bMasterNormalUserMode           = g_advancedSettings.bMasterUserMode;
    iMasterLockHomeMedia            = g_advancedSettings.iMasterLockHomeMedia;
    iMasterLockSetFile              = g_advancedSettings.iMasterLockSettingsFilemanager;
    bMasterLockEnableShutdown       = g_advancedSettings.bMasterLockEnableShutdown;
    bMasterLockProtectShares        = g_advancedSettings.bMasterLockProtectShares;
    bMasterUser                     = g_advancedSettings.bMasterUser;
    bMasterLockStartupLock          = g_advancedSettings.bMasterLockStartupLock;

    if (g_advancedSettings.iMasterLockMaxRetry > 0)
      iMasterLockMaxRetry = g_advancedSettings.iMasterLockMaxRetry;
    else 
      iMasterLockMaxRetry             = 3; //default value 3!
  }

  //bMasterLockFilemanager
  if (iMasterLockSetFile == 1 || iMasterLockSetFile == 3) 
    bMasterLockFilemanager = true;
  else bMasterLockFilemanager = false;
  //bMasterLockSettings
  if (iMasterLockSetFile == 2 || iMasterLockSetFile == 3)
    bMasterLockSettings = true;
  else bMasterLockSettings = false;
  
  return true;
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
    if(g_passwordManager.bMasterUser && (g_passwordManager.iMasterLockMode > 0) )// Check if we are the MasterUser!
    {
      if(MasterUser()) iResult = 0; 
      else iResult = -1;
    }
    else if (g_passwordManager.bMasterNormalUserMode) // Normal User: One Pass mode
    {
      if(MasterUser()) iResult = 0; 
      else iResult = -1;
    }
    else
    {
      if (!(1 > pItem->m_iBadPwdCount))
        iRetries = g_passwordManager.iMasterLockMaxRetry - pItem->m_iBadPwdCount;
      if (0 != g_passwordManager.iMasterLockMaxRetry && pItem->m_iBadPwdCount >= g_passwordManager.iMasterLockMaxRetry)
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
        if (!g_application.m_bMasterLockOverridesLocalPasswords)
          iResult = CGUIDialogNumeric::ShowAndVerifyPassword(pItem->m_strLockCode, strHeading, iRetries);
        break;
      case LOCK_MODE_GAMEPAD:
        if (!g_application.m_bMasterLockOverridesLocalPasswords)
          iResult = CGUIDialogGamepad::ShowAndVerifyPassword(pItem->m_strLockCode, strHeading, iRetries);
        break;
      case LOCK_MODE_QWERTY:
        if (!g_application.m_bMasterLockOverridesLocalPasswords)
          iResult = CGUIDialogKeyboard::ShowAndVerifyPassword(pItem->m_strLockCode, strHeading, iRetries);
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
        if (0 != g_passwordManager.iMasterLockMaxRetry)
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
    if (!(1 > pItem->m_iBadPwdCount)) iRetries = g_passwordManager.iMasterLockMaxRetry - pItem->m_iBadPwdCount;
    if (0 != g_passwordManager.iMasterLockMaxRetry && pItem->m_iBadPwdCount >= g_passwordManager.iMasterLockMaxRetry)
    {
      // user previously exhausted all retries, show access denied error
      CGUIDialogOK::ShowAndGetInput(12345, 12346, 0, 0);
      return false;
    }
    if(g_passwordManager.bMasterUser && (g_passwordManager.iMasterLockMode > 0) )// Check if we are the MasterUser!
    {
      if(MasterUser()) iResult = 0; 
      else iResult = -1;
    }
    else
    {
      // show the appropriate lock dialog
      CStdString strHeading = g_localizeStrings.Get(12348);
      switch (pItem->m_iLockMode)
      {
      case LOCK_MODE_NUMERIC:
        if (!g_application.m_bMasterLockOverridesLocalPasswords)
          iResult = CGUIDialogNumeric::ShowAndVerifyPassword(pItem->m_strLockCode, strHeading, iRetries);
        break;
      case LOCK_MODE_GAMEPAD:
        if (!g_application.m_bMasterLockOverridesLocalPasswords)
          iResult = CGUIDialogGamepad::ShowAndVerifyPassword(pItem->m_strLockCode, strHeading, iRetries);
        break;
      case LOCK_MODE_QWERTY:
        if (!g_application.m_bMasterLockOverridesLocalPasswords)
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
        if (0 != g_passwordManager.iMasterLockMaxRetry)  pItem->m_iBadPwdCount++;
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
  // prompt user for mastercode if the mastercode was set b4 or by xml
  int iVerifyPasswordResult = -1;
  CStdString strHeader = g_localizeStrings.Get(12324);
  if (g_application.m_iMasterLockRetriesRemaining == 0) g_application.m_iMasterLockRetriesRemaining = 1;
  for (int i=1; i <= g_application.m_iMasterLockRetriesRemaining; i++)
  {
    switch (g_passwordManager.iMasterLockMode)
    { // Prompt user for mastercode
      case LOCK_MODE_NUMERIC:
        iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(g_passwordManager.strMasterLockCode, strHeader, 0);
        break;
      case LOCK_MODE_GAMEPAD:
        iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(g_passwordManager.strMasterLockCode, strHeader, 0);
        break;
      case LOCK_MODE_QWERTY:
        iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword(g_passwordManager.strMasterLockCode, strHeader, 0);
        break;
    }
    if (iVerifyPasswordResult != 0 )
    {
      CStdString strLabel,strLabel1;
      strLabel1 = g_localizeStrings.Get(12343);
      int iLeft = g_application.m_iMasterLockRetriesRemaining-i;
      strLabel.Format("%i %s",iLeft,strLabel1.c_str());
      
      // PopUp OK and Display: MasterLock mode has changed but no no Mastercode has been set!
      CGUIDialogOK *dlg = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (!dlg) return false;
      dlg->SetHeading( g_localizeStrings.Get(12369) );
      dlg->SetLine( 0, g_localizeStrings.Get(12367) );
      dlg->SetLine( 1, g_localizeStrings.Get(12368) );
      dlg->SetLine( 2, strLabel);
      dlg->DoModal();
    }
    else i=g_application.m_iMasterLockRetriesRemaining;
  }
  if (iVerifyPasswordResult == 0)
  {
    g_passwordManager.bMasterLockStartupLock = 0;
    g_application.m_iMasterLockRetriesRemaining = g_passwordManager.iMasterLockMaxRetry;
      return true;  // OK The MasterCode Accepted! XBMC Can Run!
  }
  else 
  {
    g_applicationMessenger.Shutdown(); // Turn off the box
    return false;
  }
}
bool CGUIPassword::IsMasterLockUnlocked(bool bPromptUser)
{
  if ((LOCK_MODE_EVERYONE < g_passwordManager.iMasterLockMode) && !bPromptUser)
    // not unlocked, but calling code doesn't want to prompt user
    return false;

  if (1 > g_passwordManager.iMasterLockMode)
  {  // already unlocked, don't prompt user for anything
    g_application.m_bMasterLockPreviouslyEntered = true;
    return true;
  }
  if(!g_passwordManager.bMasterUser)
  {
    if (0 < g_passwordManager.iMasterLockMaxRetry && 1 > g_application.m_iMasterLockRetriesRemaining)
    {
      // user has previously exhausted all their attempts, so don't prompt them again
      UpdateMasterLockRetryCount(false);
      return false;
    }
  }
  // Are we in MasterUser Mode ?
  if (g_application.m_bMasterLockOverridesLocalPasswords) return true; 

  int iVerifyPasswordResult = -1;
  CStdString strHeading = g_localizeStrings.Get(12324);
  switch (g_passwordManager.iMasterLockMode)
  {
  case LOCK_MODE_NUMERIC:
    iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(g_passwordManager.strMasterLockCode, strHeading, 0);
    break;
  case LOCK_MODE_GAMEPAD:
    iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(g_passwordManager.strMasterLockCode, strHeading, 0);
    break;
  case LOCK_MODE_QWERTY:
    iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword(g_passwordManager.strMasterLockCode, strHeading, 0);
    break;
  default:   // must not be supported, treat as unlocked
    iVerifyPasswordResult = 0;
    break;
  }

  if ( iVerifyPasswordResult == -1 ) return false;
  if ( iVerifyPasswordResult != 0 && iVerifyPasswordResult == 1) 
  {
    UpdateMasterLockRetryCount(false);
    return false;
  }
  UpdateMasterLockRetryCount(true);
  if (!g_application.m_bMasterLockPreviouslyEntered)
  {
    g_application.m_bMasterLockPreviouslyEntered = true;
  }

  // Let's count the Overwrite the local Pass stuff!
  if ((g_application.m_MasterUserModeCounter == 0) && (g_passwordManager.bMasterUser))
    {
      if (CGUIDialogYesNo::ShowAndGetInput(12324, 12354, 0, 0)) // remove all share lock
      {
        g_application.m_bMasterLockOverridesLocalPasswords = true;
        return true;
      }
      else 
      {
        g_application.m_MasterUserModeCounter = -1 ;
        return true;
      }
    }
  else if (g_application.m_MasterUserModeCounter >=0) g_application.m_MasterUserModeCounter--;
  return true;
}
bool CGUIPassword::IsMasterLockLocked(bool bPromptUser)
{
  if (( g_passwordManager.iMasterLockMode > LOCK_MODE_EVERYONE) && !bPromptUser)
    return false;

  if (g_passwordManager.iMasterLockMode  < 1 )
  { 
    g_application.m_bMasterLockPreviouslyEntered = true;
    return true;
  }

  if (g_passwordManager.iMasterLockMaxRetry > 0 && g_application.m_iMasterLockRetriesRemaining < 1 )
  {
    UpdateMasterLockRetryCount(false);
    return false;
  }

  int iVerifyPasswordResult = -1;
  CStdString strHeading = g_localizeStrings.Get(12324);
  switch (g_passwordManager.iMasterLockMode)
  {
  case LOCK_MODE_NUMERIC:
    iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(g_passwordManager.strMasterLockCode, strHeading, 0);
    break;
  case LOCK_MODE_GAMEPAD:
    iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(g_passwordManager.strMasterLockCode, strHeading, 0);
    break;
  case LOCK_MODE_QWERTY:
    iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword(g_passwordManager.strMasterLockCode, strHeading, 0);
    break;
  default:   // must not be supported, treat as unlocked
    iVerifyPasswordResult = 0;
    break;
  }
  if (1 == iVerifyPasswordResult) UpdateMasterLockRetryCount(false);
  if (0 != iVerifyPasswordResult) return false;

  // user successfully entered mastercode
  UpdateMasterLockRetryCount(true);
  if (!g_application.m_bMasterLockPreviouslyEntered)  g_application.m_bMasterLockPreviouslyEntered = true;
  return true;
}
void CGUIPassword::UpdateMasterLockRetryCount(bool bResetCount)
{
  // \brief Updates Master Lock status.
  // \param bResetCount masterlock retry counter is zeroed if true, or incremented and displays an Access Denied dialog if false.
  if (!bResetCount)
  {
    // Bad mastercode entered
    if (0 < g_passwordManager.iMasterLockMaxRetry)
    {
      // We're keeping track of how many bad passwords are entered
      if (1 < g_application.m_iMasterLockRetriesRemaining)
      {
        // user still has at least one retry after decrementing
        g_application.m_iMasterLockRetriesRemaining--;
      }
      else
      {
        // user has run out of retry attempts
        g_application.m_iMasterLockRetriesRemaining = 0;
        if (1 == g_passwordManager.bMasterLockEnableShutdown)
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
    if (0 < g_application.m_iMasterLockRetriesRemaining)
      dlgLine1.Format("%d %s", g_application.m_iMasterLockRetriesRemaining, g_localizeStrings.Get(12343));
    CGUIDialogOK *dialog = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK); // Tell user they entered a bad password
    if (dialog) 
    {
      dialog->SetHeading(12324);
      dialog->SetLine(0, 12345);
      dialog->SetLine(1, dlgLine1);
      dialog->SetLine(2, 0);
      dialog->DoModal();
    }
  }
  else g_application.m_iMasterLockRetriesRemaining = g_passwordManager.iMasterLockMaxRetry; // user entered correct mastercode, reset retries to max allowed
  return ;
}
bool CGUIPassword::MasterUser()   // GeminiServer
{
  return (IsMasterLockUnlocked(true));
}
bool CGUIPassword::CheckMenuLock(int iWindowID)
{
  bool bCheckPW         = false;
  int iLockHomeMedia    = g_passwordManager.iMasterLockHomeMedia;
  switch (iWindowID)
  {
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

    case WINDOW_SETTINGS_MENU:  // Settings 
      bCheckPW = g_passwordManager.bMasterLockSettings;
      break;
    case WINDOW_FILES:          // Files
      bCheckPW = g_passwordManager.bMasterLockFilemanager;
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
    case WINDOW_VIDEO_FILES:    // Video
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
  if (bCheckPW) return MasterUser(); //Now let's check the PW if we need!
  else return true;
}
CStdString CGUIPassword::GetSMBAuthFilename(const CStdString& strAuth)
{
  /// \brief Gets the path of an authenticated share
  /// \param strAuth The SMB style path
  /// \return Path to share with proper username/password, or same as imput if none found in db
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
    (g_application.m_bMasterLockOverridesLocalPasswords && g_passwordManager.iMasterLockMode <= LOCK_MODE_EVERYONE))
    )
    return true;

  return false;
}

bool CGUIPassword::CheckMasterLockCode()
{
  // GeminiServer
  // prompt user for mastercode if the mastercode was set b4 or by xml
  // prompt user for mastercode when changing lock settings
  if (g_passwordManager.IsMasterLockLocked(true))  return true;
  else return false;
}
bool CGUIPassword::CheckMasterLock(bool bDisalogYesNo)
{
  bool bResetSettings=false;
  bool bCheckAndSafe=false;
  if (bMasterLockEnableShutdown != g_guiSettings.GetBool("masterlock.enableshutdown"))
  {    
    bCheckAndSafe=true;
  }
  else if (bMasterLockProtectShares != g_guiSettings.GetBool("masterlock.protectshares"))
  {    
    bCheckAndSafe=true;
  }
  else if (iMasterLockMode != g_guiSettings.GetInt("masterlock.mastermode"))
  {    
    bCheckAndSafe=true;
  }
  else if (strMasterLockCode != g_guiSettings.GetString("masterlock.mastercode"))
  {    
    bCheckAndSafe=true;
  }
  else if (bMasterLockStartupLock != g_guiSettings.GetBool("masterlock.startuplock"))
  {    
    bCheckAndSafe=true;
  }
  else if (iMasterLockHomeMedia != g_guiSettings.GetInt("masterlock.homemedia"))
  {    
    bCheckAndSafe=true;
  }
  else if (iMasterLockSetFile != g_guiSettings.GetInt("masterlock.settingsfilemanager"))
  {    
    bCheckAndSafe=true;
  }
  else if (bMasterNormalUserMode != g_guiSettings.GetString("masterlock.usermode").Equals("0"))
  {    
    bCheckAndSafe=true;
  }
  else if (bMasterUser != g_guiSettings.GetBool("masterlock.masteruser"))
  {    
    bCheckAndSafe=true;
  }
  
  if (bDisalogYesNo && bCheckAndSafe)
  {
    if(!CGUIDialogYesNo::ShowAndGetInput(12324, 12360, 750, 0)) //Settings changed Save?
      bCheckAndSafe =false;
  }
  //The settings are changed -> get GUIsettings to Password Manager
  if(bCheckAndSafe)
  {
    if(CheckMasterLockCode())
    {
      if(iMasterLockMode != g_guiSettings.GetInt("masterlock.mastermode") && ( strMasterLockCode == g_guiSettings.GetString("masterlock.mastercode") || g_guiSettings.GetString("masterlock.mastercode").IsEmpty() ))
      {
        // PopUp OK! and Display: MasterLock mode has changed but NO! Mastercode has been set!
        CGUIDialogOK::ShowAndGetInput(12360, 12370, 12371, 0);
        bResetSettings = true;
      }
      else if(iMasterLockMode != g_guiSettings.GetInt("masterlock.mastermode") && strMasterLockCode != g_guiSettings.GetString("masterlock.mastercode") && g_guiSettings.GetInt("masterlock.mastermode")!= LOCK_MODE_EVERYONE )
      {
        // PopUp OK and Display: MasterCode has changed! The new MasterCode is...!
        CGUIDialogOK *dlg = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
        if (!dlg) return false;
        dlg->SetHeading( g_localizeStrings.Get(12360));
        dlg->SetLine( 0, g_localizeStrings.Get(12366));
        dlg->SetLine( 1, strMasterLockCode.c_str());
        dlg->SetLine( 2, "");
        dlg->DoModal();
      }
      // MasterUser is should be True, save the settings now
      if(!bResetSettings)
        return GetSettings();
    }
    else
    {
      // PopUp OK and Display: Master Code is not Valid or is empty or not set!
      CGUIDialogOK::ShowAndGetInput(12360, 12367, 12368, 0);
      bResetSettings = true;
    }
  }
  if(bResetSettings)
  {
    // Resetting the changes
      g_guiSettings.SetBool("masterlock.enableshutdown",bMasterLockEnableShutdown);
      g_guiSettings.SetBool("masterlock.protectshares",bMasterLockProtectShares);
      g_guiSettings.SetBool("masterlock.masteruser",bMasterUser);
      g_guiSettings.SetInt("masterlock.mastermode",iMasterLockMode);
      g_guiSettings.SetString("masterlock.mastercode",strMasterLockCode.c_str());
      g_guiSettings.SetBool("masterlock.startuplock",bMasterLockStartupLock);
      g_guiSettings.SetInt("masterlock.homemedia",iMasterLockHomeMedia);
      g_guiSettings.SetInt("masterlock.settingsfilemanager",iMasterLockSetFile); 
      if(bMasterNormalUserMode)g_guiSettings.SetString("masterlock.usermode","0");
      if(!bMasterNormalUserMode)g_guiSettings.SetString("masterlock.usermode","1");
    //
  }
  return false;
}