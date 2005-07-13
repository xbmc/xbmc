#include "stdafx.h"
#include "GUIDialogOK.h"
#include "application.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ID_BUTTON_NO   10
#define ID_BUTTON_YES  11

CGUIDialogOK::CGUIDialogOK(void)
    : CGUIDialog(0)
{
  m_bConfirmed = false;
}

CGUIDialogOK::~CGUIDialogOK(void)
{}

bool CGUIDialogOK::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      m_bConfirmed = false;
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iAction = message.GetParam1();
      if (1 || ACTION_SELECT_ITEM == iAction)
      {
        int iControl = message.GetSenderId();
        if ( GetControl(ID_BUTTON_YES) == NULL)
        {
          m_bConfirmed = true;
          Close();
          return true;
        }
        if (iControl == ID_BUTTON_NO)
        {
          m_bConfirmed = false;
          Close();
          return true;
        }
        if (iControl == ID_BUTTON_YES)
        {
          m_bConfirmed = true;
          Close();
          return true;
        }
      }
    }
    break;
  }
  //only allow messages from or for this dialog
  if (
    (message.GetSenderId() == GetID()) ||
    (message.GetControlId() == GetID()) ||
    (message.GetControlId() == 0)
  )
  {
    return CGUIDialog::OnMessage(message);
  }
  return false;
}

// \brief Show CGUIDialogOK dialog, then wait for user to dismiss it.
void CGUIDialogOK::ShowAndGetInput(int heading, int line0, int line1, int line2)
{
  CGUIDialogOK &dialog = g_application.m_guiDialogOK;
  dialog.SetHeading( heading );
  dialog.SetLine( 0, line0 );
  dialog.SetLine( 1, line1 );
  dialog.SetLine( 2, line2 );
  dialog.DoModal(m_gWindowManager.GetActiveWindow());
  return ;
}

bool CGUIDialogOK::IsConfirmed() const
{
  return m_bConfirmed;
}


void CGUIDialogOK::SetHeading(const wstring& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 1);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogOK::SetHeading(const string& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 1);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogOK::SetLine(int iLine, const wstring& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), iLine + 2);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogOK::SetLine(int iLine, const string& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), iLine + 2);
  msg.SetLabel(strLine);
  OnMessage(msg);
}
void CGUIDialogOK::SetHeading(int iString)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 1);
  if (iString)
    msg.SetLabel(iString);
  else
    msg.SetLabel("");
  OnMessage(msg);
}

void CGUIDialogOK::SetLine(int iLine, int iString)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), iLine + 2);
  if (iString)
    msg.SetLabel(iString);
  else
    msg.SetLabel("");
  OnMessage(msg);
}
