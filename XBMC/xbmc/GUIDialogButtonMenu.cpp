/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogButtonMenu.h"
#include "GUILabelControl.h"
#include "GUIButtonControl.h"


#define CONTROL_BUTTON_LABEL  3100

CGUIDialogButtonMenu::CGUIDialogButtonMenu(void)
    : CGUIDialog(WINDOW_DIALOG_BUTTON_MENU, "DialogButtonMenu.xml")
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
    const CGUIControl *pControl = GetFocusedControl();
    if (pControl && (pControl->GetControlType() == CGUIControl::GUICONTROL_BUTTON || pControl->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON))
    {
      CGUIButtonControl *pButton = (CGUIButtonControl *)pControl;
      pLabel->SetLabel(pButton->GetLabel());
    }
  }
  CGUIDialog::Render();
}
