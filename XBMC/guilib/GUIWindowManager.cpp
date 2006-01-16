#include "include.h"
#include "GUIWindowManager.h"
#include "GUIAudioManager.h"
#include "GUIDialog.h"
#include "../xbmc/settings.h"
//GeminiServer 
#include "../xbmc/GUIPassword.h"
#include "../xbmc/utils/GUIInfoManager.h"


CGUIWindowManager m_gWindowManager;

CGUIWindowManager::CGUIWindowManager(void)
{
  InitializeCriticalSection(&m_critSection);

  m_pCallback = NULL;
  m_bShowOverlay = true;
}

CGUIWindowManager::~CGUIWindowManager(void)
{
  DeleteCriticalSection(&m_critSection);
}

void CGUIWindowManager::Initialize()
{
  g_graphicsContext.setMessageSender(this);
  LoadNotOnDemandWindows();
}

bool CGUIWindowManager::SendMessage(CGUIMessage& message)
{
  bool handled = false;
//  CLog::DebugLog("SendMessage: mess=%d send=%d control=%d param1=%d", message.GetMessage(), message.GetSenderId(), message.GetControlId(), message.GetParam1());
  // Send the message to all none window targets
  for (int i = 0; i < (int) m_vecMsgTargets.size(); i++)
  {
    IMsgTargetCallback* pMsgTarget = m_vecMsgTargets[i];

    if (pMsgTarget)
    {
      if (pMsgTarget->OnMessage( message )) handled = true;
    }
  }

  //  Send the message to all active modeless windows ..
  for (int i=0; i < (int) m_vecModelessWindows.size(); i++)
  {
    CGUIWindow* pWindow = m_vecModelessWindows[i];

    if (pWindow)
    {
      if (pWindow->OnMessage( message )) handled = true;
    }
  }

  //  A GUI_MSG_NOTIFY_ALL is send to any active modal dialog
  //  and all windows whether they are active or not
  if (message.GetMessage()==GUI_MSG_NOTIFY_ALL)
  {
    int topWindow = m_vecModalWindows.size();
    while (topWindow)
      m_vecModalWindows[--topWindow]->OnMessage(message);

    for (map<DWORD, CGUIWindow *>::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
    {
      CGUIWindow *pWindow = (*it).second;
      pWindow->OnMessage(message);
    }
    handled = true;
  }
  // Have we routed windows...
  else if (m_vecModalWindows.size() > 0)
  {
    // ...send the message to the top most.
    int topWindow = m_vecModalWindows.size();
    bool modalHandled = false;
    while (topWindow && !modalHandled)
    {
      if (m_vecModalWindows[--topWindow]->OnMessage(message))
        modalHandled = true;
    }
    if (modalHandled) handled = true;

    CGUIWindow* pWindow = GetWindow(GetActiveWindow());
    if (!pWindow)
      return false;

    // Also send the message to the parent of the routed window, if its the target
    if ( message.GetSenderId() == pWindow->GetID() ||
         message.GetControlId() == pWindow->GetID() ||
         message.GetSenderId() == 0 )
    {
      if (pWindow->OnMessage(message)) handled = true;
    }
  }
  else
  {
    // ..no, only call message function of the active window
    CGUIWindow* pWindow = GetWindow(GetActiveWindow());
    if (pWindow && pWindow->OnMessage(message)) handled = true;
  }
  return handled;
}

void CGUIWindowManager::Add(CGUIWindow* pWindow)
{
  if (!pWindow)
  {
    CLog::Log(LOGERROR, "Attempted to add a NULL window pointer to the window manager.");
    return;
  }
  // push back all the windows if there are more than one covered by this class
  for (unsigned int i = 0; i < pWindow->GetIDRange(); i++)
  {
    map<DWORD, CGUIWindow *>::iterator it = m_mapWindows.find(pWindow->GetID() + i);
    if (it != m_mapWindows.end())
    {
      CLog::Log(LOGERROR, "Error, trying to add a second window with id %i to the window manager", pWindow->GetID());
      return;
    }
    m_mapWindows.insert(pair<DWORD, CGUIWindow *>(pWindow->GetID() + i, pWindow));
  }
}

void CGUIWindowManager::AddCustomWindow(CGUIWindow* pWindow)
{
  Add(pWindow);
  m_vecCustomWindows.push_back(pWindow);
}

void CGUIWindowManager::AddModeless(CGUIWindow* pWindow)
{
  // only add the window if it's not already added
  for (unsigned int i = 0; i < m_vecModelessWindows.size(); i++)
    if (m_vecModelessWindows[i] == pWindow) return;
  m_vecModelessWindows.push_back(pWindow);
}

void CGUIWindowManager::Remove(DWORD dwID)
{
  map<DWORD, CGUIWindow *>::iterator it = m_mapWindows.find(dwID);
  if (it != m_mapWindows.end())
  {
    m_mapWindows.erase(it);
  }
  else
  {
    CLog::Log(LOGWARNING, "Attempted to remove window %i from the window manager when it didn't exist", dwID);
  }
}

// removes and deletes the window.  Should only be called
// from the class that created the window using new.
void CGUIWindowManager::Delete(DWORD dwID)
{
  CGUIWindow *pWindow = GetWindow(dwID);
  if (pWindow)
  {
    Remove(dwID);
    delete pWindow;
  }
}

void CGUIWindowManager::RemoveModeless(DWORD dwID)
{
  vector<CGUIWindow*>::iterator it = m_vecModelessWindows.begin();
  while (it != m_vecModelessWindows.end())
  {
    CGUIWindow* pWindow = *it;
    if (pWindow->GetID() == dwID)
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
  CGUIWindow *pWindow = GetWindow(GetActiveWindow());
  if (!pWindow || m_windowHistory.size() < 2)
    return;     // no windows or window history yet

  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
  pWindow->OnMessage(msg);
  // pop the window off our window stack
  m_windowHistory.pop();

  pWindow = GetWindow(GetActiveWindow());
  if (!pWindow)
  {
    CLog::Log(LOGERROR, "Unable to activate the previous window");
 //   ClearWindowHistory();
    while (m_windowHistory.size())
      m_windowHistory.pop();
    ActivateWindow(WINDOW_HOME);
  }

  // ok, initialize the new window
  CLog::DebugLog("CGUIWindowManager::PreviousWindow: Activate new");
  CGUIMessage msg2(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, GetActiveWindow());
  pWindow->OnMessage(msg2);
  return;
}

void CGUIWindowManager::RefreshWindow()
{
  // deactivate the current window
  CGUIWindow *pWindow = GetWindow(GetActiveWindow());
  if (!pWindow)
    return;

  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
  pWindow->OnMessage(msg);
  CGUIMessage msg2(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID);
  pWindow->OnMessage(msg);
}

void CGUIWindowManager::ChangeActiveWindow(int newWindow, const CStdString& strPath)
{
  ActivateWindow(newWindow, strPath, true);
}

void CGUIWindowManager::ActivateWindow(int iWindowID, const CStdString& strPath, bool swappingWindows)
{
  // translate virtual windows
  // virtual music window which returns the last open music window (aka the music start window)
  if (iWindowID == WINDOW_MUSIC)
  {
    iWindowID = g_stSettings.m_iMyMusicStartWindow;
    // ensure the music virtual window only returns music files and music library windows
    if (iWindowID != WINDOW_MUSIC_FILES && iWindowID != WINDOW_MUSIC_NAV)
      iWindowID = WINDOW_MUSIC_FILES;
  }
  // virtual video window which returns the last open video window (aka the video start window)
  if (iWindowID == WINDOW_VIDEOS)
  {
    if (g_stSettings.m_iVideoStartWindow > 0)
    {
      iWindowID = g_stSettings.m_iVideoStartWindow;
    }
  }

  // debug
  CLog::Log(LOGDEBUG, "Activating window ID: %i", iWindowID);

  // GeminiServer HomeMenuLock with MasterCode!
  if(!g_passwordManager.CheckMenuLock(iWindowID))
  {
    CLog::Log(LOGERROR, "MasterCode is Wrong: Window with id %d will not be loaded! Enter a correct MasterCode!", iWindowID);
    iWindowID = WINDOW_HOME;
  }

  // first check existence of the window we wish to activate.
  CGUIWindow *pNewWindow = GetWindow(iWindowID);
  if (!pNewWindow)
  { // nothing to see here - move along
    CLog::Log(LOGERROR, "Unable to locate window with id %d.  Check skin files", iWindowID - WINDOW_HOME);
    return ;
  }
  else if (pNewWindow->IsDialog())
  { // if we have a dialog, we do a DoModal() rather than activate the window
    if (!((CGUIDialog *)pNewWindow)->IsRunning())
      ((CGUIDialog *)pNewWindow)->DoModal(GetActiveWindow(), iWindowID);
    return;
  }

  // deactivate any window
  CGUIWindow *pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
  {
    //  Play the window specific deinit sound
    g_audioManager.PlayWindowSound(pWindow->GetID(), SOUND_DEINIT);
    CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, iWindowID);
    pWindow->OnMessage(msg);
  }

  // Add window to the history list (we must do this before we activate it,
  // as all messages done in WINDOW_INIT will want to be sent to the new
  // topmost window).  If we are swapping windows, we pop the old window
  // off the history stack
  if (swappingWindows && m_windowHistory.size())
    m_windowHistory.pop();
  AddToWindowHistory(iWindowID);

  // Send the init message
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, GetActiveWindow(), iWindowID);
  if (!strPath.IsEmpty()) msg.SetStringParam(strPath);
  pNewWindow->OnMessage(msg);
}

bool CGUIWindowManager::OnAction(const CAction &action)
{
  // Have we have routed windows...
  if (m_vecModalWindows.size() > 0)
  {
    // ...send the action to the top most.
    return m_vecModalWindows[m_vecModalWindows.size() - 1]->OnAction(action);
  }
  else
  {
    CGUIWindow* pWindow = GetWindow(GetActiveWindow());
    if (pWindow)
      return pWindow->OnAction(action);
  }
  return false;
}

void CGUIWindowManager::Render()
{
  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->Render();
}

bool RenderOrderSortFunction(CGUIDialog *first, CGUIDialog *second)
{
  return first->GetRenderOrder() < second->GetRenderOrder();
}

void CGUIWindowManager::RenderDialogs()
{
  // find the window with the lowest render order
  vector<CGUIDialog *> renderList;
  for (unsigned int i = 0; i < m_vecModalWindows.size(); i++)
    renderList.push_back((CGUIDialog *)m_vecModalWindows[i]);
  for (unsigned int i = 0; i < m_vecModelessWindows.size(); i++)
    renderList.push_back((CGUIDialog *)m_vecModelessWindows[i]);
  stable_sort(renderList.begin(), renderList.end(), RenderOrderSortFunction);

  // iterate through and render if they're running
  for (unsigned int i = 0; i < renderList.size(); i++)
  {
    CGUIDialog *pDialog = renderList[i];
    if (pDialog->IsRunning())
      pDialog->Render();
  }
}

CGUIWindow* CGUIWindowManager::GetWindow(DWORD dwID)
{
  if (dwID == WINDOW_INVALID)
  {
    return NULL;
  }

  map<DWORD, CGUIWindow *>::iterator it = m_mapWindows.find(dwID);
  if (it != m_mapWindows.end())
    return (*it).second;
  return NULL;
}

// Shows and hides modeless dialogs as necessary.
void CGUIWindowManager::UpdateModelessVisibility()
{
  for (map<DWORD, CGUIWindow *>::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
  {
    CGUIWindow *pWindow = (*it).second;
    if (pWindow && pWindow->IsDialog() && pWindow->GetVisibleCondition())
    {
      if (g_infoManager.GetBool(pWindow->GetVisibleCondition(), GetActiveWindow()))
        ((CGUIDialog *)pWindow)->Show(GetActiveWindow());
      else
        ((CGUIDialog *)pWindow)->Close();
    }
  }
}

void CGUIWindowManager::Process(bool renderOnly /*= false*/)
{
  if (m_pCallback)
  {
    if (!renderOnly)
    {
	    m_pCallback->Process();
	    m_pCallback->FrameMove();
    }
    m_pCallback->Render();
  }
}

void CGUIWindowManager::SetCallback(IWindowManagerCallback& callback)
{
  m_pCallback = &callback;
}

void CGUIWindowManager::DeInitialize()
{
  for (map<DWORD, CGUIWindow *>::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
  {
    CGUIWindow* pWindow = (*it).second;
    CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
    pWindow->OnMessage(msg);
    pWindow->FreeResources(true);
  }
  UnloadNotOnDemandWindows();

  m_vecMsgTargets.erase( m_vecMsgTargets.begin(), m_vecMsgTargets.end() );

  // destroy our custom windows...
  for (int i = 0; i < (int)m_vecCustomWindows.size(); i++)
  {
    CGUIWindow *pWindow = m_vecCustomWindows[i];
    Remove(pWindow->GetID());
    delete pWindow;
  }

  // clear our vectors of windows
  m_vecCustomWindows.clear();
  m_vecModalWindows.clear();
  m_vecModelessWindows.clear();
}

/// \brief Route to a window
/// \param pWindow Window to route to
void CGUIWindowManager::RouteToWindow(CGUIWindow* pWindow)
{
  // Just to be sure: Unroute this window,
  // #we may have routed to it before
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
    if (pWindow->GetID() == dwID)
    {
      m_vecModalWindows.erase(it);
      it = m_vecModalWindows.end();
    }
    else it++;
  }
}

bool CGUIWindowManager::IsRouted(bool includeFadeOuts /*= false */) const
{
  if (includeFadeOuts)
    return m_vecModalWindows.size() > 0;

  bool hasActiveDialog = false;
  for (unsigned int i = 0; i < m_vecModalWindows.size(); i++)
  {
    if (!m_vecModalWindows[i]->IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
    {
      hasActiveDialog = true;
      break;
    }
  }
  return hasActiveDialog;
}

bool CGUIWindowManager::IsModelessAvailable() const
{
  if (m_vecModelessWindows.size()>0)
    return true;

  return false;
}

/// \brief Get the ID of the top most routed window
/// \return dwID ID of the window or WINDOW_INVALID if no routed window available
int CGUIWindowManager::GetTopMostRoutedWindowID() const
{
  if (m_vecModalWindows.size() <= 0)
    return WINDOW_INVALID;

  return m_vecModalWindows[m_vecModalWindows.size() - 1]->GetID();
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
  while ( m_vecThreadMessages.size() > 0 )
  {
    vector<CGUIMessage*>::iterator it = m_vecThreadMessages.begin();
    CGUIMessage* pMsg = *it;
    // first remove the message from the queue,
    // else the message could be processed more then once
    it = m_vecThreadMessages.erase(it);
    
    //Leave critical section here since this can cause some thread to come back here into dispatch
    ::LeaveCriticalSection(&m_critSection );
    SendMessage( *pMsg );
    delete pMsg;
    ::EnterCriticalSection(&m_critSection );
  }

  ::LeaveCriticalSection(&m_critSection );
}

void CGUIWindowManager::AddMsgTarget( IMsgTargetCallback* pMsgTarget )
{
  m_vecMsgTargets.push_back( pMsgTarget );
}

int CGUIWindowManager::GetActiveWindow() const
{
  if (!m_windowHistory.empty())
    return m_windowHistory.top();
  return WINDOW_INVALID;
}

bool CGUIWindowManager::IsWindowActive(DWORD dwID) const
{
  if (GetActiveWindow() == dwID) return true;
  // run through the modal + modeless windows
  for (unsigned int i = 0; i < m_vecModalWindows.size(); i++)
  {
    CGUIWindow *pWindow = m_vecModalWindows[i];
    if (dwID == pWindow->GetID() && !pWindow->IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
      return true;
  }
  for (unsigned int i = 0; i < m_vecModelessWindows.size(); i++)
  {
    CGUIWindow *pWindow = m_vecModelessWindows[i];
    if (dwID == pWindow->GetID() && !pWindow->IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
      return true;
  }
  return false; // window isn't active
}

void CGUIWindowManager::LoadNotOnDemandWindows()
{
  for (map<DWORD, CGUIWindow *>::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
  {
    CGUIWindow *pWindow = (*it).second;
    if (!pWindow ->GetLoadOnDemand())
    {
      pWindow ->FreeResources(true);
      pWindow ->Initialize();
    }
  }
}

void CGUIWindowManager::UnloadNotOnDemandWindows()
{
  for (map<DWORD, CGUIWindow *>::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
  {
    CGUIWindow *pWindow = (*it).second;
    if (!pWindow->GetLoadOnDemand())
    {
      pWindow->FreeResources(true);
    }
  }
}

bool CGUIWindowManager::IsOverlayAllowed() const
{
  return m_bShowOverlay;
}

void CGUIWindowManager::ShowOverlay(bool bOnOff)
{
  m_bShowOverlay = bOnOff;
}

void CGUIWindowManager::AddToWindowHistory(DWORD newWindowID)
{
  // Check the window stack to see if this window is in our history,
  // and if so, pop all the other windows off the stack so that we
  // always have a predictable "Back" behaviour for each window
  stack<DWORD> historySave = m_windowHistory;
  while (historySave.size())
  {
    if (historySave.top() == newWindowID)
      break;
    historySave.pop();
  }
  if (!historySave.empty())
  { // found window in history
    m_windowHistory = historySave;
  }
  else
  { // didn't find window in history - add it to the stack
    m_windowHistory.push(newWindowID);
  }
} 
