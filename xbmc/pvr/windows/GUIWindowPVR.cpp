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

#include "GUIWindowPVR.h"

#include "GUIWindowPVRChannels.h"
#include "GUIWindowPVRGuide.h"
#include "GUIWindowPVRRecordings.h"
#include "GUIWindowPVRSearch.h"
#include "GUIWindowPVRTimers.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/addons/PVRClients.h"
#include "guilib/Key.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Key.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSelect.h"
#include "threads/SingleLock.h"

using namespace PVR;

#define CHANNELS_REFRESH_INTERVAL 5000

CGUIWindowPVR::CGUIWindowPVR(int windowId, const char *xmlFile, bool bRadio) :
  CGUIMediaWindow(windowId, xmlFile),
  m_guideGrid(NULL),
  m_currentSubwindow(NULL),
  m_savedSubwindow(NULL),
  m_windowChannels(NULL),
  m_windowGuide(NULL),
  m_windowRecordings(NULL),
  m_windowSearch(NULL),
  m_windowTimers(NULL),
  m_bWasReset(false),
  m_bRadio(bRadio)
{
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

    if (window == m_windowChannels)
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
  switch (action.GetID())
  {
  case ACTION_PREVIOUS_CHANNELGROUP:
  case ACTION_NEXT_CHANNELGROUP:
    {
      // switch to next or previous group
      CPVRChannelGroupPtr group = GetSelectedGroup();
      CPVRChannelGroupPtr nextGroup = action.GetID() == ACTION_NEXT_CHANNELGROUP ? group->GetNextGroup() : group->GetPreviousGroup();
      SetSelectedGroup(nextGroup);
      CGUIWindowPVRCommon *view = GetActiveView();
      if(view)
        view->UpdateData();
      return true;
    }
  }

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
    CGUIMessage msg(GUI_MSG_CLICKED, GetID(), CONTROL_BTNCHANNELS, 0, 0);
    OnMessage(msg);
  }
}

bool CGUIWindowPVR::OnMessage(CGUIMessage& message)
{
  bool bReturn = false;
  
  if(m_windowChannels == NULL)
    return false;
  
  bReturn |= OnMessageClick(message);
  bReturn |= CGUIMediaWindow::OnMessage(message);
  bReturn |= m_windowChannels->OnMessage(message) ||
    m_windowGuide->OnMessage(message) ||
    m_windowRecordings->OnMessage(message) ||
    m_windowTimers->OnMessage(message) ||
    m_windowSearch->OnMessage(message);
  
  return bReturn;
}

void CGUIWindowPVR::OnWindowLoaded(void)
{
  CreateViews();

  CGUIMediaWindow::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());

  m_viewControl.AddView(GetControl(CONTROL_LIST_CHANNELS));
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

bool CGUIWindowPVR::OnMessageClick(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    switch (message.GetSenderId())
    {
      case CONTROL_BTNCHANNEL_GROUPS:
        return OpenGroupDialogSelect();

      case CONTROL_BTNCHANNELS:
        m_windowChannels->UpdateData();
        SetActiveView(m_windowChannels);
        return true;

      default:
        return (m_windowChannels->OnClickButton(message) ||
                m_windowGuide->OnClickButton(message) ||
                m_windowRecordings->OnClickButton(message) ||
                m_windowSearch->OnClickButton(message) ||
                m_windowTimers->OnClickButton(message) ||
                
                m_windowChannels->OnClickList(message) ||
                m_windowGuide->OnClickList(message) ||
                m_windowRecordings->OnClickList(message) ||
                m_windowSearch->OnClickList(message) ||
                m_windowTimers->OnClickList(message));
    }
  }

  return false;
}

bool CGUIWindowPVR::OpenGroupDialogSelect()
{
  CGUIDialogSelect *dialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (dialog == NULL)
    return false;
  
  CFileItemList options;
  g_PVRChannelGroups->Get(m_bRadio)->GetGroupList(&options);
  
  dialog->Reset();
  dialog->SetHeading(g_localizeStrings.Get(19146));
  dialog->SetItems(&options);
  dialog->SetMultiSelection(false);
  dialog->SetSelected(m_selectedGroup->GroupName());
  dialog->DoModal();
  
  if (!dialog->IsConfirmed())
    return false;
  
  const CFileItemPtr item = dialog->GetSelectedItem();
  if (item == NULL)
    return false;
  
  SetSelectedGroup(g_PVRChannelGroups->Get(m_bRadio)->GetByName(item->m_strTitle));
  GetActiveView()->UpdateData();
  
  return true;
}

void CGUIWindowPVR::CreateViews(void)
{
  CSingleLock lock(m_critSection);
  if (!m_windowChannels)
    m_windowChannels = new CGUIWindowPVRChannels(this);

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

  m_windowChannels->ResetObservers();
  m_windowGuide->ResetObservers();
  m_windowRecordings->ResetObservers();
  m_windowTimers->ResetObservers();

  m_bWasReset = true;
}

void CGUIWindowPVR::Cleanup(void)
{
  if (m_windowChannels)
    m_windowChannels->UnregisterObservers();
  SAFE_DELETE(m_windowChannels);

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
  if (view == m_windowChannels)
  {
    if (m_refreshWatch.GetElapsedMilliseconds() > CHANNELS_REFRESH_INTERVAL)
    {
      view->SetInvalid();
      m_refreshWatch.Reset();
    }
  }
  CGUIMediaWindow::FrameMove();
}

CPVRChannelGroupPtr CGUIWindowPVR::GetSelectedGroup(void)
{
  if (!m_selectedGroup)
    SetSelectedGroup(g_PVRManager.GetPlayingGroup(m_bRadio));
  
  return m_selectedGroup;
}

void CGUIWindowPVR::SetSelectedGroup(CPVRChannelGroupPtr group)
{
  if (!group)
    return;
  
  if (m_selectedGroup)
    m_selectedGroup->UnregisterObserver(m_windowChannels);
  m_selectedGroup = group;
  // we need to register the channel window to receive changes from the new group
  m_selectedGroup->RegisterObserver(m_windowChannels);
  g_PVRManager.SetPlayingGroup(m_selectedGroup);
}

bool CGUIWindowPVR::IsRadio()
{
  return m_bRadio;
}

void CGUIWindowPVR::SetRadio(bool bRadio)
{
  m_bRadio = bRadio;
}
