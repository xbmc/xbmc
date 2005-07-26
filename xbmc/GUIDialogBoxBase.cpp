#include "stdafx.h"
#include "GUIDialogBoxBase.h"

CGUIDialogBoxBase::CGUIDialogBoxBase(DWORD dwID, const CStdString &xmlFile)
    : CGUIDialog(dwID, xmlFile)
{
  m_bConfirmed = false;
}

CGUIDialogBoxBase::~CGUIDialogBoxBase(void)
{
}

bool CGUIDialogBoxBase::OnMessage(CGUIMessage& message)
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
  }
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxBase::IsConfirmed() const
{
  return m_bConfirmed;
}


void CGUIDialogBoxBase::SetHeading(const wstring& strLine)
{
  Initialize();
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 1);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogBoxBase::SetHeading(const string& strLine)
{
  Initialize();
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 1);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogBoxBase::SetHeading(int iString)
{
  Initialize();
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 1);
  if (iString)
    msg.SetLabel(iString);
  else
    msg.SetLabel("");
  OnMessage(msg);
}

void CGUIDialogBoxBase::SetLine(int iLine, const wstring& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), iLine + 2);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogBoxBase::SetLine(int iLine, const string& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), iLine + 2);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogBoxBase::SetLine(int iLine, int iString)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), iLine + 2);
  if (iString)
    msg.SetLabel(iString);
  else
    msg.SetLabel("");
  OnMessage(msg);
}
