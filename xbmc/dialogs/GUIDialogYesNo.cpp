/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"

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
        if (iControl == 10)
        {
          m_bConfirmed = false;
          Close();
          return true;
        }
        if (iControl == 11)
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
  if (iYesLabel != -1)
    dialog->SetChoice(1,iYesLabel);
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
  if (!yesLabel.IsEmpty())
    dialog->SetChoice(1,yesLabel);
  dialog->DoModal();
  bCanceled = dialog->m_bCanceled;
  return (dialog->IsConfirmed()) ? true : false;
}

