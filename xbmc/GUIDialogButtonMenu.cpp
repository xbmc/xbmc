#include "stdafx.h"
#include "GUIDialogButtonMenu.h"
#include "GUILabelControl.h"
#include "GUIButtonControl.h"

#define CONTROL_BUTTON_LABEL		3100

CGUIDialogButtonMenu::CGUIDialogButtonMenu(void)
:CGUIDialog(0)
{	
}

CGUIDialogButtonMenu::~CGUIDialogButtonMenu(void)
{
}

void CGUIDialogButtonMenu::OnAction(const CAction &action)
{
	if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
  {
		Close();
		return;
  }
	CGUIDialog::OnAction(action);
}

bool CGUIDialogButtonMenu::OnMessage(CGUIMessage &message)
{
	bool bRet = CGUIDialog::OnMessage(message);
	if (message.GetMessage() == GUI_MSG_CLICKED)
	{
		// someone has been clicked - deinit...
		Close();
		return true;
	}
	return bRet;
}

void CGUIDialogButtonMenu::Render()
{
	// get the label control
	CGUILabelControl *pLabel = (CGUILabelControl *)GetControl(CONTROL_BUTTON_LABEL);
	if (!pLabel) return;
	// get the active window, and put it's label into the label control
	int iControl = GetFocusedControl();
	const CGUIControl *pControl = GetControl(iControl);
	if (pControl && pControl->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
	{
		CGUIButtonControl *pButton = (CGUIButtonControl *)pControl;
		CStdStringW strLabel = L"";
		if (pButton->GetLabel().size() > 0)
		{
			strLabel = pButton->GetLabel().c_str();
		}
		pLabel->SetLabel(strLabel);
	}
	CGUIDialog::Render();
}