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

#include "GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"

#define CONTROL_NO_BUTTON 10
#define CONTROL_YES_BUTTON 11

CGUIDialogYesNo::CGUIDialogYesNo(int overrideId /* = -1 */)
    : CGUIDialogBoxBase(overrideId == -1 ? WINDOW_DIALOG_YES_NO : overrideId, "DialogYesNo.xml")
{
  m_bConfirmed = false;
}

CGUIDialogYesNo::~CGUIDialogYesNo()
{
}

bool CGUIDialogYesNo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      int iAction = message.GetParam1();
      if (1 || ACTION_SELECT_ITEM == iAction)
      {
        if (iControl == CONTROL_NO_BUTTON)
        {
          m_bConfirmed = false;
          Close();
          return true;
        }
        if (iControl == CONTROL_YES_BUTTON)
        {
          m_bConfirmed = true;
          Close();
          return true;
        }
      }
    }
    break;
  }
  return CGUIDialogBoxBase::OnMessage(message);
}

bool CGUIDialogYesNo::OnBack(int actionID)
{
  m_bCanceled = true;
  m_bConfirmed = false;
  return CGUIDialogBoxBase::OnBack(actionID);
}

// \brief Show CGUIDialogYesNo dialog, then wait for user to dismiss it.
// \return true if user selects Yes, false if user selects No.
bool CGUIDialogYesNo::ShowAndGetInput(int heading, int line0, int line1, int line2, bool& bCanceled)
{
  return ShowAndGetInput(heading,line0,line1,line2,-1,-1,bCanceled);
}

bool CGUIDialogYesNo::ShowAndGetInput(int heading, int line0, int line1, int line2, int iNoLabel, int iYesLabel)
{
  bool bDummy;
  return ShowAndGetInput(heading,line0,line1,line2,iNoLabel,iYesLabel,bDummy);
}

bool CGUIDialogYesNo::ShowAndGetInput(int heading, int line0, int line1, int line2, int iNoLabel, int iYesLabel, bool& bCanceled, unsigned int autoCloseTime)
{
  CGUIDialogYesNo *dialog = (CGUIDialogYesNo *)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!dialog) return false;
  dialog->SetHeading(heading);
  dialog->SetLine(0, line0);
  dialog->SetLine(1, line1);
  dialog->SetLine(2, line2);
  if (autoCloseTime)
    dialog->SetAutoClose(autoCloseTime);
  if (iNoLabel != -1)
    dialog->SetChoice(0,iNoLabel);
  else
    dialog->SetChoice(0,106);
  if (iYesLabel != -1)
    dialog->SetChoice(1,iYesLabel);
  else
    dialog->SetChoice(1,107);
  dialog->m_bCanceled = false;
  dialog->DoModal();
  bCanceled = dialog->m_bCanceled;
  return (dialog->IsConfirmed()) ? true : false;
}

bool CGUIDialogYesNo::ShowAndGetInput(const CStdString& heading, const CStdString& line0, const CStdString& line1, const CStdString& line2, const CStdString& noLabel, const CStdString& yesLabel)
{
  bool bDummy;
  return ShowAndGetInput(heading,line0,line1,line2,bDummy,noLabel,yesLabel);
}

bool CGUIDialogYesNo::ShowAndGetInput(const CStdString& heading, const CStdString& line0, const CStdString& line1, const CStdString& line2, bool& bCanceled, const CStdString& noLabel, const CStdString& yesLabel)
{
  CGUIDialogYesNo *dialog = (CGUIDialogYesNo *)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!dialog) return false;
  dialog->SetHeading(heading);
  dialog->SetLine(0, line0);
  dialog->SetLine(1, line1);
  dialog->SetLine(2, line2);
  dialog->m_bCanceled = false;
  if (!noLabel.IsEmpty())
    dialog->SetChoice(0,noLabel);
  else
    dialog->SetChoice(0,106);
  if (!yesLabel.IsEmpty())
    dialog->SetChoice(1,yesLabel);
  else
    dialog->SetChoice(1,107);
  dialog->DoModal();
  bCanceled = dialog->m_bCanceled;
  return (dialog->IsConfirmed()) ? true : false;
}

int CGUIDialogYesNo::GetDefaultLabelID(int controlId) const
{
  if (controlId == CONTROL_NO_BUTTON)
    return 106;
  else if (controlId == CONTROL_YES_BUTTON)
    return 107;
  return CGUIDialogBoxBase::GetDefaultLabelID(controlId);
}
