/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogButtonMenu.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIButtonControl.h"


#define CONTROL_BUTTON_LABEL  3100

CGUIDialogButtonMenu::CGUIDialogButtonMenu(int id, const CStdString &xmlFile)
: CGUIDialog(id, xmlFile)
{
  m_loadType = KEEP_IN_MEMORY;
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

void CGUIDialogButtonMenu::FrameMove()
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
  CGUIDialog::FrameMove();
}
