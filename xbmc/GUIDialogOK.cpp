#include "stdafx.h"
#include "GUIDialogOK.h"

#define ID_BUTTON_OK   10

CGUIDialogOK::CGUIDialogOK(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_OK, "DialogOK.xml")
{
}

CGUIDialogOK::~CGUIDialogOK(void)
{}

bool CGUIDialogOK::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == ID_BUTTON_OK)
      {
        m_bConfirmed = true;
        Close();
        return true;
      }
    }
    break;
  }
  return CGUIDialogBoxBase::OnMessage(message);
}

// \brief Show CGUIDialogOK dialog, then wait for user to dismiss it.
void CGUIDialogOK::ShowAndGetInput(int heading, int line0, int line1, int line2)
{
  CGUIDialogOK *dialog = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  if (!dialog) return;
  dialog->SetHeading( heading );
  dialog->SetLine( 0, line0 );
  dialog->SetLine( 1, line1 );
  dialog->SetLine( 2, line2 );
  dialog->DoModal(m_gWindowManager.GetActiveWindow());
  return ;
}
