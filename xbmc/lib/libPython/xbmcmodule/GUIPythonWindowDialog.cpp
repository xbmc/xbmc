#include "stdafx.h"
#include "GUIPythonWindowDialog.h"
#include "graphiccontext.h"
#include "GUIWindowManager.h"

CGUIPythonWindowDialog::CGUIPythonWindowDialog(DWORD dwId)
:CGUIPythonWindow(dwId)
{
	m_dwParentWindowID = 0;
	m_pParentWindow = NULL;
	m_bRunning = false;
}

CGUIPythonWindowDialog::~CGUIPythonWindowDialog(void)
{
}

void CGUIPythonWindowDialog::Activate(DWORD dwParentId)
{
	m_dwParentWindowID = dwParentId;
	m_pParentWindow = m_gWindowManager.GetWindow( m_dwParentWindowID);
	if (!m_pParentWindow)
	{
		m_dwParentWindowID=0;
		return;
	}

	m_gWindowManager.RouteToWindow(this);

  // active this dialog...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0);
  OnMessage(msg);
}

bool CGUIPythonWindowDialog::OnMessage(CGUIMessage& message)
{
	switch(message.GetMessage())
	{
		case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);
			return true;
		}
		break;

		case GUI_MSG_CLICKED:
		{
			return CGUIPythonWindow::OnMessage(message);
		}
		break;
	}

	// we do not message CGUIPythonWindow here..
	return CGUIWindow::OnMessage(message);
}

void CGUIPythonWindowDialog::Close()
{
	CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0);
  OnMessage(msg);

  m_gWindowManager.UnRoute(GetID());
	m_pParentWindow = NULL;
	m_bRunning = false;
}
