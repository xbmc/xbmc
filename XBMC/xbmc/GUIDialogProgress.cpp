#include "guidialogprogress.h"
#include "guiWindowManager.h"

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
	int iControl=1;
  CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iControl,0,0,(void*)strLine.c_str());
  OnMessage(msg);

}

void CGUIDialogProgress::SetLine(int iLine, const wstring& strLine)
{
	int iControl=iLine+2;

  CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iControl,0,0,(void*)strLine.c_str());
  OnMessage(msg);
}