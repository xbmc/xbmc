/*
 *      Copyright (C) 2012 Team XBMC
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

#include "GUIWindowPVR.h"

#include "GUIWindowPVRChannels.h"
#include "GUIWindowPVRGuide.h"
#include "GUIWindowPVRRecordings.h"
#include "GUIWindowPVRSearch.h"
#include "GUIWindowPVRTimers.h"

#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "threads/SingleLock.h"

using namespace PVR;

CGUIWindowPVR::CGUIWindowPVR(void) :
  CGUIMediaWindow(WINDOW_PVR, "MyPVR.xml"),
  m_guideGrid(NULL),
  m_currentSubwindow(NULL),
  m_savedSubwindow(NULL),
  m_windowChannelsTV(NULL),
  m_windowChannelsRadio(NULL),
  m_windowGuide(NULL),
  m_windowRecordings(NULL),
  m_windowSearch(NULL),
  m_windowTimers(NULL)
{
  m_loadType = LOAD_EVERY_TIME;
}

CGUIWindowPVR::~CGUIWindowPVR(void)
{
  Cleanup();
}

CGUIWindowPVRCommon *CGUIWindowPVR::GetActiveView(void) const
{
  CSingleLock lock(m_critSection);
  return m_currentSubwindow;
}

void CGUIWindowPVR::SetActiveView(CGUIWindowPVRCommon *window)
{
  CSingleLock lock(m_critSection);

  if ((!window && m_currentSubwindow) || (window && !m_currentSubwindow) ||
      (window->GetWindowId() != m_currentSubwindow->GetWindowId()))
  {
    // switched views, save current history
    if (m_currentSubwindow)
      m_currentSubwindow->m_history = m_history;

    // update m_history
    if (window)
      m_history = window->m_history;
    else
      m_history.ClearPathHistory();
  }
  m_currentSubwindow = window;
}

void CGUIWindowPVR::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CGUIWindowPVRCommon *view = GetActiveView();
  if (view)
    view->GetContextButtons(itemNumber, buttons);

  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

CGUIWindowPVRCommon *CGUIWindowPVR::GetSavedView(void) const
{
  CSingleLock lock(m_critSection);
  return m_savedSubwindow;
}

bool CGUIWindowPVR::OnAction(const CAction &action)
{
  CGUIWindowPVRCommon *view = GetActiveView();
  return (view && view->OnAction(action)) ||
      CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowPVR::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CGUIWindowPVRCommon *view = GetActiveView();
  return (view && view->OnContextButton(itemNumber, button)) ||
      CGUIMediaWindow::OnContextButton(itemNumber, button);
}

void CGUIWindowPVR::OnInitWindow(void)
{
  if (!g_PVRManager.IsStarted() || !g_PVRClients->HasConnectedClients())
  {
    g_windowManager.PreviousWindow();
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning,
        g_localizeStrings.Get(19045),
        g_localizeStrings.Get(19044));
    return;
  }

  CreateViews();

  CSingleLock graphicsLock(g_graphicsContext);
  SET_CONTROL_VISIBLE(CONTROL_LIST_TIMELINE);

  CSingleLock lock(m_critSection);
  if (m_savedSubwindow)
    m_savedSubwindow->OnInitWindow();
  lock.Leave();
  graphicsLock.Leave();

  CGUIMediaWindow::OnInitWindow();
}

bool CGUIWindowPVR::OnMessage(CGUIMessage& message)
{
  return (OnMessageFocus(message) ||OnMessageClick(message) ||
      CGUIMediaWindow::OnMessage(message));
}

void CGUIWindowPVR::OnWindowLoaded(void)
{
  CreateViews();

  CGUIMediaWindow::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());

  m_viewControl.AddView(GetControl(CONTROL_LIST_CHANNELS_TV));
  m_viewControl.AddView(GetControl(CONTROL_LIST_CHANNELS_RADIO));
  m_viewControl.AddView(GetControl(CONTROL_LIST_RECORDINGS));
  m_viewControl.AddView(GetControl(CONTROL_LIST_TIMERS));
  m_viewControl.AddView(GetControl(CONTROL_LIST_GUIDE_CHANNEL));
  m_viewControl.AddView(GetControl(CONTROL_LIST_GUIDE_NOW_NEXT));
  m_viewControl.AddView(GetControl(CONTROL_LIST_TIMELINE));
  m_viewControl.AddView(GetControl(CONTROL_LIST_SEARCH));
}

void CGUIWindowPVR::OnWindowUnload(void)
{
  CGUIWindowPVRCommon *view = GetActiveView();
  if (view)
  {
    view->OnWindowUnload();
    m_savedSubwindow = view;
  }
  else
  {
    m_savedSubwindow = NULL;
  }

  m_currentSubwindow = NULL;

  m_viewControl.Reset();
  CGUIMediaWindow::OnWindowUnload();
}

void CGUIWindowPVR::SetLabel(int iControl, const CStdString &strLabel)
{
  SET_CONTROL_LABEL(iControl, strLabel);
}

void CGUIWindowPVR::SetLabel(int iControl, int iLabel)
{
  SET_CONTROL_LABEL(iControl, iLabel);
}

void CGUIWindowPVR::UpdateButtons(void)
{
  m_windowGuide->UpdateButtons();
}

bool CGUIWindowPVR::OnMessageFocus(CGUIMessage &message)
{
  bool bReturn = false;

  if (message.GetMessage() == GUI_MSG_FOCUSED)
  {
    m_windowChannelsRadio->OnMessageFocus(message) ||
        m_windowChannelsTV->OnMessageFocus(message) ||
        m_windowGuide->OnMessageFocus(message) ||
        m_windowRecordings->OnMessageFocus(message) ||
        m_windowSearch->OnMessageFocus(message) ||
        m_windowTimers->OnMessageFocus(message);

    m_savedSubwindow = NULL;
  }

  return bReturn;
}

bool CGUIWindowPVR::OnMessageClick(CGUIMessage &message)
{
  bool bReturn = false;

  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    bReturn = m_windowChannelsRadio->OnClickButton(message) ||
        m_windowChannelsTV->OnClickButton(message) ||
        m_windowGuide->OnClickButton(message) ||
        m_windowRecordings->OnClickButton(message) ||
        m_windowSearch->OnClickButton(message) ||
        m_windowTimers->OnClickButton(message) ||

        m_windowChannelsRadio->OnClickList(message) ||
        m_windowChannelsTV->OnClickList(message) ||
        m_windowGuide->OnClickList(message) ||
        m_windowRecordings->OnClickList(message) ||
        m_windowSearch->OnClickList(message) ||
        m_windowTimers->OnClickList(message);
  }

  return bReturn;
}

void CGUIWindowPVR::CreateViews(void)
{
  CSingleLock lock(m_critSection);
  if (!m_windowChannelsRadio)
    m_windowChannelsRadio = new CGUIWindowPVRChannels(this, true);

  if (!m_windowChannelsTV)
    m_windowChannelsTV = new CGUIWindowPVRChannels(this, false);

  if (!m_windowGuide)
    m_windowGuide = new CGUIWindowPVRGuide(this);

  if (!m_windowRecordings)
    m_windowRecordings = new CGUIWindowPVRRecordings(this);

  if (!m_windowSearch)
    m_windowSearch = new CGUIWindowPVRSearch(this);

  if (!m_windowTimers)
    m_windowTimers = new CGUIWindowPVRTimers(this);
}

void CGUIWindowPVR::Reset(void)
{
  CSingleLock graphicsLock(g_graphicsContext);
  CSingleLock lock(m_critSection);

  Cleanup();
  CreateViews();

  m_windowChannelsRadio->ResetObservers();
  m_windowChannelsTV->ResetObservers();
  m_windowGuide->ResetObservers();
  m_windowRecordings->ResetObservers();
  m_windowTimers->ResetObservers();
}

void CGUIWindowPVR::Cleanup(void)
{
  if (m_windowChannelsRadio)
    m_windowChannelsRadio->UnregisterObservers();
  SAFE_DELETE(m_windowChannelsRadio);

  if (m_windowChannelsTV)
    m_windowChannelsTV->UnregisterObservers();
  SAFE_DELETE(m_windowChannelsTV);

  if (m_windowGuide)
    m_windowGuide->UnregisterObservers();
  SAFE_DELETE(m_windowGuide);

  if (m_windowRecordings)
    m_windowRecordings->UnregisterObservers();
  SAFE_DELETE(m_windowRecordings);

  SAFE_DELETE(m_windowSearch);

  if (m_windowTimers)
    m_windowTimers->UnregisterObservers();
  SAFE_DELETE(m_windowTimers);

  m_currentSubwindow = NULL;
  m_savedSubwindow = NULL;

  ClearFileItems();
  FreeResources();
}
