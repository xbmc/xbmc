#include "include.h"
#include "GUIWindowManager.h"
#include "GUIAudioManager.h"
#include "GUIDialog.h"
#include "../xbmc/Settings.h"
#include "../xbmc/GUIPassword.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "../xbmc/Util.h"

#if !defined(_XBOX) && !defined(_LINUX)
#include "../Tools/Win32/XBMC_PC.h"
#endif

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
//  CLog::Log(LOGDEBUG,"SendMessage: mess=%d send=%d control=%d param1=%d", message.GetMessage(), message.GetSenderId(), message.GetControlId(), message.GetParam1());
  // Send the message to all none window targets
  for (int i = 0; i < (int) m_vecMsgTargets.size(); i++)
  {
    IMsgTargetCallback* pMsgTarget = m_vecMsgTargets[i];

    if (pMsgTarget)
    {
      if (pMsgTarget->OnMessage( message )) handled = true;
    }
  }

  //  A GUI_MSG_NOTIFY_ALL is send to any active modal dialog
  //  and all windows whether they are active or not
  if (message.GetMessage()==GUI_MSG_NOTIFY_ALL)
  {
    for (rDialog it = m_activeDialogs.rbegin(); it != m_activeDialogs.rend(); ++it)
    {
      CGUIWindow *dialog = *it;
      dialog->OnMessage(message);
    }

    for (map<DWORD, CGUIWindow *>::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
    {
      CGUIWindow *pWindow = (*it).second;
      pWindow->OnMessage(message);
    }
    return true;
  }

  // Normal messages are sent to:
  // 1. All active modeless dialogs
  // 2. The topmost dialog that accepts the message
  // 3. The underlying window (only if it is the sender or receiver if a modal dialog is active)

  bool hasModalDialog(false);
  bool modalAcceptedMessage(false);
  // don't use an iterator for this loop, as some messages mean that m_activeDialogs is altered,
  // which will invalidate any iterator
  unsigned int topWindow = m_activeDialogs.size();
  while (topWindow)
  {
    CGUIWindow* dialog = m_activeDialogs[--topWindow];
    if (!modalAcceptedMessage && dialog->IsModalDialog())
    { // modal window
      hasModalDialog = true;
      if (!modalAcceptedMessage && dialog->OnMessage( message ))
      {
        modalAcceptedMessage = handled = true;
      }
    }
    else if (!dialog->IsModalDialog())
    { // modeless
      if (dialog->OnMessage( message ))
        handled = true;
    }
  }

  // now send to the underlying window
  CGUIWindow* window = GetWindow(GetActiveWindow());
  if (window)
  {
    if (hasModalDialog)
    {
      // only send the message to the underlying window if it's the recipient
      // or sender (or we have no sender)
      if (message.GetSenderId() == window->GetID() ||
          message.GetControlId() == window->GetID() ||
          message.GetSenderId() == 0 )
      {
        if (window->OnMessage(message)) handled = true;
      }
    }
    else
    {
      if (window->OnMessage(message)) handled = true;
    }
  }
  return handled;
}

bool CGUIWindowManager::SendMessage(CGUIMessage& message, DWORD dwWindow)
{
  CGUIWindow* pWindow = GetWindow(dwWindow);
  if(pWindow)
    return pWindow->OnMessage(message);
  else
    return false;
}

void CGUIWindowManager::AddUniqueInstance(CGUIWindow *window)
{
  // increment our instance (upper word of windowID)
  // until we get a window we don't have
  DWORD instance = 0;
  while (GetWindow(window->GetID()))
    window->SetID(window->GetID() + (++instance << 16));
  Add(window);
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
      CLog::Log(LOGERROR, "Error, trying to add a second window with id %lu to the window manager", pWindow->GetID());
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

void CGUIWindowManager::AddModeless(CGUIWindow* dialog)
{
  // only add the window if it's not already added
  for (iDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
    if (*it == dialog) return;
  m_activeDialogs.push_back(dialog);
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
    CLog::Log(LOGWARNING, "Attempted to remove window %lu from the window manager when it didn't exist", dwID);
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

void CGUIWindowManager::PreviousWindow()
{
  // deactivate any window
  CLog::Log(LOGDEBUG,"CGUIWindowManager::PreviousWindow: Deactivate");
  DWORD currentWindow = GetActiveWindow();
  CGUIWindow *pCurrentWindow = GetWindow(currentWindow);
  if (!pCurrentWindow)
    return;     // no windows or window history yet

  // check to see whether our current window has a <previouswindow> tag
  if (pCurrentWindow->GetPreviousWindow() != WINDOW_INVALID)
  {
    // TODO: we may need to test here for the
    //       whether our history should be changed

    // don't reactivate the previouswindow if it is ourselves.
    if (currentWindow != pCurrentWindow->GetPreviousWindow())
      ActivateWindow(pCurrentWindow->GetPreviousWindow());
    return;
  }
  // get the previous window in our stack
  if (m_windowHistory.size() < 2)
  { // no previous window history yet - check if we should just activate home
    if (GetActiveWindow() != WINDOW_INVALID && GetActiveWindow() != WINDOW_HOME)
    {
      ClearWindowHistory();
      ActivateWindow(WINDOW_HOME);
    }
    return;
  }
  m_windowHistory.pop();
  int previousWindow = GetActiveWindow();
  m_windowHistory.push(currentWindow);

  CGUIWindow *pNewWindow = GetWindow(previousWindow);
  if (!pNewWindow)
  {
    CLog::Log(LOGERROR, "Unable to activate the previous window");
    ClearWindowHistory();
    ActivateWindow(WINDOW_HOME);
    return;
  }

  // ok to go to the previous window now

  // tell our info manager which window we are going to
  g_infoManager.SetNextWindow(previousWindow);

  // set our overlay state (enables out animations on window change)
  HideOverlay(pNewWindow->GetOverlayState());

  // deinitialize our window
  g_audioManager.PlayWindowSound(pCurrentWindow->GetID(), SOUND_DEINIT);
  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
  pCurrentWindow->OnMessage(msg);

  g_infoManager.SetNextWindow(WINDOW_INVALID);
  g_infoManager.SetPreviousWindow(currentWindow);

  // remove the current window off our window stack
  m_windowHistory.pop();

  // ok, initialize the new window
  CLog::Log(LOGDEBUG,"CGUIWindowManager::PreviousWindow: Activate new");
  g_audioManager.PlayWindowSound(pNewWindow->GetID(), SOUND_INIT);
  CGUIMessage msg2(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, GetActiveWindow());
  pNewWindow->OnMessage(msg2);

  g_infoManager.SetPreviousWindow(WINDOW_INVALID);
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
    iWindowID = g_stSettings.m_iVideoStartWindow;
    // ensure the virtual video window only returns video windows
    if (iWindowID != WINDOW_VIDEO_NAV)
      iWindowID = WINDOW_VIDEO_FILES;
  }

  // debug
  CLog::Log(LOGDEBUG, "Activating window ID: %i", iWindowID);

  if(!g_passwordManager.CheckMenuLock(iWindowID))
  {
    CLog::Log(LOGERROR, "MasterCode is Wrong: Window with id %d will not be loaded! Enter a correct MasterCode!", iWindowID);
    return;
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
    if (!pNewWindow->IsDialogRunning())
      ((CGUIDialog *)pNewWindow)->DoModal(iWindowID);
    return;
  }

  g_infoManager.SetNextWindow(iWindowID);

  // set our overlay state
  HideOverlay(pNewWindow->GetOverlayState());

  // deactivate any window
  int currentWindow = GetActiveWindow();
  CGUIWindow *pWindow = GetWindow(currentWindow);
  if (pWindow)
  {
    //  Play the window specific deinit sound
    g_audioManager.PlayWindowSound(pWindow->GetID(), SOUND_DEINIT);
    CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, iWindowID);
    pWindow->OnMessage(msg);
  }
  g_infoManager.SetNextWindow(WINDOW_INVALID);

  // Add window to the history list (we must do this before we activate it,
  // as all messages done in WINDOW_INIT will want to be sent to the new
  // topmost window).  If we are swapping windows, we pop the old window
  // off the history stack
  if (swappingWindows && m_windowHistory.size())
    m_windowHistory.pop();
  AddToWindowHistory(iWindowID);

  g_infoManager.SetPreviousWindow(currentWindow);
  g_audioManager.PlayWindowSound(pNewWindow->GetID(), SOUND_INIT);
  // Send the init message
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, GetActiveWindow(), iWindowID);
  if (!strPath.IsEmpty()) msg.SetStringParam(strPath);
  pNewWindow->OnMessage(msg);
//  g_infoManager.SetPreviousWindow(WINDOW_INVALID);
}

void CGUIWindowManager::CloseDialogs(bool forceClose)
{
  while (m_activeDialogs.size() > 0)
  {
    CGUIDialog* dialog = (CGUIDialog *)m_activeDialogs[0];
    dialog->Close(forceClose);
  }
}

bool CGUIWindowManager::OnAction(const CAction &action)
{
  for (rDialog it = m_activeDialogs.rbegin(); it != m_activeDialogs.rend(); ++it)
  {
    CGUIWindow *dialog = *it;
    if (dialog->IsModalDialog())
    { // we have the topmost modal dialog
      if (!dialog->IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
        return dialog->OnAction(action);
      return true; // do nothing with the action until the anim is finished
    }
  }
  CGUIWindow* window = GetWindow(GetActiveWindow());
  if (window)
    return window->OnAction(action);
  return false;
}

void CGUIWindowManager::Render()
{
  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->Render();
}

bool RenderOrderSortFunction(CGUIWindow *first, CGUIWindow *second)
{
  return first->GetRenderOrder() < second->GetRenderOrder();
}

void CGUIWindowManager::RenderDialogs()
{
  // find the window with the lowest render order
  vector<CGUIWindow *> renderList = m_activeDialogs;
  stable_sort(renderList.begin(), renderList.end(), RenderOrderSortFunction);

  // iterate through and render if they're running
  for (iDialog it = renderList.begin(); it != renderList.end(); ++it)
  {
    if ((*it)->IsDialogRunning())
      (*it)->Render();
  }
}

CGUIWindow* CGUIWindowManager::GetWindow(DWORD dwID) const
{
  if (dwID == WINDOW_INVALID)
  {
    return NULL;
  }

  map<DWORD, CGUIWindow *>::const_iterator it = m_mapWindows.find(dwID);
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
        ((CGUIDialog *)pWindow)->Show();
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
#if defined(WIN32) && !defined(HAS_SDL)
    extern CXBMC_PC *g_xbmcPC;
    g_xbmcPC->ProcessMessage(NULL);
    Sleep(0);
#endif
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
    pWindow->ResetControlStates();
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
  m_activeDialogs.clear();
}

/// \brief Route to a window
/// \param pWindow Window to route to
void CGUIWindowManager::RouteToWindow(CGUIWindow* dialog)
{
  // Just to be sure: Unroute this window,
  // #we may have routed to it before
  RemoveDialog(dialog->GetID());

  m_activeDialogs.push_back(dialog);
}

/// \brief Unroute window
/// \param dwID ID of the window routed
void CGUIWindowManager::RemoveDialog(DWORD dwID)
{
  for (iDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    if ((*it)->GetID() == dwID)
    {
      m_activeDialogs.erase(it);
      return;
    }
  }
}

bool CGUIWindowManager::HasModalDialog() const
{
  for (ciDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    CGUIWindow *window = *it;
    if (window->IsModalDialog())
    { // have a modal window
      if (!window->IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
        return true;
    }
  }
  return false;
}

bool CGUIWindowManager::HasDialogOnScreen() const
{
  return (m_activeDialogs.size() > 0);
}

/// \brief Get the ID of the top most routed window
/// \return dwID ID of the window or WINDOW_INVALID if no routed window available
int CGUIWindowManager::GetTopMostModalDialogID() const
{
  for (crDialog it = m_activeDialogs.rbegin(); it != m_activeDialogs.rend(); ++it)
  {
    CGUIWindow *dialog = *it;
    if (dialog->IsModalDialog())
    { // have a modal window
      return dialog->GetID();
    }
  }
  return WINDOW_INVALID;
}

void CGUIWindowManager::SendThreadMessage(CGUIMessage& message)
{
  ::EnterCriticalSection(&m_critSection );

  CGUIMessage* msg = new CGUIMessage(message);
  m_vecThreadMessages.push_back( pair<CGUIMessage*,DWORD>(msg,0) );

  ::LeaveCriticalSection(&m_critSection );
}

void CGUIWindowManager::SendThreadMessage(CGUIMessage& message, DWORD dwWindow)
{
  ::EnterCriticalSection(&m_critSection );

  CGUIMessage* msg = new CGUIMessage(message);
  m_vecThreadMessages.push_back( pair<CGUIMessage*,DWORD>(msg,dwWindow) );

  ::LeaveCriticalSection(&m_critSection );
}

void CGUIWindowManager::DispatchThreadMessages()
{
  ::EnterCriticalSection(&m_critSection );
  while ( m_vecThreadMessages.size() > 0 )
  {
    vector< pair<CGUIMessage*,DWORD> >::iterator it = m_vecThreadMessages.begin();
    CGUIMessage* pMsg = it->first;
    DWORD dwWindow = it->second;
    // first remove the message from the queue,
    // else the message could be processed more then once
    it = m_vecThreadMessages.erase(it);
    
    //Leave critical section here since this can cause some thread to come back here into dispatch
    ::LeaveCriticalSection(&m_critSection );
    if(dwWindow)
      SendMessage( *pMsg, dwWindow );
    else
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

bool CGUIWindowManager::IsWindowActive(DWORD dwID, bool ignoreClosing /* = true */) const
{
  // mask out multiple instances of the same window
  dwID &= WINDOW_ID_MASK;
  if ((GetActiveWindow() & WINDOW_ID_MASK) == dwID) return true;
  // run through the dialogs
  for (ciDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    CGUIWindow *window = *it;
    if ((window->GetID() & WINDOW_ID_MASK) == dwID && (!ignoreClosing || !window->IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
      return true;
  }
  return false; // window isn't active
}

bool CGUIWindowManager::IsWindowActive(const CStdString &xmlFile, bool ignoreClosing /* = true */) const
{
  CGUIWindow *window = GetWindow(GetActiveWindow());
  if (window && CUtil::GetFileName(window->GetXMLFile()).Equals(xmlFile)) return true;
  // run through the dialogs
  for (ciDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    CGUIWindow *window = *it;
    if (CUtil::GetFileName(window->GetXMLFile()).Equals(xmlFile) && (!ignoreClosing || !window->IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
      return true;
  }
  return false; // window isn't active
}

bool CGUIWindowManager::IsWindowVisible(DWORD id) const
{
  return IsWindowActive(id, false);
}

bool CGUIWindowManager::IsWindowVisible(const CStdString &xmlFile) const
{
  return IsWindowActive(xmlFile, false);
}

void CGUIWindowManager::LoadNotOnDemandWindows()
{
  for (map<DWORD, CGUIWindow *>::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
  {
    CGUIWindow *pWindow = (*it).second;
    if (!pWindow ->GetLoadOnDemand())
    {
      pWindow->FreeResources(true);
      pWindow->Initialize();
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

void CGUIWindowManager::ShowOverlay(CGUIWindow::OVERLAY_STATE state)
{
  if (state != CGUIWindow::OVERLAY_STATE_PARENT_WINDOW)
    m_bShowOverlay = state == CGUIWindow::OVERLAY_STATE_SHOWN;
}

void CGUIWindowManager::HideOverlay(CGUIWindow::OVERLAY_STATE state)
{
  if (state == CGUIWindow::OVERLAY_STATE_HIDDEN)
    m_bShowOverlay = false;
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

void CGUIWindowManager::GetActiveModelessWindows(vector<DWORD> &ids)
{
  // run through our modeless windows, and construct a vector of them
  // useful for saving and restoring the modeless windows on skin change etc.
  for (iDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    if (!(*it)->IsModalDialog())
      ids.push_back((*it)->GetID());
  }
}

CGUIWindow *CGUIWindowManager::GetTopMostDialog() const
{
  // find the window with the lowest render order
  vector<CGUIWindow *> renderList = m_activeDialogs;
  stable_sort(renderList.begin(), renderList.end(), RenderOrderSortFunction);

  if (!renderList.size())
    return NULL;

  // return the last window in the list
  return *renderList.rbegin();
}

bool CGUIWindowManager::IsWindowTopMost(DWORD id) const
{
  CGUIWindow *topMost = GetTopMostDialog();
  if (topMost && (topMost->GetID() & WINDOW_ID_MASK) == id)
    return true;
  return false;
}

bool CGUIWindowManager::IsWindowTopMost(const CStdString &xmlFile) const
{
  CGUIWindow *topMost = GetTopMostDialog();
  if (topMost && CUtil::GetFileName(topMost->GetXMLFile()).Equals(xmlFile))
    return true;
  return false;
}

void CGUIWindowManager::ClearWindowHistory()
{
  while (m_windowHistory.size())
    m_windowHistory.pop();
}

#ifdef _DEBUG
void CGUIWindowManager::DumpTextureUse()
{
  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->DumpTextureUse();

  for (iDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    if ((*it)->IsDialogRunning())
      (*it)->DumpTextureUse();
  }
}
#endif
