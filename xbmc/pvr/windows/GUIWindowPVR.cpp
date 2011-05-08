/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogBusy.h"
#include "threads/SingleLock.h"

using namespace PVR;

CGUIWindowPVR::CGUIWindowPVR(void) :
  CGUIMediaWindow(WINDOW_PVR, "MyPVR.xml")
{
  m_guideGrid        = NULL;
  m_bViewsCreated    = false;
  m_currentSubwindow = NULL;
  m_savedSubwindow   = NULL;
  m_bDialogOKActive  = false;
}

CGUIWindowPVR::~CGUIWindowPVR(void)
{
  if (m_bViewsCreated)
  {
    delete m_windowChannelsRadio;
    delete m_windowChannelsTV;
    delete m_windowGuide;
    delete m_windowRecordings;
    delete m_windowSearch;
    delete m_windowTimers;
  }
}

CGUIWindowPVRCommon *CGUIWindowPVR::GetActiveView(void) const
{
  CSingleLock lock(m_critSection);
  if (!m_bViewsCreated)
    return NULL;

  return m_currentSubwindow;
}

void CGUIWindowPVR::SetActiveView(CGUIWindowPVRCommon *window)
{
  CSingleLock lock(m_critSection);
  if (!m_bViewsCreated)
    return;

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
  if (!m_bViewsCreated)
    return NULL;

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
  if (!g_PVRManager.IsStarted() || !g_PVRClients->HasActiveClients())
  {
    m_bDialogOKActive = true;
    g_windowManager.PreviousWindow();
    CGUIDialogOK::ShowAndGetInput(19033,0,19045,19044);
    m_bDialogOKActive = false;
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
  if (!m_bViewsCreated)
  {
    m_bViewsCreated = true;

    m_windowChannelsRadio = new CGUIWindowPVRChannels(this, true);
    m_windowChannelsTV    = new CGUIWindowPVRChannels(this, false);
    m_windowGuide         = new CGUIWindowPVRGuide(this);
    m_windowRecordings    = new CGUIWindowPVRRecordings(this);
    m_windowSearch        = new CGUIWindowPVRSearch(this);
    m_windowTimers        = new CGUIWindowPVRTimers(this);
  }
}

void CGUIWindowPVR::UnlockWindow(void)
{
  if (m_bDialogOKActive)
  {
    CGUIDialogOK *dialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (dialog)
    {
      dialog->Close();
      g_windowManager.ActivateWindow(WINDOW_PVR);
    }
  }
}

void CGUIWindowPVR::Reset(void)
{
  CSingleLock graphicsLock(g_graphicsContext);
  CSingleLock lock(m_critSection);

  if (m_bViewsCreated)
  {
    delete m_windowChannelsRadio;
    delete m_windowChannelsTV;
    delete m_windowGuide;
    delete m_windowRecordings;
    delete m_windowSearch;
    delete m_windowTimers;
    m_bViewsCreated = false;
  }

  CreateViews();

  m_windowChannelsRadio->ResetObservers();
  m_windowChannelsTV->ResetObservers();
  m_windowGuide->ResetObservers();
  m_windowRecordings->ResetObservers();
  m_windowTimers->ResetObservers();
}
