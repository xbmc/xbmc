#include "stdafx.h"
#include "guidialog.h"
#include "guimessage.h"
#include "guiwindowmanager.h"
#include "GUILabelControl.h"

CGUIDialog::CGUIDialog(DWORD dwID)
:CGUIWindow(dwID)
{
	m_dwParentWindowID		= 0;
	m_pParentWindow			= NULL;
	m_dwPrevRouteWindow		= WINDOW_INVALID;
	m_pPrevRouteWindow		= NULL;
	m_bPrevOverlayAllowed	= true;
	m_bModal				= true;
}

CGUIDialog::~CGUIDialog(void)
{
}

bool CGUIDialog::Load(const CStdString& strFileName, bool bContainsPath)
{
	if (!CGUIWindow::Load(strFileName, bContainsPath))
	{
		return false;
	}

	// Clip labels to extents
	if (m_vecControls.size())
	{
		CGUIControl* pBase = m_vecControls[0];

		for (ivecControls p = m_vecControls.begin() + 1; p != m_vecControls.end(); ++p)
		{
			if ((*p)->GetControlType() == CGUIControl::GUICONTROL_LABEL)
			{
				CGUILabelControl* pLabel = (CGUILabelControl*)(*p);

				if (!pLabel->GetWidth())
				{
					int spacing = (pLabel->GetXPosition() - pBase->GetXPosition()) * 2;
					pLabel->SetWidth(pBase->GetWidth() - spacing);
					pLabel->m_dwTextAlign |= XBFONT_TRUNCATED;
				}
			}
		}
	}

	return true;
}

bool CGUIDialog::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_WINDOW_INIT:
		{
			m_bPrevOverlayAllowed = g_graphicsContext.IsOverlayAllowed();
			CGUIWindow::OnMessage(message);
			return true;
		}

		case GUI_MSG_WINDOW_DEINIT:
		{
			g_graphicsContext.SetOverlay(m_bPrevOverlayAllowed);
			break;
		}
	}

	return CGUIWindow::OnMessage(message);
}

void CGUIDialog::Render()
{
	// render the parent window
	if (m_bModal)
	{
		//Parent is rendered by the WindowManager
		//if (m_pParentWindow)
		//{
		//	m_pParentWindow->Render();
		//}

		// render the previous route window
		if (m_pPrevRouteWindow)
		{
			m_pPrevRouteWindow->Render();
		}
	}

	// render this dialog box
	CGUIWindow::Render();
}


void CGUIDialog::Close()
{
	CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0);
	OnMessage(msg);

	if (m_bModal)
	{
		m_gWindowManager.UnRoute(m_dwPrevRouteWindow);
	}
	else
	{
		m_gWindowManager.RemoveModeless( GetID() );
	}

	m_pParentWindow = NULL;
	m_bRunning = false;
}



void CGUIDialog::DoModal(DWORD dwParentId)
{
	m_dwParentWindowID	= dwParentId;
	m_pParentWindow		= m_gWindowManager.GetWindow( m_dwParentWindowID);

	if (!m_pParentWindow)
	{
		m_dwParentWindowID=0;
		return;
	}

	if (m_gWindowManager.m_pRouteWindow == this)
	{
		// already routed to this dialog
		return;
	}

	m_bModal			= true;
	m_dwPrevRouteWindow	= m_gWindowManager.RouteToWindow(GetID());
	m_pPrevRouteWindow	= m_gWindowManager.GetWindow(m_dwPrevRouteWindow);

	// active this window...
	CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0);
	OnMessage(msg);

	m_bRunning = true;
	while (m_bRunning)
	{
		m_gWindowManager.Process();
	}
}



void CGUIDialog::Show(DWORD dwParentId)
{
	m_dwParentWindowID	= dwParentId;
	m_pParentWindow		= m_gWindowManager.GetWindow( m_dwParentWindowID);

	if (!m_pParentWindow)
	{
		m_dwParentWindowID=0;
		return;
	}

	m_bModal = false;
	m_gWindowManager.AddModeless(this);

	// active this window...
	CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0);
	OnMessage(msg);

	m_bRunning = true;
}