#include "guiwindowmanager.h"

CGUIWindowManager     m_gWindowManager;

CGUIWindowManager::CGUIWindowManager(void)
{
	m_pCallback=NULL;
  m_pRouteWindow=NULL;
  m_iActiveWindow=-1;
  g_graphicsContext.setMessageSender(this);
}

CGUIWindowManager::~CGUIWindowManager(void)
{
}

void CGUIWindowManager::SendMessage(CGUIMessage& message)
{
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
  pWindow->OnKey(key);
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
  }
}

void CGUIWindowManager::RouteToWindow(DWORD dwID)
{
  m_pRouteWindow=GetWindow(dwID);
}

void CGUIWindowManager::UnRoute()
{
  m_pRouteWindow=NULL;
}