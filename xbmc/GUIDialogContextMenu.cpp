
#include "stdafx.h"
#include "GUIDialogContextMenu.h"
#include "GUIButtonControl.h"
#include "GUIWindowManager.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogYesNo.h"
#include "Settings.h"
#include "localizeStrings.h"

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

bool CGUIDialogContextMenu::BookmarksMenu(const CStdString &strType, const CStdString &strLabel, const CStdString &strPath, int iPosX, int iPosY)
{
	// popup the context menu
	CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
	if (pMenu)
	{
		// clean any buttons not needed
		pMenu->ClearButtons();
		// add the needed buttons
		pMenu->AddButton(118);	// Rename
		pMenu->AddButton(748);	// Edit Path
		pMenu->AddButton(117);	// Delete
		pMenu->AddButton(749);	// Add Share
		// set the correct position
		pMenu->SetPosition(iPosX-pMenu->GetWidth()/2, iPosY-pMenu->GetHeight()/2);
		pMenu->DoModal(m_gWindowManager.GetActiveWindow());
		switch (pMenu->GetButton())
		{
		case 1:
			{
				CStdString strNewLabel = strLabel;
				CStdString strHeading = g_localizeStrings.Get(753);
				if (CGUIDialogKeyboard::ShowAndGetInput(strNewLabel, strHeading, false))
				{
					g_settings.UpdateBookmark(strType, strLabel, strNewLabel, strPath);
					return true;
				}
			}
			break;
		case 2:
			{
				CStdString strNewPath = strPath;
				CStdString strHeading = g_localizeStrings.Get(752);
				if (CGUIDialogKeyboard::ShowAndGetInput(strNewPath, strHeading, false))
				{
					g_settings.UpdateBookmark(strType, strLabel, strLabel, strNewPath);
					return true;
				}
			}
			break;
		case 3:
			{
				// prompt user
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
		case 4:
			{	// Add new share
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
		}
	}
	return false;
}