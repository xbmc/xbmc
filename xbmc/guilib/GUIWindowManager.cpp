/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindowManager.h"
#include "GUIAudioManager.h"
#include "GUIDialog.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "GUIPassword.h"
#include "GUIInfoManager.h"
#include "threads/SingleLock.h"
#include "utils/URIUtils.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "addons/Skin.h"
#include "GUITexture.h"
#include "windowing/WindowingFactory.h"
#include "utils/Variant.h"

using namespace std;

CGUIWindowManager::CGUIWindowManager(void)
{
  m_pCallback = NULL;
  m_bShowOverlay = true;
  m_iNested = 0;
  m_initialized = false;
}

CGUIWindowManager::~CGUIWindowManager(void)
{
}

void CGUIWindowManager::Initialize()
{
  m_tracker.SelectAlgorithm();
  m_initialized = true;

  LoadNotOnDemandWindows();
}

bool CGUIWindowManager::SendMessage(int message, int senderID, int destID, int param1, int param2)
{
  CGUIMessage msg(message, senderID, destID, param1, param2);
  return SendMessage(msg);
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
    CSingleLock lock(g_graphicsContext);
    for (rDialog it = m_activeDialogs.rbegin(); it != m_activeDialogs.rend(); ++it)
    {
      CGUIWindow *dialog = *it;
      dialog->OnMessage(message);
    }

    for (WindowMap::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
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
  CSingleLock lock(g_graphicsContext);
  unsigned int topWindow = m_activeDialogs.size();
  while (topWindow)
  {
    CGUIWindow* dialog = m_activeDialogs[--topWindow];
    lock.Leave();
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
    lock.Enter();
    if (topWindow > m_activeDialogs.size())
      topWindow = m_activeDialogs.size();
  }
  lock.Leave();

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

bool CGUIWindowManager::SendMessage(CGUIMessage& message, int window)
{
  CGUIWindow* pWindow = GetWindow(window);
  if(pWindow)
    return pWindow->OnMessage(message);
  else
    return false;
}

void CGUIWindowManager::AddUniqueInstance(CGUIWindow *window)
{
  CSingleLock lock(g_graphicsContext);
  // increment our instance (upper word of windowID)
  // until we get a window we don't have
  int instance = 0;
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
  CSingleLock lock(g_graphicsContext);
  const vector<int>& idRange = pWindow->GetIDRange();
  for (vector<int>::const_iterator idIt = idRange.begin(); idIt != idRange.end() ; idIt++)
  {
    WindowMap::iterator it = m_mapWindows.find(*idIt);
    if (it != m_mapWindows.end())
    {
      CLog::Log(LOGERROR, "Error, trying to add a second window with id %u "
                          "to the window manager", *idIt);
      return;
    }
    m_mapWindows.insert(pair<int, CGUIWindow *>(*idIt, pWindow));
  }
}

void CGUIWindowManager::AddCustomWindow(CGUIWindow* pWindow)
{
  CSingleLock lock(g_graphicsContext);
  Add(pWindow);
  m_vecCustomWindows.push_back(pWindow);
}

void CGUIWindowManager::AddModeless(CGUIWindow* dialog)
{
  CSingleLock lock(g_graphicsContext);
  // only add the window if it's not already added
  for (iDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
    if (*it == dialog) return;
  m_activeDialogs.push_back(dialog);
}

void CGUIWindowManager::Remove(int id)
{
  CSingleLock lock(g_graphicsContext);
  WindowMap::iterator it = m_mapWindows.find(id);
  if (it != m_mapWindows.end())
  {
    for(vector<CGUIWindow*>::iterator it2 = m_activeDialogs.begin(); it2 != m_activeDialogs.end();)
    {
      if(*it2 == it->second)
        it2 = m_activeDialogs.erase(it2);
      else
        it2++;
    }

    m_mapWindows.erase(it);
  }
  else
  {
    CLog::Log(LOGWARNING, "Attempted to remove window %u "
                          "from the window manager when it didn't exist",
              id);
  }
}

// removes and deletes the window.  Should only be called
// from the class that created the window using new.
void CGUIWindowManager::Delete(int id)
{
  CSingleLock lock(g_graphicsContext);
  CGUIWindow *pWindow = GetWindow(id);
  if (pWindow)
  {
    Remove(id);
    m_deleteWindows.push_back(pWindow);
  }
}

void CGUIWindowManager::PreviousWindow()
{
  // deactivate any window
  CSingleLock lock(g_graphicsContext);
  CLog::Log(LOGDEBUG,"CGUIWindowManager::PreviousWindow: Deactivate");
  int currentWindow = GetActiveWindow();
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
  CloseWindowSync(pCurrentWindow);

  g_infoManager.SetNextWindow(WINDOW_INVALID);
  g_infoManager.SetPreviousWindow(currentWindow);

  // remove the current window off our window stack
  m_windowHistory.pop();

  // ok, initialize the new window
  CLog::Log(LOGDEBUG,"CGUIWindowManager::PreviousWindow: Activate new");
  CGUIMessage msg2(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, GetActiveWindow());
  pNewWindow->OnMessage(msg2);

  g_infoManager.SetPreviousWindow(WINDOW_INVALID);
  return;
}

void CGUIWindowManager::ChangeActiveWindow(int newWindow, const CStdString& strPath)
{
  vector<CStdString> params;
  if (!strPath.IsEmpty())
    params.push_back(strPath);
  ActivateWindow(newWindow, params, true);
}

void CGUIWindowManager::ActivateWindow(int iWindowID, const CStdString& strPath)
{
  vector<CStdString> params;
  if (!strPath.IsEmpty())
    params.push_back(strPath);
  ActivateWindow(iWindowID, params, false);
}

void CGUIWindowManager::ActivateWindow(int iWindowID, const vector<CStdString>& params, bool swappingWindows)
{
  if (!g_application.IsCurrentThread())
  {
    // make sure graphics lock is not held
    CSingleExit leaveIt(g_graphicsContext);
    CApplicationMessenger::Get().ActivateWindow(iWindowID, params, swappingWindows);
  }
  else
  {
    CSingleLock lock(g_graphicsContext);
    ActivateWindow_Internal(iWindowID, params, swappingWindows);
  }
}

void CGUIWindowManager::ActivateWindow_Internal(int iWindowID, const vector<CStdString>& params, bool swappingWindows)
{
  // translate virtual windows
  // virtual music window which returns the last open music window (aka the music start window)
  if (iWindowID == WINDOW_MUSIC)
  {
    iWindowID = g_settings.m_iMyMusicStartWindow;
    // ensure the music virtual window only returns music files and music library windows
    if (iWindowID != WINDOW_MUSIC_NAV)
      iWindowID = WINDOW_MUSIC_FILES;
  }
  // virtual video window which returns the last open video window (aka the video start window)
  if (iWindowID == WINDOW_VIDEOS || iWindowID == WINDOW_VIDEO_FILES)
  { // backward compatibility for pre-Eden
    iWindowID = WINDOW_VIDEO_NAV;
  }
  if (iWindowID == WINDOW_SCRIPTS)
  { // backward compatibility for pre-Dharma
    iWindowID = WINDOW_PROGRAMS;
  }
  if (iWindowID == WINDOW_START)
  { // virtual start window
    iWindowID = g_SkinInfo->GetStartWindow();
  }

  // debug
  CLog::Log(LOGDEBUG, "Activating window ID: %i", iWindowID);

  if (!g_passwordManager.CheckMenuLock(iWindowID))
  {
    CLog::Log(LOGERROR, "MasterCode is Wrong: Window with id %d will not be loaded! Enter a correct MasterCode!", iWindowID);
    if (GetActiveWindow() == WINDOW_INVALID && iWindowID != WINDOW_HOME)
      ActivateWindow(WINDOW_HOME);
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
    {
      CSingleExit exitit(g_graphicsContext);
      ((CGUIDialog *)pNewWindow)->DoModal(iWindowID, params.size() ? params[0] : "");
    }
    return;
  }

  g_infoManager.SetNextWindow(iWindowID);

  // set our overlay state
  HideOverlay(pNewWindow->GetOverlayState());

  // deactivate any window
  int currentWindow = GetActiveWindow();
  CGUIWindow *pWindow = GetWindow(currentWindow);
  if (pWindow)
    CloseWindowSync(pWindow, iWindowID);
  g_infoManager.SetNextWindow(WINDOW_INVALID);

  // Add window to the history list (we must do this before we activate it,
  // as all messages done in WINDOW_INIT will want to be sent to the new
  // topmost window).  If we are swapping windows, we pop the old window
  // off the history stack
  if (swappingWindows && m_windowHistory.size())
    m_windowHistory.pop();
  AddToWindowHistory(iWindowID);

  g_infoManager.SetPreviousWindow(currentWindow);
  // Send the init message
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, currentWindow, iWindowID);
  msg.SetStringParams(params);
  pNewWindow->OnMessage(msg);
//  g_infoManager.SetPreviousWindow(WINDOW_INVALID);
}

void CGUIWindowManager::CloseDialogs(bool forceClose)
{
  CSingleLock lock(g_graphicsContext);
  while (m_activeDialogs.size() > 0)
  {
    CGUIWindow* win = m_activeDialogs[0];
    win->Close(forceClose);
  }
}

bool CGUIWindowManager::OnAction(const CAction &action)
{
  CSingleLock lock(g_graphicsContext);
  unsigned int topMost = m_activeDialogs.size();
  while (topMost)
  {
    CGUIWindow *dialog = m_activeDialogs[--topMost];
    lock.Leave();
    if (dialog->IsModalDialog())
    { // we have the topmost modal dialog
      if (!dialog->IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
      {
        bool fallThrough = (dialog->GetID() == WINDOW_DIALOG_FULLSCREEN_INFO);
        if (dialog->OnAction(action))
          return true;
        // dialog didn't want the action - we'd normally return false
        // but for some dialogs we want to drop the actions through
        if (fallThrough)
          break;
        return false;
      }
      return true; // do nothing with the action until the anim is finished
    }
    // music or video overlay are handled as a special case, as they're modeless, but we allow
    // clicking on them with the mouse.
    if (action.IsMouse() && (dialog->GetID() == WINDOW_DIALOG_VIDEO_OVERLAY ||
                             dialog->GetID() == WINDOW_DIALOG_MUSIC_OVERLAY))
    {
      if (dialog->OnAction(action))
        return true;
    }
    lock.Enter();
    if (topMost > m_activeDialogs.size())
      topMost = m_activeDialogs.size();
  }
  lock.Leave();
  CGUIWindow* window = GetWindow(GetActiveWindow());
  if (window)
    return window->OnAction(action);
  return false;
}

bool RenderOrderSortFunction(CGUIWindow *first, CGUIWindow *second)
{
  return first->GetRenderOrder() < second->GetRenderOrder();
}

void CGUIWindowManager::Process(unsigned int currentTime)
{
  assert(g_application.IsCurrentThread());
  CSingleLock lock(g_graphicsContext);

  CDirtyRegionList dirtyregions;

  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->DoProcess(currentTime, dirtyregions);

  // process all dialogs - visibility may change etc.
  for (WindowMap::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
  {
    CGUIWindow *pWindow = (*it).second;
    if (pWindow && pWindow->IsDialog())
      pWindow->DoProcess(currentTime, dirtyregions);
  }

  if (g_application.m_AppActive)
  {
    for (CDirtyRegionList::iterator itr = dirtyregions.begin(); itr != dirtyregions.end(); itr++)
      m_tracker.MarkDirtyRegion(*itr);
  }
}

void CGUIWindowManager::MarkDirty()
{
  m_tracker.MarkDirtyRegion(CRect(0, 0, (float)g_graphicsContext.GetWidth(), (float)g_graphicsContext.GetHeight()));
}

void CGUIWindowManager::MarkDirty(const CRect& rect)
{
  m_tracker.MarkDirtyRegion(rect);
}

void CGUIWindowManager::RenderPass()
{
  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
  {
    pWindow->ClearBackground();
    pWindow->DoRender();
  }

  // we render the dialogs based on their render order.
  vector<CGUIWindow *> renderList = m_activeDialogs;
  stable_sort(renderList.begin(), renderList.end(), RenderOrderSortFunction);
  
  for (iDialog it = renderList.begin(); it != renderList.end(); ++it)
  {
    if ((*it)->IsDialogRunning())
      (*it)->DoRender();
  }
}

bool CGUIWindowManager::Render()
{
  assert(g_application.IsCurrentThread());
  CSingleLock lock(g_graphicsContext);

  CDirtyRegionList dirtyRegions = m_tracker.GetDirtyRegions();

  bool hasRendered = false;
  // If we visualize the regions we will always render the entire viewport
  if (g_advancedSettings.m_guiVisualizeDirtyRegions || g_advancedSettings.m_guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_FILL_VIEWPORT_ALWAYS)
  {
    RenderPass();
    hasRendered = true;
  }
  else if (g_advancedSettings.m_guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_FILL_VIEWPORT_ON_CHANGE)
  {
    if (dirtyRegions.size() > 0)
    {
      RenderPass();
      hasRendered = true;
    }
  }
  else
  {
    for (CDirtyRegionList::const_iterator i = dirtyRegions.begin(); i != dirtyRegions.end(); i++)
    {
      if (i->IsEmpty())
        continue;

      g_graphicsContext.SetScissors(*i);
      RenderPass();
      hasRendered = true;
    }
    g_graphicsContext.ResetScissors();
  }

  if (g_advancedSettings.m_guiVisualizeDirtyRegions)
  {
    g_graphicsContext.SetRenderingResolution(g_graphicsContext.GetResInfo(), false);
    const CDirtyRegionList &markedRegions  = m_tracker.GetMarkedRegions(); 
    for (CDirtyRegionList::const_iterator i = markedRegions.begin(); i != markedRegions.end(); i++)
      CGUITexture::DrawQuad(*i, 0x0fff0000);
    for (CDirtyRegionList::const_iterator i = dirtyRegions.begin(); i != dirtyRegions.end(); i++)
      CGUITexture::DrawQuad(*i, 0x4c00ff00);
  }

  m_tracker.CleanMarkedRegions();

  return hasRendered;
}

void CGUIWindowManager::FrameMove()
{
  assert(g_application.IsCurrentThread());
  CSingleLock lock(g_graphicsContext);

  if(m_iNested == 0)
  {
    // delete any windows queued for deletion
    for(iDialog it = m_deleteWindows.begin(); it != m_deleteWindows.end(); it++)
    {
      // Free any window resources
      (*it)->FreeResources(true);
      delete *it;
    }
    m_deleteWindows.clear();
  }

  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->FrameMove();
  // update any dialogs - we take a copy of the vector as some dialogs may close themselves
  // during this call
  vector<CGUIWindow *> dialogs = m_activeDialogs;
  for (iDialog it = dialogs.begin(); it != dialogs.end(); ++it)
    (*it)->FrameMove();
}

CGUIWindow* CGUIWindowManager::GetWindow(int id) const
{
  if (id == WINDOW_INVALID)
  {
    return NULL;
  }

  CSingleLock lock(g_graphicsContext);
  WindowMap::const_iterator it = m_mapWindows.find(id);
  if (it != m_mapWindows.end())
    return (*it).second;
  return NULL;
}

void CGUIWindowManager::ProcessRenderLoop(bool renderOnly /*= false*/)
{
  if (g_application.IsCurrentThread() && m_pCallback)
  {
    m_iNested++;
    if (!renderOnly)
      m_pCallback->Process();
    m_pCallback->FrameMove(!renderOnly);
    m_pCallback->Render();
    m_iNested--;
  }
}

void CGUIWindowManager::SetCallback(IWindowManagerCallback& callback)
{
  m_pCallback = &callback;
}

void CGUIWindowManager::DeInitialize()
{
  CSingleLock lock(g_graphicsContext);
  for (WindowMap::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
  {
    CGUIWindow* pWindow = (*it).second;
    if (IsWindowActive(it->first))
    {
      pWindow->DisableAnimations();
      pWindow->Close(true);
    }
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

  m_initialized = false;
}

/// \brief Route to a window
/// \param pWindow Window to route to
void CGUIWindowManager::RouteToWindow(CGUIWindow* dialog)
{
  CSingleLock lock(g_graphicsContext);
  // Just to be sure: Unroute this window,
  // #we may have routed to it before
  RemoveDialog(dialog->GetID());

  m_activeDialogs.push_back(dialog);
}

/// \brief Unroute window
/// \param id ID of the window routed
void CGUIWindowManager::RemoveDialog(int id)
{
  CSingleLock lock(g_graphicsContext);
  for (iDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    if ((*it)->GetID() == id)
    {
      m_activeDialogs.erase(it);
      return;
    }
  }
}

bool CGUIWindowManager::HasModalDialog() const
{
  CSingleLock lock(g_graphicsContext);
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
/// \return id ID of the window or WINDOW_INVALID if no routed window available
int CGUIWindowManager::GetTopMostModalDialogID(bool ignoreClosing /*= false*/) const
{
  CSingleLock lock(g_graphicsContext);
  for (crDialog it = m_activeDialogs.rbegin(); it != m_activeDialogs.rend(); ++it)
  {
    CGUIWindow *dialog = *it;
    if (dialog->IsModalDialog() && (!ignoreClosing || !dialog->IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
    { // have a modal window
      return dialog->GetID();
    }
  }
  return WINDOW_INVALID;
}

void CGUIWindowManager::SendThreadMessage(CGUIMessage& message)
{
  CSingleLock lock(m_critSection);

  CGUIMessage* msg = new CGUIMessage(message);
  m_vecThreadMessages.push_back( pair<CGUIMessage*,int>(msg,0) );
}

void CGUIWindowManager::SendThreadMessage(CGUIMessage& message, int window)
{
  CSingleLock lock(m_critSection);

  CGUIMessage* msg = new CGUIMessage(message);
  m_vecThreadMessages.push_back( pair<CGUIMessage*,int>(msg,window) );
}

void CGUIWindowManager::DispatchThreadMessages()
{
  CSingleLock lock(m_critSection);
  vector< pair<CGUIMessage*,int> > messages(m_vecThreadMessages);
  m_vecThreadMessages.erase(m_vecThreadMessages.begin(), m_vecThreadMessages.end());
  lock.Leave();

  while ( messages.size() > 0 )
  {
    vector< pair<CGUIMessage*,int> >::iterator it = messages.begin();
    CGUIMessage* pMsg = it->first;
    int window = it->second;
    // first remove the message from the queue,
    // else the message could be processed more then once
    it = messages.erase(it);

    if (window)
      SendMessage( *pMsg, window );
    else
      SendMessage( *pMsg );
    delete pMsg;
  }
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

// same as GetActiveWindow() except it first grabs dialogs
int CGUIWindowManager::GetFocusedWindow() const
{
  int dialog = GetTopMostModalDialogID(true);
  if (dialog != WINDOW_INVALID)
    return dialog;

  return GetActiveWindow();
}

bool CGUIWindowManager::IsWindowActive(int id, bool ignoreClosing /* = true */) const
{
  // mask out multiple instances of the same window
  id &= WINDOW_ID_MASK;
  if ((GetActiveWindow() & WINDOW_ID_MASK) == id) return true;
  // run through the dialogs
  CSingleLock lock(g_graphicsContext);
  for (ciDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    CGUIWindow *window = *it;
    if ((window->GetID() & WINDOW_ID_MASK) == id && (!ignoreClosing || !window->IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
      return true;
  }
  return false; // window isn't active
}

bool CGUIWindowManager::IsWindowActive(const CStdString &xmlFile, bool ignoreClosing /* = true */) const
{
  CSingleLock lock(g_graphicsContext);
  CGUIWindow *window = GetWindow(GetActiveWindow());
  if (window && URIUtils::GetFileName(window->GetProperty("xmlfile").asString()).Equals(xmlFile)) return true;
  // run through the dialogs
  for (ciDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    CGUIWindow *window = *it;
    if (URIUtils::GetFileName(window->GetProperty("xmlfile").asString()).Equals(xmlFile) && (!ignoreClosing || !window->IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
      return true;
  }
  return false; // window isn't active
}

bool CGUIWindowManager::IsWindowVisible(int id) const
{
  return IsWindowActive(id, false);
}

bool CGUIWindowManager::IsWindowVisible(const CStdString &xmlFile) const
{
  return IsWindowActive(xmlFile, false);
}

void CGUIWindowManager::LoadNotOnDemandWindows()
{
  CSingleLock lock(g_graphicsContext);
  for (WindowMap::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
  {
    CGUIWindow *pWindow = (*it).second;
    if (pWindow->GetLoadType() == CGUIWindow::LOAD_ON_GUI_INIT)
    {
      pWindow->FreeResources(true);
      pWindow->Initialize();
    }
  }
}

void CGUIWindowManager::UnloadNotOnDemandWindows()
{
  CSingleLock lock(g_graphicsContext);
  for (WindowMap::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); it++)
  {
    CGUIWindow *pWindow = (*it).second;
    if (pWindow->GetLoadType() == CGUIWindow::LOAD_ON_GUI_INIT ||
        pWindow->GetLoadType() == CGUIWindow::KEEP_IN_MEMORY)
    {
      pWindow->FreeResources(true);
    }
  }
}

bool CGUIWindowManager::IsOverlayAllowed() const
{
  if (GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
      GetActiveWindow() == WINDOW_SCREENSAVER)
    return false;
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

void CGUIWindowManager::AddToWindowHistory(int newWindowID)
{
  // Check the window stack to see if this window is in our history,
  // and if so, pop all the other windows off the stack so that we
  // always have a predictable "Back" behaviour for each window
  stack<int> historySave = m_windowHistory;
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

void CGUIWindowManager::GetActiveModelessWindows(vector<int> &ids)
{
  // run through our modeless windows, and construct a vector of them
  // useful for saving and restoring the modeless windows on skin change etc.
  CSingleLock lock(g_graphicsContext);
  for (iDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    if (!(*it)->IsModalDialog())
      ids.push_back((*it)->GetID());
  }
}

CGUIWindow *CGUIWindowManager::GetTopMostDialog() const
{
  CSingleLock lock(g_graphicsContext);
  // find the window with the lowest render order
  vector<CGUIWindow *> renderList = m_activeDialogs;
  stable_sort(renderList.begin(), renderList.end(), RenderOrderSortFunction);

  if (!renderList.size())
    return NULL;

  // return the last window in the list
  return *renderList.rbegin();
}

bool CGUIWindowManager::IsWindowTopMost(int id) const
{
  CGUIWindow *topMost = GetTopMostDialog();
  if (topMost && (topMost->GetID() & WINDOW_ID_MASK) == id)
    return true;
  return false;
}

bool CGUIWindowManager::IsWindowTopMost(const CStdString &xmlFile) const
{
  CGUIWindow *topMost = GetTopMostDialog();
  if (topMost && URIUtils::GetFileName(topMost->GetProperty("xmlfile").asString()).Equals(xmlFile))
    return true;
  return false;
}

void CGUIWindowManager::ClearWindowHistory()
{
  while (m_windowHistory.size())
    m_windowHistory.pop();
}

void CGUIWindowManager::CloseWindowSync(CGUIWindow *window, int nextWindowID /*= 0*/)
{
  window->Close(false, nextWindowID);
  while (window->IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
    ProcessRenderLoop(true);
}

#ifdef _DEBUG
void CGUIWindowManager::DumpTextureUse()
{
  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->DumpTextureUse();

  CSingleLock lock(g_graphicsContext);
  for (iDialog it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    if ((*it)->IsDialogRunning())
      (*it)->DumpTextureUse();
  }
}
#endif
