#include "stdafx.h"
#include "guiwindowmanager.h"

CGUIWindowManager     m_gWindowManager;

CGUIWindowManager::CGUIWindowManager(void)
{
	InitializeCriticalSection(&m_critSection);
	m_pCallback=NULL;
  m_pRouteWindow=NULL;
  m_iActiveWindow=-1;
}

CGUIWindowManager::~CGUIWindowManager(void)
{
	DeleteCriticalSection(&m_critSection);
}

void CGUIWindowManager::Initialize()
{
  m_iActiveWindow=-1;
  g_graphicsContext.setMessageSender(this);
}

void CGUIWindowManager::SendMessage(CGUIMessage& message)
{
	for (int i=0; i < (int) m_vecMsgTargets.size(); i++)
	{
		IMsgTargetCallback* pMsgTarget = m_vecMsgTargets[i];
		if (pMsgTarget)
			pMsgTarget->OnMessage( message );
	}

  if (m_pRouteWindow)
  {
    m_pRouteWindow->OnMessage(message);
    return;
  }
  if (m_iActiveWindow < 0) return;
  CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];
  pWindow->OnMessage(message);
}

void CGUIWindowManager::Add(CGUIWindow* pWindow)
{
  m_vecWindows.push_back(pWindow);
}

void CGUIWindowManager::Remove(DWORD dwID)
{
	vector<CGUIWindow*>::iterator it = m_vecWindows.begin();
	while (it != m_vecWindows.end())
	{
		CGUIWindow* pWindow = *it;
		if(pWindow->GetID() == dwID)
		{
			m_vecWindows.erase(it);
			it = m_vecWindows.end();
		}
		else it++;
	}
}

void CGUIWindowManager::PreviousWindow()
{
	// deactivate any window
	int iPrevActiveWindow=m_iActiveWindow;
	int iPrevActiveWindowID=0;
	if (m_iActiveWindow >=0)
	{
    
    if (m_iActiveWindow >=0 && m_iActiveWindow < (int)m_vecWindows.size())
    {
		  CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];
		  iPrevActiveWindowID = pWindow->GetPreviousWindowID();
		  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0,iPrevActiveWindowID);
		  pWindow->OnMessage(msg);
		  m_iActiveWindow=-1;
    }
	}

	// activate the new window
	for (int i=0; i < (int)m_vecWindows.size(); i++)
	{
		CGUIWindow* pWindow=m_vecWindows[i];
		if (pWindow->GetID() == iPrevActiveWindowID) 
		{
			m_iActiveWindow=i;
			CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0,WINDOW_INVALID);
			pWindow->OnMessage(msg);
			return;
		}
	}

	// previous window doesnt exists. (maybe .xml file is invalid or doesnt exists)
	// so we go back to the previous window
	m_iActiveWindow=0;
	CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];
	CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0,WINDOW_INVALID);
	pWindow->OnMessage(msg);
}

void CGUIWindowManager::RefreshWindow()
{
	// deactivate the current window
	if (m_iActiveWindow >=0)
	{
		CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];
		CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0);
		pWindow->OnMessage(msg);
	}
	// reactivate the current window
	CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];
	CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0,WINDOW_INVALID);
	pWindow->OnMessage(msg);
}

void CGUIWindowManager::ActivateWindow(int iWindowID)
{
  // deactivate any window
	int iPrevActiveWindow=m_iActiveWindow;
  if (m_iActiveWindow >=0)
  {
    CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];
    CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0,iWindowID);
    pWindow->OnMessage(msg);
    m_iActiveWindow=-1;
  }

  // activate the new window
  for (int i=0; i < (int)m_vecWindows.size(); i++)
  {
    CGUIWindow* pWindow=m_vecWindows[i];
    if (pWindow->GetID() == iWindowID) 
    {
		m_iActiveWindow=i;
		// Check to see that this window is not our previous window
		if (m_vecWindows[iPrevActiveWindow]->GetPreviousWindowID() == iWindowID)
		{	// we are going to the lsat window - don't update it's previous window id
			CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0,WINDOW_INVALID);
			pWindow->OnMessage(msg);
		}
		else
		{	// we are going to a new window - put our current window into it's previous window ID
			CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0,m_vecWindows[iPrevActiveWindow]->GetID());
			pWindow->OnMessage(msg);
		}
		return;
    }
  }

	// new window doesnt exists. (maybe .xml file is invalid or doesnt exists)
	// so we go back to the previous window
	m_iActiveWindow=iPrevActiveWindow;
	CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];
	CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0,WINDOW_INVALID);
	pWindow->OnMessage(msg);
}


void CGUIWindowManager::OnAction(const CAction &action)
{
  if (m_pRouteWindow)
  {
    m_pRouteWindow->OnAction(action);
    return;
  }
  if (m_iActiveWindow < 0) return;
  CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];
  if (pWindow) pWindow->OnAction(action);
}

void CGUIWindowManager::Render()
{
  if (m_pRouteWindow)
  {
    m_pRouteWindow->Render();
    return;
  }
  if (m_iActiveWindow < 0) return;
  CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];
  pWindow->Render();
}

CGUIWindow*  CGUIWindowManager::GetWindow(DWORD dwID)
{
  for (int i=0; i < (int)m_vecWindows.size(); i++)
  {
    CGUIWindow* pWindow=m_vecWindows[i];
    if (pWindow)
    {
      if (pWindow->GetID() == dwID) 
		  {
			  return pWindow;
		  }
    }
	}
	return NULL;
}

void CGUIWindowManager::Process()
{
	if (m_pCallback)
	{
		m_pCallback->FrameMove();
		m_pCallback->Render();

	}
}


void CGUIWindowManager::SetCallback(IWindowManagerCallback& callback) 
{
	m_pCallback=&callback;
}

void CGUIWindowManager::DeInitialize()
{
  for (int i=0; i < (int)m_vecWindows.size(); i++)
  {
    CGUIWindow* pWindow=m_vecWindows[i];
    CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0);
    pWindow->OnMessage(msg);
		pWindow->ClearAll();
  }
	m_pRouteWindow=NULL;

	m_vecMsgTargets.erase( m_vecMsgTargets.begin(), m_vecMsgTargets.end() );
}

void CGUIWindowManager::RouteToWindow(DWORD dwID)
{
  m_pRouteWindow=GetWindow(dwID);
}

void CGUIWindowManager::UnRoute()
{
  m_pRouteWindow=NULL;
}
void CGUIWindowManager::SendThreadMessage(CGUIMessage& message)
{
	::EnterCriticalSection(&m_critSection );
	CGUIMessage* msg = new CGUIMessage(message);
	m_vecThreadMessages.push_back( msg );

	::LeaveCriticalSection(&m_critSection );
}

void CGUIWindowManager::DispatchThreadMessages()
{
	::EnterCriticalSection(&m_critSection );

	if ( m_vecThreadMessages.size() > 0 ) 
	{
		for (int i=0; i < (int) m_vecThreadMessages.size(); i++ ) 
		{
			CGUIMessage* pMsg = m_vecThreadMessages[i];
			SendMessage( *pMsg );
			delete pMsg;
		}

		m_vecThreadMessages.erase( m_vecThreadMessages.begin(), m_vecThreadMessages.end() );
	}

	::LeaveCriticalSection(&m_critSection );
}

void CGUIWindowManager::AddMsgTarget( IMsgTargetCallback* pMsgTarget )
{
	m_vecMsgTargets.push_back( pMsgTarget );
}


int CGUIWindowManager::GetActiveWindow() const
{
	if (m_iActiveWindow < 0) return 0;
	CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];
	return pWindow->GetID();
}

bool CGUIWindowManager::IsRouted() const
{
  if (m_pRouteWindow) return true;
  return false;
}