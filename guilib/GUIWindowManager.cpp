#include "stdafx.h"
#include "guiwindowmanager.h"
#include "../xbmc/utils/log.h"

CGUIWindowManager m_gWindowManager;

CGUIWindowManager::CGUIWindowManager(void)
{
	InitializeCriticalSection(&m_critSection);

	m_pCallback		= NULL;
	m_iActiveWindow	= -1;
}

CGUIWindowManager::~CGUIWindowManager(void)
{
	DeleteCriticalSection(&m_critSection);
}

void CGUIWindowManager::Initialize()
{
	m_iActiveWindow = -1;
	g_graphicsContext.setMessageSender(this);
}

void CGUIWindowManager::SendMessage(CGUIMessage& message)
{
	//	Send the message to all none window targets
	for (int i=0; i < (int) m_vecMsgTargets.size(); i++)
	{
		IMsgTargetCallback* pMsgTarget = m_vecMsgTargets[i];
		
		if (pMsgTarget)
		{
			pMsgTarget->OnMessage( message );
		}
	}

	//	Have we routed windows...
	if (m_vecModalWindows.size()>0)
	{
		//	...send the message to the top most.
		m_vecModalWindows[m_vecModalWindows.size()-1]->OnMessage(message);

		if (m_iActiveWindow < 0)
		{
			return;
		}

		CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];

		//	Also send the message to the parent of the routed window, if its the target
		if ( message.GetSenderId() == pWindow->GetID() ||
			 message.GetControlId()== pWindow->GetID() ||
			 message.GetSenderId() == 0 )
		{
			pWindow->OnMessage(message);
		}
	}
	else
	{
		//	..no, only call message function of the active window
		if (m_iActiveWindow < 0)
		{
			return;
		}
		
		CGUIWindow* pWindow = m_vecWindows[m_iActiveWindow];
		pWindow->OnMessage(message);
	}
}

void CGUIWindowManager::Add(CGUIWindow* pWindow)
{
	m_vecWindows.push_back(pWindow);
}

void CGUIWindowManager::AddCustomWindow(CGUIWindow* pWindow)
{
	Add(pWindow);
	m_vecCustomWindows.push_back(pWindow);
}

void CGUIWindowManager::AddModeless(CGUIWindow* pWindow)
{
	m_vecModelessWindows.push_back(pWindow);
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

void CGUIWindowManager::RemoveModeless(DWORD dwID)
{
	vector<CGUIWindow*>::iterator it = m_vecModelessWindows.begin();
	while (it != m_vecModelessWindows.end())
	{
		CGUIWindow* pWindow = *it;
		if(pWindow->GetID() == dwID)
		{
			m_vecModelessWindows.erase(it);
			it = m_vecModelessWindows.end();
		}
		else it++;
	}
}

void CGUIWindowManager::PreviousWindow()
{
	// deactivate any window
	CLog::DebugLog("CGUIWindowManager::PreviousWindow: Deactivate");

	int iPrevActiveWindow = m_iActiveWindow;
	int iPrevActiveWindowID = 0;

	if (m_iActiveWindow >=0 && m_iActiveWindow < (int)m_vecWindows.size())
    {
		  CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];
		  iPrevActiveWindowID = pWindow->GetPreviousWindowID();
		  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0,iPrevActiveWindowID);
		  pWindow->OnMessage(msg);
		  m_iActiveWindow=WINDOW_INVALID;
    }

	CLog::DebugLog("CGUIWindowManager::PreviousWindow: Activate new");
	// activate the new window
	for (int i=0; i < (int)m_vecWindows.size(); i++)
	{
		CGUIWindow* pWindow = m_vecWindows[i];

		if (pWindow->HasID(iPrevActiveWindowID)) 
		{
			CLog::DebugLog("CGUIWindowManager::PreviousWindow: Activating");
			m_iActiveWindow = i;
			CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0,WINDOW_INVALID, iPrevActiveWindowID);
			pWindow->OnMessage(msg);
			return;
		}
	}

	CLog::DebugLog("CGUIWindowManager::PreviousWindow: No previous");
	// previous window doesnt exists. (maybe .xml file is invalid or doesnt exists)
	// so we go back to the previous window
	m_iActiveWindow = 0;
	CGUIWindow* pWindow = m_vecWindows[m_iActiveWindow];
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

void CGUIWindowManager::ActivateWindow(int iWindowID, const CStdString& strPath)
{
	// first check existence of the window we wish to activate.
	CGUIWindow *pNewWindow = GetWindow(iWindowID);
	if (!pNewWindow)
	{ // nothing to see here - move along
		CLog::Log(LOGERROR, "Unable to locate window with id %d.  Check skin files", iWindowID - WINDOW_HOME);
		return;
	}

	// deactivate any window
	int iPrevActiveWindow = m_iActiveWindow;
	if (m_iActiveWindow >=0)
	{
		CGUIWindow* pWindow = m_vecWindows[m_iActiveWindow];
		CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0,iWindowID);
		pWindow->OnMessage(msg);
		m_iActiveWindow = -1;
	}

	// activate the new window
	for (int i=0; i < (int)m_vecWindows.size(); i++)
	{
		CGUIWindow* pWindow = m_vecWindows[i];

		if (pWindow->HasID(iWindowID)) 
		{
			CLog::Log(LOGINFO,"Activating Window ID: %i",iWindowID);
			m_iActiveWindow = i;
			
			// Check to see that this window is not our previous window
			if (iPrevActiveWindow==-1 || m_vecWindows[iPrevActiveWindow]->GetPreviousWindowID() == iWindowID)
			{
				// we are going to the last window - don't update it's previous window id
				CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0,WINDOW_INVALID, iWindowID);

				// append the destination path
				if (strPath.size() > 0) msg.SetStringParam(strPath);
				pWindow->OnMessage(msg);
			}
			else
			{
				if (m_vecWindows[iPrevActiveWindow]==pWindow)
				{
					// we are going to the same window - leave previous window ID as is
					CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0,pWindow->GetPreviousWindowID(), iWindowID);

					// do we need to append a destination path if the window is the same?
					if (strPath.size() > 0) msg.SetStringParam(strPath);
					pWindow->OnMessage(msg);
				}
				else
				{
					// we are going to a new window - put our current window into it's previous window ID
					CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0,m_vecWindows[iPrevActiveWindow]->GetID(), iWindowID);

					// append the destination path
					if (strPath.size() > 0) msg.SetStringParam(strPath);
					pWindow->OnMessage(msg);
				}
			}
			return;
		}
	}
	// we should never, ever get here.
	CLog::Log(LOGERROR, "ActivateWindow() failed trying to activate window %d", iWindowID - WINDOW_HOME);
}

void CGUIWindowManager::OnAction(const CAction &action)
{
	//	Have we have routed windows...
	if (m_vecModalWindows.size()>0)
	{
		//	...send the action to the top most.
		m_vecModalWindows[m_vecModalWindows.size()-1]->OnAction(action);
	}
	else if (m_iActiveWindow >= 0)
	{
		CGUIWindow* pWindow = m_vecWindows[m_iActiveWindow];

		if (pWindow)
		{
			pWindow->OnAction(action);
		}
	}
}

void CGUIWindowManager::Render()
{
	if (m_iActiveWindow >= 0)
	{
		CGUIWindow* pWindow=m_vecWindows[m_iActiveWindow];
		pWindow->Render();
	}

	// render modal windows
	int nWindow = 0;
	int nWindowsPre;

	// continuously evaluate if there are more windows in our list to process
	while ( nWindow < (nWindowsPre = m_vecModalWindows.size()) )
	{
		m_vecModalWindows[nWindow]->Render();

		// if the modal dialog hasn't closed, and removed itself from the list
		if (nWindowsPre==m_vecModalWindows.size())
		{
			// try the next window
			nWindow++;
		}
	}

	// render modeless windows
	nWindow = 0;

	// continuously evaluate if there are more windows in our list to process
	while ( nWindow < (nWindowsPre = m_vecModelessWindows.size()) )
	{
		m_vecModelessWindows[nWindow]->Render();

		// if the modeless dialog hasn't closed, and removed itself from the list
		if (nWindowsPre==m_vecModelessWindows.size())
		{
			// try the next window
			nWindow++;
		}
	}
}

CGUIWindow*  CGUIWindowManager::GetWindow(DWORD dwID)
{
	if (dwID == WINDOW_INVALID)
	{
		return NULL;
	}

	for (int i=0; i < (int)m_vecWindows.size(); i++)
	{
		CGUIWindow* pWindow=m_vecWindows[i];
		if (pWindow)
		{
			if (pWindow->HasID(dwID)) 
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
		m_pCallback->Process();
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

	m_vecMsgTargets.erase( m_vecMsgTargets.begin(), m_vecMsgTargets.end() );

	// destroy our custom windows...
	for (int i=0; i < (int)m_vecCustomWindows.size(); i++)
	{
		CGUIWindow *pWindow = m_vecCustomWindows[i];
		Remove(pWindow->GetID());
		delete pWindow;
	}
	m_vecCustomWindows.erase( m_vecCustomWindows.begin(), m_vecCustomWindows.end() );

}

/// \brief Route to a window
/// \param pWindow Window to route to
void CGUIWindowManager::RouteToWindow(CGUIWindow* pWindow)
{
	//	Just to be sure: Unroute this window, 
	//	#we may have routed to it before
	UnRoute(pWindow->GetID());

	m_vecModalWindows.push_back(pWindow);

}

/// \brief Unroute window
/// \param dwID ID of the window routed
void CGUIWindowManager::UnRoute(DWORD dwID)
{
	vector<CGUIWindow*>::iterator it = m_vecModalWindows.begin();
	while (it != m_vecModalWindows.end())
	{
		CGUIWindow* pWindow = *it;
		if(pWindow->GetID() == dwID)
		{
			m_vecModalWindows.erase(it);
			it = m_vecModalWindows.end();
		}
		else it++;
	}
}

bool CGUIWindowManager::IsRouted() const
{
	if (m_vecModalWindows.size()>0)
		return true;

	return false;
}

/// \brief Get the ID of the top most routed window
/// \return dwID ID of the window or WINDOW_INVALID if no routed window available
int CGUIWindowManager::GetTopMostRoutedWindowID() const
{
	if (m_vecModalWindows.size()<=0)
		return WINDOW_INVALID;

	return m_vecModalWindows[m_vecModalWindows.size()-1]->GetID();
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
		vector<CGUIMessage*>::iterator it = m_vecThreadMessages.begin();
		while (it != m_vecThreadMessages.end())
		{
			CGUIMessage* pMsg = *it;
			// first remove the message from the queue,
			// else the message could be processed more then once
			it = m_vecThreadMessages.erase(it);
			SendMessage( *pMsg );
			delete pMsg;
		}
	}

	::LeaveCriticalSection(&m_critSection );
}

void CGUIWindowManager::AddMsgTarget( IMsgTargetCallback* pMsgTarget )
{
	m_vecMsgTargets.push_back( pMsgTarget );
}

int CGUIWindowManager::GetActiveWindow() const
{
	if (m_iActiveWindow < 0)
	{
		return 0;
	}

	CGUIWindow* pWindow = m_vecWindows[m_iActiveWindow];
	
	return pWindow->GetID();
}
