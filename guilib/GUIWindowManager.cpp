#include "guiwindowmanager.h"

CGUIWindowManager     m_gWindowManager;

CGUIWindowManager::CGUIWindowManager(void)
{
	InitializeCriticalSection(&m_critSection);
	m_pCallback=NULL;
  m_pRouteWindow=NULL;
  m_iActiveWindow=-1;
  g_graphicsContext.setMessageSender(this);
}

CGUIWindowManager::~CGUIWindowManager(void)
{
	DeleteCriticalSection(&m_critSection);
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

void CGUIWindowManager::ActivateWindow(int iWindowID)
{
  // deactivate any window
	int iPrevActiveWindow=m_iActiveWindow;
  if (m_iActiveWindow >=0)
  {
    CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];
    CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0);
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
      CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0);
      pWindow->OnMessage(msg);
			return;
    }
  }

	// new window doesnt exists. (maybe .xml file is invalid or doesnt exists)
	// so we go back to the previous window
	for (int i=0; i < (int)m_vecWindows.size(); i++)
	{
		CGUIWindow* pWindow=m_vecWindows[i];
		if (pWindow->GetID() == iPrevActiveWindow) 
		{
			m_iActiveWindow=iPrevActiveWindow;
			CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];
			CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0);
			pWindow->OnMessage(msg);
		}
	}
}


void CGUIWindowManager::OnKey(const CKey& key)
{
  if (m_pRouteWindow)
  {
    m_pRouteWindow->OnKey(key);
    return;
  }
  if (m_iActiveWindow < 0) return;
  CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];
  if (pWindow) pWindow->OnKey(key);
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
    if (pWindow->GetID() == dwID) 
		{
			return pWindow;
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
