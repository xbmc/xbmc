
#include "stdafx.h"
#include "GUIDialogContextMenu.h"
#include "GUIButtonControl.h"
#include "localizeStrings.h"

#define BACKGROUND_IMAGE 999
#define BUTTON_TEMPLATE 1000


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
	pButton->SetPosition(0,(m_iNumButtons-1)*pButtonTemplate->GetHeight());
	pButton->SetVisible(true);
	pButton->SetNavigation(dwID-1, dwID+1, dwID, dwID);
	pButton->SetText(strLabel);
	Add(pButton);
	// and update the size of our menu
	CGUIControl *pControl = (CGUIControl *)GetControl(BACKGROUND_IMAGE);
	if (pControl)
	{
		pControl->SetHeight(m_iNumButtons*pButtonTemplate->GetHeight());
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
