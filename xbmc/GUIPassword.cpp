
#include "stdafx.h"
#include "guipassword.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizeStrings.h"
#include "application.h"
#include "GUIDialogOK.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogGamepad.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogOK.h"
#include "GUIDialogYesNo.h"
#include "xbox/xkutils.h"

CGUIPassword::CGUIPassword(void)
:CGUIDialog(0)
{
  m_bConfirmed=false;
  m_bCanceled=false;
  CStdStringW m_strUserInput = L"";
  CStdStringW m_strPassword = L"";
  int m_iRetries = 0;
  m_bUserInputCleanup = true;
}

CGUIPassword::~CGUIPassword(void)
{
}

/// \brief Tests if the user is allowed to access the share folder
/// \param pItem The share folder item to access
/// \param strType The type of share being accessed, e.g. "music", "videos", etc. See CSettings::UpdateBookmark
/// \return If access is granted, returns \e true
bool CGUIPassword::IsItemUnlocked(CFileItem* pItem, const CStdString &strType)
{
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

// \brief Verify whether Master Lock is currently unlocked.
// \param bPromptUser If set to true and masterlock is locked, show Master Lock dialog to user for verification.
// \return true if masterlock is unlocked and/or if user enters correct mastercode. false if not unlocked.
bool CGUIPassword::IsMasterLockUnlocked(bool bPromptUser)
{
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
  default:  // must not be supported, treat as unlocked
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

// \brief Updates Master Lock status.
// \param bResetCount masterlock retry counter is zeroed if true, or incremented and displays an Access Denied dialog if false.
void CGUIPassword::UpdateMasterLockRetryCount(bool bResetCount)
{
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
          return;
        }
        // Tell the user they ran out of retry attempts
        CGUIDialogOK::ShowAndGetInput(L"12345", L"12346", L"", L"");
        return;
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
  return;
}

bool CGUIPassword::IsConfirmed() const
{
  return m_bConfirmed;
}

bool CGUIPassword::IsCanceled() const
{
  return m_bCanceled;
}
