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

// \brief Show CGUIDialogYesNo dialog, then wait for user to dismiss it.
// \return true if user selects Yes, false if user selects No.
bool CGUIDialogYesNo::ShowAndGetInput(int heading, int line0, int line1, int line2)
{
  CGUIDialogYesNo *dialog = (CGUIDialogYesNo *)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!dialog) return false;
  dialog->SetHeading(heading);
  dialog->SetLine(0, line0);
  dialog->SetLine(1, line1);
  dialog->SetLine(2, line2);
  dialog->DoModal(m_gWindowManager.GetActiveWindow());
  return (dialog->IsConfirmed()) ? true : false;
}
