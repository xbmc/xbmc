#include "stdafx.h"
#include "GUIPassword.h"
#include "Application.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogGamepad.h"
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
    
    //
    if(g_guiSettings.GetBool("Masterlock.MasterUser") && (g_guiSettings.GetInt("Masterlock.Mastermode") > 0) )// Check if we are the MasterUser!
    {
      if(MasterUser()) iResult = 0; 
      else iResult = -1;
    }
    else
    {
    //
      if (!(1 > pItem->m_iBadPwdCount))
        iRetries = g_stSettings.m_iMasterLockMaxRetry - pItem->m_iBadPwdCount;
      if (0 != g_stSettings.m_iMasterLockMaxRetry && pItem->m_iBadPwdCount >= g_stSettings.m_iMasterLockMaxRetry)
      { // user previously exhausted all retries, show access denied error
        CGUIDialogOK::ShowAndGetInput(12345, 12346, 0, 0);
        return false;
      }
      
      // show the appropriate lock dialog
      CStdStringW strHeading = L"";
      if (pItem->m_bIsFolder) strHeading = g_localizeStrings.Get(12325);
      else strHeading = g_localizeStrings.Get(12348);
      
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
    //
    }
    //


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
        if (0 != g_stSettings.m_iMasterLockMaxRetry)
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
    if (!(1 > pItem->m_iBadPwdCount))
    {
      iRetries = g_stSettings.m_iMasterLockMaxRetry - pItem->m_iBadPwdCount;
    }
    if (0 != g_stSettings.m_iMasterLockMaxRetry && pItem->m_iBadPwdCount >= g_stSettings.m_iMasterLockMaxRetry)
    {
      // user previously exhausted all retries, show access denied error
      CGUIDialogOK::ShowAndGetInput(12345, 12346, 0, 0);
      return false;
    }
    
    //
    if(g_guiSettings.GetBool("Masterlock.MasterUser") && (g_guiSettings.GetInt("Masterlock.Mastermode") > 0) )// Check if we are the MasterUser!
    {
      if(MasterUser()) iResult = 0; 
      else iResult = -1;
    }
    else
    {
    //
      // show the appropriate lock dialog
      CStdStringW strHeading = L""; /* CShares don't have this attribute if (pItem->m_bIsFolder) strHeading = g_localizeStrings.Get(12325); else  */
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
        return true;
        break;
      }
    //
    }
    //
    switch (iResult)
    {
    case -1:
      {
        // user canceled out
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
        if (0 != g_stSettings.m_iMasterLockMaxRetry)  pItem->m_iBadPwdCount++;
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
bool CGUIPassword::IsMasterLockUnlocked(bool bPromptUser)
{
  // \brief Verify whether Master Lock is currently unlocked.
  // \param bPromptUser If set to true and masterlock is locked, show Master Lock dialog to user for verification.
  // \return true if masterlock is unlocked and/or if user enters correct mastercode. false if not unlocked.
  if ((LOCK_MODE_EVERYONE < g_stSettings.m_iMasterLockMode) && !bPromptUser)
    // not unlocked, but calling code doesn't want to prompt user
    return false;

  if (1 > g_stSettings.m_iMasterLockMode)
  {  // already unlocked, don't prompt user for anything
    g_application.m_bMasterLockPreviouslyEntered = true;
    return true;
  }

  if(!g_guiSettings.GetBool("Masterlock.MasterUser"))
  {
    if (0 < g_stSettings.m_iMasterLockMaxRetry && 1 > g_application.m_iMasterLockRetriesRemaining)
    {
      // user has previously exhausted all their attempts, so don't prompt them again
      UpdateMasterLockRetryCount(false);
      return false;
    }
  }
  
  // Are we in MasterUser Mode ?
  if (g_application.m_bMasterLockOverridesLocalPasswords) return true; 

  int iVerifyPasswordResult = -1;
  CStdStringW strHeading = g_localizeStrings.Get(12324);
  switch (g_stSettings.m_iMasterLockMode)
  {
  case LOCK_MODE_NUMERIC:
    iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(g_stSettings.m_masterLockCode, strHeading, 0);
    break;
  case LOCK_MODE_GAMEPAD:
    iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(g_stSettings.m_masterLockCode, strHeading, 0);
    break;
  case LOCK_MODE_QWERTY:
    iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword(g_stSettings.m_masterLockCode, strHeading, 0);
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
  if ((g_application.m_MasterUserModeCounter == 0) && (g_guiSettings.GetBool("Masterlock.MasterUser")))
    {
      if (CGUIDialogYesNo::ShowAndGetInput(12324, 12354, 0, 0)) // remove all share lock
      {
        g_application.m_bMasterLockOverridesLocalPasswords = true;
        /*
        if (CGUIDialogYesNo::ShowAndGetInput(12324, 12349, 0, 0)) // remove Temp. MasterCode stuff! [will be defulat after reboot]
        {
          g_stSettings.m_iMasterLockMode = g_stSettings.m_iMasterLockMode * -1;
        }
        */
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
  if ((LOCK_MODE_EVERYONE < g_stSettings.m_iMasterLockMode) && !bPromptUser)
    return false;

  if (g_stSettings.m_iMasterLockMode  < 1 )
  { 
    g_application.m_bMasterLockPreviouslyEntered = true;
    return true;
  }

  if (g_stSettings.m_iMasterLockMaxRetry > 0 && g_application.m_iMasterLockRetriesRemaining < 1 )
  {
    UpdateMasterLockRetryCount(false);
    return false;
  }

  int iVerifyPasswordResult = -1;
  CStdStringW strHeading = g_localizeStrings.Get(12324);
  switch (g_stSettings.m_iMasterLockMode)
  {
  case LOCK_MODE_NUMERIC:
    iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(g_stSettings.m_masterLockCode, strHeading, 0);
    break;
  case LOCK_MODE_GAMEPAD:
    iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(g_stSettings.m_masterLockCode, strHeading, 0);
    break;
  case LOCK_MODE_QWERTY:
    iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword(g_stSettings.m_masterLockCode, strHeading, 0);
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
    if (0 < g_stSettings.m_iMasterLockMaxRetry)
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
        if (1 == g_stSettings.m_iMasterLockEnableShutdown)
        {
          // Shutdown enabled, tell the user we're shutting off
          CGUIDialogOK::ShowAndGetInput(12345, 12346, 12347, 0);
#ifndef _DEBUG  // don't actually shut off if debug build, it hangs VS for a long time
          g_application.Stop();
          Sleep(200);
          XKUtils::XBOXPowerOff();
#endif
          return ;
        }
        // Tell the user they ran out of retry attempts
        CGUIDialogOK::ShowAndGetInput(12345, 12346, 0, 0);
        return ;
      }
    }
    // Tell user they entered a bad password
    CStdStringW dlgLine1 = "";
    if (0 < g_application.m_iMasterLockRetriesRemaining)
      dlgLine1.Format(L"%d %s", g_application.m_iMasterLockRetriesRemaining, g_localizeStrings.Get(12343));
    CGUIDialogOK *dialog = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
    if (dialog)
    {
      dialog->SetHeading(12324);
      dialog->SetLine(0, 12345);
      dialog->SetLine(1, dlgLine1);
      dialog->SetLine(2, 0);
      dialog->DoModal(m_gWindowManager.GetActiveWindow());
    }
  }
  else
  {
    // user entered correct mastercode, reset retries to max allowed
    g_application.m_iMasterLockRetriesRemaining = g_stSettings.m_iMasterLockMaxRetry;
  }
  return ;
}

bool CGUIPassword::IsConfirmed() const
{ return m_bConfirmed; }
bool CGUIPassword::IsCanceled() const
{ return m_bCanceled; }
void CGUIPassword::SetSMBShare(const CStdString &strPath)
{
  m_SMBShare = strPath;
  m_bCanceled = false;
}
CStdString CGUIPassword::GetSMBShare()
{ return m_SMBShare;  }
void CGUIPassword::GetSMBShareUserPassword()
{
  CURL url(m_SMBShare);
  CStdString passcode;
  CStdString username = url.GetUserName();
  CStdString outusername = username;
  CStdString share;
  url.GetURLWithoutUserDetails(share);

  CStdStringW header;
  header.Format(L"%s %s", g_localizeStrings.Get(14062).c_str(), CStdStringW(share.c_str()).c_str());

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
    switch (g_stSettings.m_iMasterLockMode)
    { // Prompt user for mastercode
      case LOCK_MODE_NUMERIC:
        iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(g_stSettings.m_masterLockCode, strHeader, 0);
        break;
      case LOCK_MODE_GAMEPAD:
        iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(g_stSettings.m_masterLockCode, strHeader, 0);
        break;
      case LOCK_MODE_QWERTY:
        iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword(g_stSettings.m_masterLockCode, strHeader, 0);
        break;
    }
    if (iVerifyPasswordResult != 0 )
    {
      CStdString strLabel,strLabel1;
      strLabel1 = g_localizeStrings.Get(12343);
      int iLeft = g_application.m_iMasterLockRetriesRemaining-i;
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
    {i=g_application.m_iMasterLockRetriesRemaining;}
  }
  if (iVerifyPasswordResult == 0)
  {
    g_stSettings.m_iMasterLockStartupLock = 0;
    g_application.m_iMasterLockRetriesRemaining = g_stSettings.m_iMasterLockMaxRetry;
      return true;  // OK The MasterCode Accepted! XBMC Can Run!
  }
  else 
  {
    g_applicationMessenger.Shutdown(); // Turn off the box
    return false;
  }
}
bool CGUIPassword::MasterUser()   // GeminiServer
{
  return (IsMasterLockUnlocked(true));
  
  /*
  if(g_guiSettings.GetInt("Masterlock.Mastermode") != 0)
  {
    if(g_guiSettings.GetBool("Masterlock.MasterUser")) // default: false
    {
      int iVerifyPasswordResult = -1;
      CStdString strHeader = g_localizeStrings.Get(12324);
      if (!g_application.m_bMasterLockOverridesLocalPasswords)
      {
        
        for (int i=0; i <= g_application.m_iMasterLockRetriesRemaining; i++)
        {
          switch (g_stSettings.m_iMasterLockMode)
          { // Prompt user for mastercode
            case LOCK_MODE_NUMERIC:
              iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(g_stSettings.m_masterLockCode, strHeader, 0);
              break;
            case LOCK_MODE_GAMEPAD:
              iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(g_stSettings.m_masterLockCode, strHeader, 0);
              break;
            case LOCK_MODE_QWERTY:
              iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword(g_stSettings.m_masterLockCode, strHeader, 0);
              break;
          }
        }
        if (iVerifyPasswordResult == 0)
        {
          if (g_application.m_MasterUserModeCounter == 0)
          {
            UpdateMasterLockRetryCount(true);
            if (!g_application.m_bMasterLockPreviouslyEntered)
            {
              g_application.m_bMasterLockPreviouslyEntered = true;
              if (CGUIDialogYesNo::ShowAndGetInput(12324, 12354, 0, 0))
              {
                g_application.m_bMasterLockOverridesLocalPasswords = true;
                return true;
              }
            }
          }
          else g_application.m_MasterUserModeCounter = g_application.m_MasterUserModeCounter -1;
          return true;
        }
        else return false; //The MasterCode is wrong!
      }
      else return true;  // OK The MasterCode Accepted!
    }
  }
  return false; //The MasterUser is Disabled!
  */
}

bool CGUIPassword::CheckMenuLock(int iWindowID)
{
  bool bCheckPW         = false;
  int iLockFilemanager  = g_stSettings.m_iMasterLockFilemanager;
  int iLockSettings     = g_stSettings.m_iMasterLockSettings;
  int iLockHomeMedia    = g_stSettings.m_iMasterLockHomeMedia;
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
  //Now let's check the PW if we need!
  if (bCheckPW)
  {
    //if(g_guiSettings.GetBool("Masterlock.MasterUser")) return MasterUser(); // Check if we are the MasterUser!
    //return IsMasterLockLocked(true);
    return MasterUser();
  }
  else return true;
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
