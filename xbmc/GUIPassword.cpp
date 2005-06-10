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
    if (!(1 > pItem->m_iBadPwdCount))
    {
      iRetries = g_stSettings.m_iMasterLockMaxRetry - pItem->m_iBadPwdCount;
    }

    if (0 != g_stSettings.m_iMasterLockMaxRetry && pItem->m_iBadPwdCount >= g_stSettings.m_iMasterLockMaxRetry)
    {
      // user previously exhausted all retries, show access denied error
      CGUIDialogOK::ShowAndGetInput(L"12345", L"12346", L"", L"");
      return false;
    }

    // show the appropriate lock dialog
    CStdStringW strHeading = L"";
    if (pItem->m_bIsFolder)
      strHeading = L"12325";
    else
      strHeading = L"12348";

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
        if (0 != g_stSettings.m_iMasterLockMaxRetry)
          pItem->m_iBadPwdCount++;
        g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", itoa(pItem->m_iBadPwdCount, buffer, 10));
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
      CGUIDialogOK::ShowAndGetInput(L"12345", L"12346", L"", L"");
      return false;
    }

    // show the appropriate lock dialog
    CStdStringW strHeading = L"";
    /* CShares don't have this attribute
    if (pItem->m_bIsFolder)
      strHeading = L"12325";
    else
    */
    strHeading = L"12348";

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
        if (0 != g_stSettings.m_iMasterLockMaxRetry)
          pItem->m_iBadPwdCount++;
        g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", itoa(pItem->m_iBadPwdCount, buffer, 10));
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

  if (0 < g_stSettings.m_iMasterLockMaxRetry && 1 > g_application.m_iMasterLockRetriesRemaining)
  {
    // user has previously exhausted all their attempts, so don't prompt them again
    UpdateMasterLockRetryCount(false);
    return false;
  }

  int iVerifyPasswordResult = -1;
  switch (g_stSettings.m_iMasterLockMode)
  {
  case LOCK_MODE_NUMERIC:
    iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword((CStdStringW)g_stSettings.szMasterLockCode, L"12324", L"12327", L"12329", L"");
    break;
  case LOCK_MODE_GAMEPAD:
    iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword((CStdStringW)g_stSettings.szMasterLockCode, L"12324", L"12352", L"12331", L"");
    break;
  case LOCK_MODE_QWERTY:
    iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword((CStdStringW)g_stSettings.szMasterLockCode, L"12324", 0);
    break;
  default:   // must not be supported, treat as unlocked
    iVerifyPasswordResult = 0;
    break;
  }

  if (1 == iVerifyPasswordResult)
    UpdateMasterLockRetryCount(false);
  if (0 != iVerifyPasswordResult)
    return false;

  // user successfully entered mastercode
  UpdateMasterLockRetryCount(true);
  if (!g_application.m_bMasterLockPreviouslyEntered)
  {
    g_application.m_bMasterLockPreviouslyEntered = true;
    // TODO: change dlgLine1 and dlgLine2 to show 12350 and 12351. hook up Settings items to clear
    //       m_bMasterLockPreviouslyEntered and m_bMasterLockOverridesLocalPasswords, then prompt for mastercode
    if (CGUIDialogYesNo::ShowAndGetInput(L"12324", L"12349", L"", L""))
    {  // invert cached mastermode setting to make it stay unlocked (this doesn't persist through a reboot)
      g_stSettings.m_iMasterLockMode = g_stSettings.m_iMasterLockMode * -1;
      // TODO: change dlgLine1 and dlgLine2 to show 12350 and 12355 after the TODO: listed above is completed
      if (CGUIDialogYesNo::ShowAndGetInput(L"12324", L"12354", L"", L""))
        g_application.m_bMasterLockOverridesLocalPasswords = true;
    }
  }
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
  switch (g_stSettings.m_iMasterLockMode)
  {
  case LOCK_MODE_NUMERIC:
    iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword((CStdStringW)g_stSettings.szMasterLockCode, L"12324", L"12327", L"12329", L"");
    break;
  case LOCK_MODE_GAMEPAD:
    iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword((CStdStringW)g_stSettings.szMasterLockCode, L"12324", L"12352", L"12331", L"");
    break;
  case LOCK_MODE_QWERTY:
    iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword((CStdStringW)g_stSettings.szMasterLockCode, L"12324", 0);
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
          CGUIDialogOK::ShowAndGetInput(L"12345", L"12346", L"12347", L"");
#ifndef _DEBUG  // don't actually shut off if debug build, it hangs VS for a long time
          g_application.Stop();
          Sleep(200);
          XKUtils::XBOXPowerOff();
#endif
          return ;
        }
        // Tell the user they ran out of retry attempts
        CGUIDialogOK::ShowAndGetInput(L"12345", L"12346", L"", L"");
        return ;
      }
    }
    // Tell user they entered a bad password
    CStdStringW dlgLine1 = "";
    if (0 < g_application.m_iMasterLockRetriesRemaining)
      dlgLine1.Format(L"%d %s", g_application.m_iMasterLockRetriesRemaining, g_localizeStrings.Get(12343));
    CGUIDialogOK::ShowAndGetInput(L"12324", L"12345", dlgLine1, L"");
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
  if (g_application.m_iMasterLockRetriesRemaining == 0) g_application.m_iMasterLockRetriesRemaining = 1;
  for (int i=1; i <= g_application.m_iMasterLockRetriesRemaining; i++)
  {
    switch (g_stSettings.m_iMasterLockMode)
    { // Prompt user for mastercode
      case LOCK_MODE_NUMERIC:
        iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword((CStdStringW)g_stSettings.szMasterLockCode, L"12324", L"12327", L"12329", L"");
        break;
      case LOCK_MODE_GAMEPAD:
        iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword((CStdStringW)g_stSettings.szMasterLockCode, L"12324", L"12352", L"12331", L"");
        break;
      case LOCK_MODE_QWERTY:
        iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword((CStdStringW)g_stSettings.szMasterLockCode, L"12324", 0);
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