#include "guidialogok.h"
#include "guiWindowManager.h"
#include "localizeStrings.h"

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
			if (ACTION_SELECT_ITEM==iAction)
			{
				int iControl=message.GetSenderId();
				if (iControl==10)
				{
					m_bConfirmed=false;
					Close();
					return true;
				}
				if (iControl==11)
				{
					m_bConfirmed=true;
					Close();
					return true;
				}
			}
		}
		break;
	}
	return CGUIDialog::OnMessage(message);
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