#include "guidialogprogress.h"
#include "guiWindowManager.h"
#include "localizeStrings.h"
#include "GUIProgressControl.h"

#define CONTROL_PROGRESS_BAR 20

CGUIDialogProgress::CGUIDialogProgress(void)
:CGUIDialog(0)
{
	m_bCanceled=false;
}

CGUIDialogProgress::~CGUIDialogProgress(void)
{
}


void CGUIDialogProgress::StartModal(DWORD dwParentId)
{
	m_bCanceled=false;
	m_dwParentWindowID=dwParentId;
	m_pParentWindow=m_gWindowManager.GetWindow( m_dwParentWindowID);
	if (!m_pParentWindow)
	{
		m_dwParentWindowID=0;
		return;
	}

	m_gWindowManager.RouteToWindow( GetID() );

  // active this window...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0);
  OnMessage(msg);
	ShowProgressBar(false);
	SetPercentage(0);
	m_bRunning=true;
}

void CGUIDialogProgress::Progress()
{
	if  (m_bRunning)
	{
		m_gWindowManager.Process();
	}
}

void  CGUIDialogProgress::SetHeading(const wstring& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void  CGUIDialogProgress::SetHeading(const string& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void CGUIDialogProgress::SetLine(int iLine, const wstring& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void CGUIDialogProgress::SetLine(int iLine, const string& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
	msg.SetLabel(strLine);
	OnMessage(msg);
}
void CGUIDialogProgress::SetHeading(int iString)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(iString);
	OnMessage(msg);
}


void	CGUIDialogProgress::SetLine(int iLine, int iString)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
	msg.SetLabel(iString);
	OnMessage(msg);
}

void CGUIDialogProgress::Close()
{
	CGUIDialog::Close();
}

bool CGUIDialogProgress::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
  {

    case GUI_MSG_CLICKED:
    {
			int iAction=message.GetParam1();
			if (ACTION_SELECT_ITEM==iAction)
			{
				int iControl=message.GetSenderId();
				if (iControl==10)
				{
					m_bCanceled=true;
					return true;
				}
			}
		}
		break;
	}
	return CGUIDialog::OnMessage(message);
}

bool CGUIDialogProgress::IsCanceled() const
{
	return m_bCanceled;
}

void CGUIDialogProgress::SetPercentage(int iPercentage)
{
	CGUIProgressControl* pControl = (CGUIProgressControl*)GetControl(CONTROL_PROGRESS_BAR);
	pControl->SetPercentage(iPercentage);
}
void CGUIDialogProgress::ShowProgressBar(bool bOnOff)
{
	if (bOnOff)
	{
		CGUIMessage msg(GUI_MSG_VISIBLE, GetID(), CONTROL_PROGRESS_BAR); 
		OnMessage(msg);
	}
	else
	{
		CGUIMessage msg(GUI_MSG_HIDDEN, GetID(), CONTROL_PROGRESS_BAR); 
		OnMessage(msg);
	}
}