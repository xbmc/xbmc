#include "stdafx.h"
#include "GUIDialogButtonMenu.h"
#include "GUILabelControl.h"
#include "GUIButtonControl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define CONTROL_BUTTON_LABEL  3100

CGUIDialogButtonMenu::CGUIDialogButtonMenu(void)
    : CGUIDialog(0)
{
}

CGUIDialogButtonMenu::~CGUIDialogButtonMenu(void)
{}

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
  if (pLabel)
  {
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
  }
  CGUIDialog::Render();
}
