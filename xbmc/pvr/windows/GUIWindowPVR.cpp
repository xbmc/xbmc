/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#ifndef WINDOWS_GUIWINDOWPVR_H_INCLUDED
#define WINDOWS_GUIWINDOWPVR_H_INCLUDED
#include "GUIWindowPVR.h"
#endif


#ifndef WINDOWS_GUIWINDOWPVRCHANNELS_H_INCLUDED
#define WINDOWS_GUIWINDOWPVRCHANNELS_H_INCLUDED
#include "GUIWindowPVRChannels.h"
#endif

#ifndef WINDOWS_GUIWINDOWPVRGUIDE_H_INCLUDED
#define WINDOWS_GUIWINDOWPVRGUIDE_H_INCLUDED
#include "GUIWindowPVRGuide.h"
#endif

#ifndef WINDOWS_GUIWINDOWPVRRECORDINGS_H_INCLUDED
#define WINDOWS_GUIWINDOWPVRRECORDINGS_H_INCLUDED
#include "GUIWindowPVRRecordings.h"
#endif

#ifndef WINDOWS_GUIWINDOWPVRSEARCH_H_INCLUDED
#define WINDOWS_GUIWINDOWPVRSEARCH_H_INCLUDED
#include "GUIWindowPVRSearch.h"
#endif

#ifndef WINDOWS_GUIWINDOWPVRTIMERS_H_INCLUDED
#define WINDOWS_GUIWINDOWPVRTIMERS_H_INCLUDED
#include "GUIWindowPVRTimers.h"
#endif


#ifndef WINDOWS_PVR_PVRMANAGER_H_INCLUDED
#define WINDOWS_PVR_PVRMANAGER_H_INCLUDED
#include "pvr/PVRManager.h"
#endif

#ifndef WINDOWS_PVR_ADDONS_PVRCLIENTS_H_INCLUDED
#define WINDOWS_PVR_ADDONS_PVRCLIENTS_H_INCLUDED
#include "pvr/addons/PVRClients.h"
#endif

#ifndef WINDOWS_GUILIB_GUIMESSAGE_H_INCLUDED
#define WINDOWS_GUILIB_GUIMESSAGE_H_INCLUDED
#include "guilib/GUIMessage.h"
#endif

#ifndef WINDOWS_GUILIB_GUIWINDOWMANAGER_H_INCLUDED
#define WINDOWS_GUILIB_GUIWINDOWMANAGER_H_INCLUDED
#include "guilib/GUIWindowManager.h"
#endif

#ifndef WINDOWS_GUILIB_KEY_H_INCLUDED
#define WINDOWS_GUILIB_KEY_H_INCLUDED
#include "guilib/Key.h"
#endif

#ifndef WINDOWS_DIALOGS_GUIDIALOGBUSY_H_INCLUDED
#define WINDOWS_DIALOGS_GUIDIALOGBUSY_H_INCLUDED
#include "dialogs/GUIDialogBusy.h"
#endif

#ifndef WINDOWS_DIALOGS_GUIDIALOGKAITOAST_H_INCLUDED
#define WINDOWS_DIALOGS_GUIDIALOGKAITOAST_H_INCLUDED
#include "dialogs/GUIDialogKaiToast.h"
#endif

#ifndef WINDOWS_THREADS_SINGLELOCK_H_INCLUDED
#define WINDOWS_THREADS_SINGLELOCK_H_INCLUDED
#include "threads/SingleLock.h"
#endif


using namespace PVR;

#define CHANNELS_REFRESH_INTERVAL 5000

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
  m_windowTimers(NULL),
  m_bWasReset(false)
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
      (window && m_currentSubwindow && window->GetWindowId() != m_currentSubwindow->GetWindowId()))
  {
    // switched views, save current history
    if (m_currentSubwindow)
    {
      m_currentSubwindow->m_history = m_history;
      m_currentSubwindow->m_iSelected = m_viewControl.GetSelectedItem();
    }

    if (window == m_windowChannelsRadio || window == m_windowChannelsTV)
      m_refreshWatch.StartZero();

    // update m_history
    if (window)
      m_history = window->m_history;
    else
      m_history.ClearPathHistory();
  }
  m_currentSubwindow = window;
}

bool CGUIWindowPVR::Update(const CStdString &strDirectory, bool updateFilterPath)
{
  CGUIWindowPVRCommon *view = GetActiveView();

  if(view)
    view->BeforeUpdate(strDirectory);

  if(!CGUIMediaWindow::Update(strDirectory))
    return false;

  if(view)
    view->AfterUpdate(*m_unfilteredItems);

  return true;
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

bool CGUIWindowPVR::OnBack(int actionID)
{
  if (actionID == ACTION_NAV_BACK)
    // don't call CGUIMediaWindow as it will attempt to go to the parent folder which we don't want.
    return CGUIWindow::OnBack(actionID);
  return CGUIMediaWindow::OnBack(actionID);
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

  bool bReset(m_bWasReset);
  m_bWasReset = false;
  lock.Leave();
  graphicsLock.Leave();

  CGUIMediaWindow::OnInitWindow();

  if (bReset)
  {
    CGUIMessage msg(GUI_MSG_FOCUSED, GetID(), CONTROL_BTNCHANNELS_TV, 0, 0);
    OnMessageFocus(msg);
  }
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

  m_bWasReset = true;
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

void CGUIWindowPVR::FrameMove()
{
  CGUIWindowPVRCommon* view = GetActiveView();
  if (view == m_windowChannelsRadio || view == m_windowChannelsTV)
  {
    if (m_refreshWatch.GetElapsedMilliseconds() > CHANNELS_REFRESH_INTERVAL)
    {
      view->SetInvalid();
      m_refreshWatch.Reset();
    }
  }
  CGUIMediaWindow::FrameMove();
}
