
#include "stdafx.h"
#include "GUIDialogContextMenu.h"
#include "GUIButtonControl.h"
#include "GUIWindowManager.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "GUIDialogPasswordNumeric.h"
#include "GUIDialogPasswordGamepad.h"
#include "Settings.h"
#include "localizeStrings.h"
#include "application.h"
#include "xbox/xkutils.h"

#define BACKGROUND_IMAGE 999
#define BACKGROUND_BOTTOM 998
#define BUTTON_TEMPLATE 1000

#define SPACE_BETWEEN_BUTTONS 2

CGUIDialogContextMenu::CGUIDialogContextMenu(void)
:CGUIDialog(0)
{
	m_iClickedButton = -1;
	m_iNumButtons = 0;
}

CGUIDialogContextMenu::~CGUIDialogContextMenu(void)
{
}

void CGUIDialogContextMenu::OnAction(const CAction &action)
{
	if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
  {
		Close();
		return;
  }
	CGUIDialog::OnAction(action);
}

bool CGUIDialogContextMenu::OnMessage(CGUIMessage &message)
{
	if (message.GetMessage() == GUI_MSG_CLICKED)
	{
		// someone has been clicked - deinit...
		m_iClickedButton = message.GetSenderId() - BUTTON_TEMPLATE;
		Close();
		return true;
	}
	return CGUIDialog::OnMessage(message);
}

void CGUIDialogContextMenu::OnInitWindow()
{	// disable the template button control
	CGUIControl *pControl = (CGUIControl *)GetControl(BUTTON_TEMPLATE);
	if (pControl)
	{
		pControl->SetVisible(false);
	}
	m_iClickedButton = -1;
	CGUIDialog::OnInitWindow();
}

void CGUIDialogContextMenu::ClearButtons()
{
	// destroy our buttons (if we have them from a previous viewing)
	for (int i=1; i<=m_iNumButtons; i++)
	{
		// get the button to remove...
		CGUIControl *pControl = (CGUIControl *)GetControl(BUTTON_TEMPLATE+i);
		if (pControl)
		{
			// remove the control from our list
			Remove(BUTTON_TEMPLATE+i);
			// kill the button
			delete pControl;
		}
	}
	m_iNumButtons = 0;
}

void CGUIDialogContextMenu::AddButton(int iLabel)
{
	AddButton(g_localizeStrings.Get(iLabel));
}

void CGUIDialogContextMenu::AddButton(const wstring &strLabel)
{
	// add a button to our control
	CGUIButtonControl *pButtonTemplate = (CGUIButtonControl *)GetControl(BUTTON_TEMPLATE);
	if (!pButtonTemplate) return;
	CGUIButtonControl *pButton = new CGUIButtonControl(*pButtonTemplate);
	if (!pButton) return;
	// set the button's ID and position
	m_iNumButtons++;
	DWORD dwID = BUTTON_TEMPLATE+m_iNumButtons;
	pButton->SetID(dwID);
	pButton->SetPosition(pButtonTemplate->GetXPosition(),(m_iNumButtons-1)*(pButtonTemplate->GetHeight()+SPACE_BETWEEN_BUTTONS));
	pButton->SetVisible(true);
	pButton->SetNavigation(dwID-1, dwID+1, dwID, dwID);
	pButton->SetText(strLabel);
	Add(pButton);
	// and update the size of our menu
	CGUIControl *pControl = (CGUIControl *)GetControl(BACKGROUND_IMAGE);
	if (pControl)
	{
		pControl->SetHeight(m_iNumButtons*(pButtonTemplate->GetHeight()+SPACE_BETWEEN_BUTTONS));
		CGUIControl *pControl2 = (CGUIControl *)GetControl(BACKGROUND_BOTTOM);
		if (pControl2)
		{
			pControl2->SetPosition(pControl2->GetXPosition(), pControl->GetYPosition()+pControl->GetHeight()); 
		}
	}
}

void CGUIDialogContextMenu::DoModal(DWORD dwParentId)
{
	// update the navigation of the first and last buttons
	CGUIControl *pControl = (CGUIControl *)GetControl(BUTTON_TEMPLATE+1);
	if (pControl)
	{
		pControl->SetNavigation(BUTTON_TEMPLATE+m_iNumButtons, pControl->GetControlIdDown(),
														pControl->GetControlIdLeft(), pControl->GetControlIdRight());
	}
	pControl = (CGUIControl *)GetControl(BUTTON_TEMPLATE+m_iNumButtons);
	if (pControl)
	{
		pControl->SetNavigation(pControl->GetControlIdUp(), BUTTON_TEMPLATE+1,
														pControl->GetControlIdLeft(), pControl->GetControlIdRight());
	}
	// update our default control
	if (m_dwDefaultFocusControlID <= BUTTON_TEMPLATE || m_dwDefaultFocusControlID > (DWORD)(BUTTON_TEMPLATE+m_iNumButtons))
		m_dwDefaultFocusControlID = BUTTON_TEMPLATE+1;
	// check the default control has focus...
	while (m_dwDefaultFocusControlID <= (DWORD)(BUTTON_TEMPLATE+m_iNumButtons) && !(GetControl(m_dwDefaultFocusControlID)->CanFocus()))
		m_dwDefaultFocusControlID++;
	CGUIDialog::DoModal(dwParentId);
}

int CGUIDialogContextMenu::GetButton()
{
	return m_iClickedButton;
}

DWORD CGUIDialogContextMenu::GetHeight()
{
	CGUIControl *pControl = (CGUIControl *)GetControl(BACKGROUND_IMAGE);
	if (pControl)
		return pControl->GetHeight();
	else
		return CGUIDialog::GetHeight();
}

DWORD CGUIDialogContextMenu::GetWidth()
{
	CGUIControl *pControl = (CGUIControl *)GetControl(BACKGROUND_IMAGE);
	if (pControl)
		return pControl->GetWidth();
	else
		return CGUIDialog::GetWidth();
}

void CGUIDialogContextMenu::EnableButton(int iButton, bool bEnable)
{
	CGUIControl *pControl = (CGUIControl *)GetControl(BUTTON_TEMPLATE+iButton);
	if (pControl) pControl->SetEnabled(bEnable);
}

bool CGUIDialogContextMenu::BookmarksMenu(const CStdString &strType, const CStdString &strLabel, const CStdString &strPath, int iLockMode, bool bMaxRetryExceeded, int iPosX, int iPosY)
{
	// popup the context menu
	CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
	if (pMenu)
	{
		// clean any buttons not needed
		pMenu->ClearButtons();
		// add the needed buttons
		pMenu->AddButton(118);	// 1: Rename
		pMenu->AddButton(748);	// 2: Edit Path
		pMenu->AddButton(117);	// 3: Delete
		pMenu->AddButton(749);	// 4: Add Share
    if (0 == iLockMode)     // The following if statement should always be the *last* button
      pMenu->AddButton(12332);  // 5: Lock Share
    else if (0 < iLockMode && bMaxRetryExceeded)
      pMenu->AddButton(12334);  // 5: Reset Share Lock
    else if (0 > iLockMode && !bMaxRetryExceeded)
      pMenu->AddButton(12335);  // 5: Remove Share Lock

		// set the correct position
		pMenu->SetPosition(iPosX-pMenu->GetWidth()/2, iPosY-pMenu->GetHeight()/2);
		pMenu->DoModal(m_gWindowManager.GetActiveWindow());
		switch (pMenu->GetButton())
		{
		case 1:	 // 1: Rename
			{
				if (!CheckMasterCode(iLockMode)) return false;
        // rename share
				CStdString strNewLabel = strLabel;
				CStdString strHeading = g_localizeStrings.Get(753);
				if (CGUIDialogKeyboard::ShowAndGetInput(strNewLabel, strHeading, false))
				{
					g_settings.UpdateBookmark(strType, strLabel, "name", strNewLabel);
					return true;
				}
			}
			break;
		case 2:	 // 2: Edit Path
			{
				if (!CheckMasterCode(iLockMode)) return false;
        // edit path
				CStdString strNewPath = strPath;
				CStdString strHeading = g_localizeStrings.Get(752);
				if (CGUIDialogKeyboard::ShowAndGetInput(strNewPath, strHeading, false))
				{
					g_settings.UpdateBookmark(strType, strLabel, "path", strNewPath);
					return true;
				}
			}
			break;
		case 3:	 // 3: Delete
			{
				if (!CheckMasterCode(iLockMode)) return false;
				// prompt user if they want to really delete the bookmark
				CGUIDialogYesNo *pDlg = (CGUIDialogYesNo *)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
				if (pDlg)
				{
					pDlg->SetHeading(751);
					pDlg->SetLine(0, "");
					pDlg->SetLine(1,750);
					pDlg->SetLine(2, "");
					pDlg->DoModal(m_gWindowManager.GetActiveWindow());
					if (pDlg->IsConfirmed())
					{
						// delete this share
						g_settings.DeleteBookmark(strType, strLabel, strPath);
						return true;
					}
				}
			}
			break;
		case 4:	 // 4: Add Share
			{
				if (!CheckMasterCode(iLockMode)) return false;
        // Add new share
				CStdString strNewPath;
				CStdString strHeading = g_localizeStrings.Get(752);	// Share Path
				if (CGUIDialogKeyboard::ShowAndGetInput(strNewPath, strHeading, false))
				{	// got a valid path
					CStdString strNewName;
					strHeading = g_localizeStrings.Get(753);	// Share Name
					if (CGUIDialogKeyboard::ShowAndGetInput(strNewName, strHeading, false))
					{
						g_settings.AddBookmark(strType, strNewName, strNewPath);
						return true;
					}
				}
			}
			break;
    case 5:  // 5: Lock Share
      {	// Share Lock settings
        // Prompt user for mastercode
        CGUIDialogPasswordNumeric *pDialog = (CGUIDialogPasswordNumeric *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PASSWORD_NUMERIC);
        pDialog->m_strPassword = g_stSettings.szMasterLockCode;
        pDialog->SetHeading( 12324 );
        pDialog->SetLine( 0, 12327 );
        pDialog->SetLine( 1, 12329 );
        pDialog->SetLine( 2, L"" );
        pDialog->DoModal(m_gWindowManager.GetActiveWindow());
        if (pDialog->IsConfirmed() && !pDialog->IsCanceled())
        {
          // mastercode entry succeeded
          g_application.m_iMasterCodeRetriesRemaining = g_stSettings.m_iMasterLockMaxRetry;

          if (0 == iLockMode)
          {
            // popup the context menu
            CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
            if (pMenu)
            {
              // clean any buttons not needed
              pMenu->ClearButtons();
              // add the needed buttons
              pMenu->AddButton(12337);	// 1: Numeric Password
              pMenu->AddButton(12338);	// 2: XBOX gamepad button combo
              pMenu->AddButton(12339);	// 3: Full-text Password

              // set the correct position
              pMenu->SetPosition(iPosX-pMenu->GetWidth()/2, iPosY-pMenu->GetHeight()/2);
              pMenu->DoModal(m_gWindowManager.GetActiveWindow());
              switch (pMenu->GetButton())
              {
              case 1:	// 1: Numeric Password
                {
                  // Prompt user for password input
                  CStdString strNewPassword = "";
                  CGUIDialogPasswordNumeric *pDialog = (CGUIDialogPasswordNumeric *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PASSWORD_NUMERIC);
                  pDialog->m_strPassword = "";  // starting out with blank password
                  pDialog->SetHeading( 12340 );
                  pDialog->SetLine( 0, 12328 );
                  pDialog->SetLine( 1, 12329 );
                  pDialog->SetLine( 2, L"" );
                  pDialog->m_bUserInputCleanup = false;
                  pDialog->DoModal(m_gWindowManager.GetActiveWindow());

                  if (pDialog->IsConfirmed() && !pDialog->IsCanceled())
                  {
                      // user entered a blank password
                      return false;
                  }
                  else if (!pDialog->IsConfirmed() && pDialog->IsCanceled())
                  {
                      // user canceled out
                      return false;
                  }
                  // Prompt again for password input, this time sending previous input as the password to verify
                  strNewPassword = pDialog->m_strUserInput;
                  pDialog->m_strPassword = strNewPassword;
                  pDialog->m_strUserInput = "";
                  pDialog->SetHeading( 12341 );
                  pDialog->SetLine( 0, 12328 );
                  pDialog->SetLine( 1, 12329 );
                  pDialog->SetLine( 2, L"" );
                  pDialog->DoModal(m_gWindowManager.GetActiveWindow());

                  if (pDialog->IsConfirmed() && !pDialog->IsCanceled())
                  {
                      // password re-entry succeeded
                      g_settings.UpdateBookmark(strType, strLabel, "lockmode", "1");
                      g_settings.UpdateBookmark(strType, strLabel, "lockcode", strNewPassword);
                      g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", "0");
                      return true;
                  }
 									return false;
                }
                break;
	            case 2:	// 2: XBOX gamepad button combo
                {
                  // Prompt user for password input
                  CStdString strNewPassword = "";
                  CGUIDialogPasswordGamepad *pDialog = (CGUIDialogPasswordGamepad *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PASSWORD_GAMEPAD);
                  pDialog->m_strPassword = "";  // starting out with blank password
                  pDialog->SetHeading( 12340 );
                  pDialog->SetLine( 0, 12330 );
                  pDialog->SetLine( 1, 12331 );
                  pDialog->SetLine( 2, L"" );
                  pDialog->m_bUserInputCleanup = false;
                  pDialog->DoModal(m_gWindowManager.GetActiveWindow());

                  if (pDialog->IsConfirmed() && !pDialog->IsCanceled())
                  {
                    // user entered a blank password
                    return false;
                  }
                  else if (!pDialog->IsConfirmed() && pDialog->IsCanceled())
                  {
                    // user canceled out
                    return false;
                  }
                  // Prompt again for password input, this time sending previous input as the password to verify
                  strNewPassword = pDialog->m_strUserInput;
                  pDialog->m_strPassword = strNewPassword;
                  pDialog->m_strUserInput = "";
                  pDialog->SetHeading( 12341 );
                  pDialog->SetLine( 0, 12330 );
                  pDialog->SetLine( 1, 12331 );
                  pDialog->SetLine( 2, L"" );
                  pDialog->DoModal(m_gWindowManager.GetActiveWindow());

                  if (pDialog->IsConfirmed() && !pDialog->IsCanceled())
                  {
                    // password re-entry succeeded
                    g_settings.UpdateBookmark(strType, strLabel, "lockmode", "2");
                    g_settings.UpdateBookmark(strType, strLabel, "lockcode", strNewPassword);
                    g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", "0");
                    return true;
                  }
 									return false;
                }
                break;
	            case 3:	// 3: Full-text Password
                {
                  // Prompt user for password input
                  CStdString strNewPassword = "";
                  CStdString strTextInput = "";
                  bool bCanceled = false;
                  bool bConfirmed = false;
                  CStdString strHeading = g_localizeStrings.Get(12340);
                  bCanceled = !CGUIDialogKeyboard::ShowAndGetInput(strNewPassword, strHeading, false);

                  if (bCanceled || "" == strNewPassword)
                  {
                    // user canceled out or entered a blank password
                    return false;
                  }
                  
                  // Re-enter password input
                  strHeading = g_localizeStrings.Get(12341);
                  bCanceled = !CGUIDialogKeyboard::ShowAndGetInput(strTextInput, strHeading, false);
                  bConfirmed = (!bCanceled && (strTextInput == strNewPassword));

                  if (bConfirmed && !bCanceled)
                  {
                    // password re-entry succeeded
                    g_settings.UpdateBookmark(strType, strLabel, "lockmode", "3");
                    g_settings.UpdateBookmark(strType, strLabel, "lockcode", strNewPassword);
                    g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", "0");
                    return true;
                  }
									return false;
                }
                break;
              }
            }
          }
          else if (0 < iLockMode && bMaxRetryExceeded)  // 5: Reset Share Lock
          {
            g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", "0");
            return true;
          }
          else if (0 > iLockMode && !bMaxRetryExceeded)  // 5: Remove Share Lock
          {
            g_settings.UpdateBookmark(strType, strLabel, "lockmode", "0");
            g_settings.UpdateBookmark(strType, strLabel, "lockcode", "-");
            g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", "0");
            return true;
          }
          else  // this should never happen, but if it does, don't perform any action                            
            return false;

          // reset bookmark badpwdcount value to 0
          {
            g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", "0");
            return true;
          }
        }
        if (!pDialog->IsConfirmed() && pDialog->IsCanceled())
        {
          // user canceled out
          return false;
        }
        else
        {
          // mastercode entry failed
          bool bExceededMasterRetries = false;
          if (0 != g_stSettings.m_iMasterLockMaxRetry && g_stSettings.m_iMasterLockEnableShutdown)
          {
            g_application.m_iMasterCodeRetriesRemaining--;
            bExceededMasterRetries = (!(0 < g_application.m_iMasterCodeRetriesRemaining));
          }
          CGUIDialogOK* pDialogOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
          pDialogOK->SetHeading( 12324 );
          pDialogOK->SetLine( 0, 12345 );
          if (bExceededMasterRetries)
            pDialogOK->SetLine( 1, 12347 );
          else
            pDialogOK->SetLine( 1, L"" );
          pDialogOK->SetLine( 2, L"" );
  				pDialogOK->DoModal(m_gWindowManager.GetActiveWindow());
          if (bExceededMasterRetries)
          {
						g_application.Stop();
	          Sleep(200);
	          XKUtils::XBOXPowerOff();
	          return false;
          }
        }
      }
      break;
		}
	}
	return false;
}

bool CGUIDialogContextMenu::CheckMasterCode(int iLockMode)
{
	// prompt user for mastercode if the bookmark is locked
	// or if m_iMasterLockProtectShares is enabled
	if (0 < iLockMode || 0 != g_stSettings.m_iMasterLockProtectShares)
	{
		// Prompt user for mastercode
		CGUIDialogPasswordNumeric *pDialog = (CGUIDialogPasswordNumeric *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PASSWORD_NUMERIC);
		pDialog->m_strPassword = g_stSettings.szMasterLockCode;
		pDialog->SetHeading( 12324 );
		pDialog->SetLine( 0, 12327 );
		pDialog->SetLine( 1, 12329 );
		pDialog->SetLine( 2, L"" );
		pDialog->DoModal(m_gWindowManager.GetActiveWindow());
		if (!pDialog->IsConfirmed() || pDialog->IsCanceled())
		{
			if (pDialog->IsCanceled()) // user canceled out
				return false;

			// mastercode entry failed
			bool bExceededMasterRetries = false;
			if (0 != g_stSettings.m_iMasterLockMaxRetry && g_stSettings.m_iMasterLockEnableShutdown)
			{
				g_application.m_iMasterCodeRetriesRemaining--;
				bExceededMasterRetries = (!(0 < g_application.m_iMasterCodeRetriesRemaining));
			}
			CGUIDialogOK* pDialogOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
			pDialogOK->SetHeading( 12324 );
			pDialogOK->SetLine( 0, 12345 );
			if (bExceededMasterRetries)
				pDialogOK->SetLine( 1, 12347 );
			else
				pDialogOK->SetLine( 1, L"" );
			pDialogOK->SetLine( 2, L"" );
			pDialogOK->DoModal(m_gWindowManager.GetActiveWindow());
			if (bExceededMasterRetries)
			{
				g_application.Stop();
				Sleep(200);
				XKUtils::XBOXPowerOff();
			}
			return false;
		}
		// mastercode entry succeeded
		g_application.m_iMasterCodeRetriesRemaining = g_stSettings.m_iMasterLockMaxRetry;
		return true;
	}
	return false;
}