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

#include "GUIDialogOK.h"
#include "guilib/GUIWindowManager.h"

#define ID_BUTTON_OK   10

CGUIDialogOK::CGUIDialogOK(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_OK, "DialogOK.xml")
{
}

CGUIDialogOK::~CGUIDialogOK(void)
{}

bool CGUIDialogOK::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iControl = message.GetSenderId();
    if (iControl == ID_BUTTON_OK)
    {
      m_bConfirmed = true;
      Close();
      return true;
    }
  }
  return CGUIDialogBoxBase::OnMessage(message);
}

// \brief Show CGUIDialogOK dialog, then wait for user to dismiss it.
void CGUIDialogOK::ShowAndGetInput(const CVariant &heading, const CVariant &line0, const CVariant &line1, const CVariant &line2)
{
  CGUIDialogOK *dialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
  if (!dialog) 
    return;
  dialog->SetHeading(heading);
  dialog->SetLine(0, line0);
  dialog->SetLine(1, line1);
  dialog->SetLine(2, line2);
  dialog->DoModal();
}

int CGUIDialogOK::GetDefaultLabelID(int controlId) const
{
  if (controlId == ID_BUTTON_OK)
    return 186;
  return CGUIDialogBoxBase::GetDefaultLabelID(controlId);
}
