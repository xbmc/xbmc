#include "stdafx.h"
#include "GUIDialogYesNo.h"

CGUIDialogYesNo::CGUIDialogYesNo(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_YES_NO, "DialogYesNo.xml")
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

bool CGUIDialogYesNo::OnAction(const CAction& action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_bCanceled = true;
    m_bConfirmed = false;
    Close();
    return true;
  }

  return CGUIDialogBoxBase::OnAction(action);
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

bool CGUIDialogYesNo::ShowAndGetInput(int heading, int line0, int line1, int line2, int iNoLabel, int iYesLabel, bool& bCanceled)
{
  CGUIDialogYesNo *dialog = (CGUIDialogYesNo *)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!dialog) return false;
  dialog->SetHeading(heading);
  dialog->SetLine(0, line0);
  dialog->SetLine(1, line1);
  dialog->SetLine(2, line2);
  if (iNoLabel != -1)
    dialog->SetChoice(0,iNoLabel);
  if (iYesLabel != -1)
    dialog->SetChoice(1,iYesLabel);
  dialog->m_bCanceled = false;
  dialog->DoModal();
  bCanceled = dialog->m_bCanceled;
  return (dialog->IsConfirmed()) ? true : false;
}

bool CGUIDialogYesNo::ShowAndGetInput(const CStdString& heading, const CStdString& line0, const CStdString& line1, const CStdString& line2)
{
  bool bDummy;
  return ShowAndGetInput(heading,line0,line1,line2,bDummy);
}

bool CGUIDialogYesNo::ShowAndGetInput(const CStdString& heading, const CStdString& line0, const CStdString& line1, const CStdString& line2, bool& bCanceled)
{
  CGUIDialogYesNo *dialog = (CGUIDialogYesNo *)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!dialog) return false;
  dialog->SetHeading(heading);
  dialog->SetLine(0, line0);
  dialog->SetLine(1, line1);
  dialog->SetLine(2, line2);
  dialog->m_bCanceled = false;
  dialog->DoModal();
  bCanceled = dialog->m_bCanceled;
  return (dialog->IsConfirmed()) ? true : false;
}