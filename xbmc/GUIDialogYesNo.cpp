#include "stdafx.h"
#include "GUIDialogYesNo.h"
#include "application.h"
#include "util.h"

CGUIDialogYesNo::CGUIDialogYesNo(void)
    : CGUIDialog(0)
{
  m_bConfirmed = false;
}

CGUIDialogYesNo::~CGUIDialogYesNo(void)
{}

bool CGUIDialogYesNo::OnMessage(CGUIMessage& message)
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
  return CGUIDialog::OnMessage(message);
}

// \brief Show CGUIDialogYesNo dialog, then wait for user to dismiss it.
// \return true if user selects Yes, false if user selects No.
bool CGUIDialogYesNo::ShowAndGetInput(int heading, int line0, int line1, int line2)
{
  CGUIDialogYesNo &dialog = g_application.m_guiDialogYesNo;
  dialog.SetHeading(heading);
  dialog.SetLine(0, line0);
  dialog.SetLine(1, line1);
  dialog.SetLine(2, line2);
  dialog.DoModal(m_gWindowManager.GetActiveWindow());
  return (dialog.IsConfirmed()) ? true : false;
}

bool CGUIDialogYesNo::IsConfirmed() const
{
  return m_bConfirmed;
}

void CGUIDialogYesNo::SetHeading(const wstring& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 1);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogYesNo::SetHeading(const string& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 1);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogYesNo::SetLine(int iLine, const wstring& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), iLine + 2);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogYesNo::SetLine(int iLine, const string& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), iLine + 2);
  msg.SetLabel(strLine);
  OnMessage(msg);
}
void CGUIDialogYesNo::SetHeading(int iString)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 1);
  if (iString)
    msg.SetLabel(iString);
  else
    msg.SetLabel("");
  OnMessage(msg);
}


void CGUIDialogYesNo::SetLine(int iLine, int iString)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), iLine + 2);
  msg.SetLabel(iString);
  OnMessage(msg);
}
