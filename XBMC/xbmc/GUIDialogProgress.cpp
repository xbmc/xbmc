#include "guidialogprogress.h"
#include "guiWindowManager.h"
#include "localizeStrings.h"

CGUIDialogProgress::CGUIDialogProgress(void)
:CGUIDialog(0)
{
}

CGUIDialogProgress::~CGUIDialogProgress(void)
{
}


void CGUIDialogProgress::StartModal(DWORD dwParentId)
{

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