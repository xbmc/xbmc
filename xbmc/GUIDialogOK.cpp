
#include "stdafx.h"
#include "guidialogok.h"
#include "guiWindowManager.h"
#include "localizeStrings.h"

#define ID_BUTTON_NO   10
#define ID_BUTTON_YES  11

CGUIDialogOK::CGUIDialogOK(void)
:CGUIDialog(0)
{
	m_bConfirmed=false;
}

CGUIDialogOK::~CGUIDialogOK(void)
{
}

bool CGUIDialogOK::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_INIT:
    {
			CGUIDialog::OnMessage(message);
			m_bConfirmed=false;
			return true;
		}
		break;

    case GUI_MSG_CLICKED:
    {
			int iAction=message.GetParam1();
			if (1||ACTION_SELECT_ITEM==iAction)
			{
				int iControl=message.GetSenderId();
        if ( GetControl(ID_BUTTON_YES) == NULL)
        {
          m_bConfirmed=true;
					Close();
					return true;
        }
				if (iControl==ID_BUTTON_NO)
				{
					m_bConfirmed=false;
					Close();
					return true;
				}
				if (iControl==ID_BUTTON_YES)
				{
					m_bConfirmed=true;
					Close();
					return true;
				}
			}
		}
		break;
	}
  //only allow messages from or for this dialog
  if (
    (message.GetSenderId()  == GetID()) || 
    (message.GetControlId() == GetID()) || 
    (message.GetControlId() == 0)
  ) {
	  return CGUIDialog::OnMessage(message);
  }
  return false;
}


bool CGUIDialogOK::IsConfirmed() const
{
	return m_bConfirmed;
}


void  CGUIDialogOK::SetHeading(const wstring& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void  CGUIDialogOK::SetHeading(const string& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void CGUIDialogOK::SetLine(int iLine, const wstring& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void CGUIDialogOK::SetLine(int iLine, const string& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
	msg.SetLabel(strLine);
	OnMessage(msg);
}
void CGUIDialogOK::SetHeading(int iString)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(iString);
	OnMessage(msg);
}


void	CGUIDialogOK::SetLine(int iLine, int iString)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
	msg.SetLabel(iString);
	OnMessage(msg);
}

void CGUIDialogOK::OnAction(const CAction &action)
{
	if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
  {
		Close();
		return;
  }
	CGUIWindow::OnAction(action);
}
