#include "stdafx.h"
#include "GUIPythonWindowDialog.h"
#include "graphiccontext.h"
#include "GUIWindowManager.h"

CGUIPythonWindowDialog::CGUIPythonWindowDialog(DWORD dwId)
:CGUIPythonWindow(dwId)
{
	m_dwParentWindowID = 0;
	m_pParentWindow = NULL;
	m_dwPrevRouteWindow = WINDOW_INVALID;
	m_pPrevRouteWindow = NULL;
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

	if (m_gWindowManager.m_pRouteWindow == this) return;		// already routed to this dialog

	m_dwPrevRouteWindow=m_gWindowManager.RouteToWindow(GetID());
	m_pPrevRouteWindow=m_gWindowManager.GetWindow(m_dwPrevRouteWindow);

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

void CGUIPythonWindowDialog::Render()
{
	// render the parent window
	if (m_pParentWindow) 
		m_pParentWindow->Render();

	// render the previous route window
	if (m_pPrevRouteWindow)
		m_pPrevRouteWindow->Render();

	// render this dialog box
	CGUIPythonWindow::Render();
}

void CGUIPythonWindowDialog::Close()
{
	CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0);
  OnMessage(msg);

  m_gWindowManager.UnRoute(m_dwPrevRouteWindow);
	m_pParentWindow = NULL;
	m_bRunning = false;
}
