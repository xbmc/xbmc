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

#include "GUIViewControl.h"
#include "guilib/GUIWindowManager.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "GUIInfoManager.h"
#include "guilib/Key.h"

CGUIViewControl::CGUIViewControl(void)
{
  m_viewAsControl = -1;
  m_parentWindow = WINDOW_INVALID;
  m_fileItems = NULL;
  Reset();
}

CGUIViewControl::~CGUIViewControl(void)
{
}

void CGUIViewControl::Reset()
{
  m_currentView = -1;
  m_visibleViews.clear();
  m_allViews.clear();
}

void CGUIViewControl::AddView(const CGUIControl *control)
{
  if (!control || !control->IsContainer()) return;
  m_allViews.push_back((CGUIControl *)control);
}

void CGUIViewControl::SetViewControlID(int control)
{
  m_viewAsControl = control;
}

void CGUIViewControl::SetParentWindow(int window)
{
  m_parentWindow = window;
}

void CGUIViewControl::SetCurrentView(int viewMode, bool bRefresh /* = false */)
{
  // grab the previous control
  CGUIControl *previousView = NULL;
  if (m_currentView >= 0 && m_currentView < (int)m_visibleViews.size())
    previousView = m_visibleViews[m_currentView];

  UpdateViewVisibility();

  // viewMode is of the form TYPE << 16 | ID
  VIEW_TYPE type = (VIEW_TYPE)(viewMode >> 16);
  int id = viewMode & 0xffff;

  // first find a view that matches this view, if possible...
  int newView = GetView(type, id);
  if (newView < 0) // no suitable view that matches both id and type, so try just type
    newView = GetView(type, 0);
  if (newView < 0 && type == VIEW_TYPE_BIG_ICON) // try icon view if they want big icon
    newView = GetView(VIEW_TYPE_ICON, 0);
  if (newView < 0 && type == VIEW_TYPE_BIG_INFO)
    newView = GetView(VIEW_TYPE_INFO, 0);
  if (newView < 0) // try a list view
    newView = GetView(VIEW_TYPE_LIST, 0);
  if (newView < 0) // try anything!
    newView = GetView(VIEW_TYPE_NONE, 0);

  if (newView < 0)
    return;

  m_currentView = newView;
  CGUIControl *pNewView = m_visibleViews[m_currentView];

  // make only current control visible...
  for (ciViews view = m_allViews.begin(); view != m_allViews.end(); view++)
    (*view)->SetVisible(false);
  pNewView->SetVisible(true);

  if (!bRefresh && pNewView == previousView)
    return; // no need to actually update anything (other than visibility above)

//  CLog::Log(LOGDEBUG,"SetCurrentView: Oldview: %i, Newview :%i", m_currentView, viewMode);

  bool hasFocus(false);
  int item = -1;
  if (previousView)
  { // have an old view - let's clear it out and hide it.
    hasFocus = previousView->HasFocus();
    item = GetSelectedItem(previousView);
    CGUIMessage msg(GUI_MSG_LABEL_RESET, m_parentWindow, previousView->GetID());
    previousView->OnMessage(msg);
  }

  // Update it with the contents
  UpdateContents(pNewView, item);

  // and focus if necessary
  if (hasFocus)
  {
    CGUIMessage msg(GUI_MSG_SETFOCUS, m_parentWindow, pNewView->GetID(), 0);
    g_windowManager.SendMessage(msg);
  }

  // Update our view control only if we are not in the TV Window
  if (m_parentWindow != WINDOW_PVR)
    UpdateViewAsControl(((CGUIBaseContainer *)pNewView)->GetLabel());
}

void CGUIViewControl::SetItems(CFileItemList &items)
{
//  CLog::Log(LOGDEBUG,"SetItems: %i", m_currentView);
  m_fileItems = &items;
  // update our current view control...
  UpdateView();
}

void CGUIViewControl::UpdateContents(const CGUIControl *control, int currentItem)
{
  if (!control || !m_fileItems) return;
  CGUIMessage msg(GUI_MSG_LABEL_BIND, m_parentWindow, control->GetID(), currentItem, 0, m_fileItems);
  g_windowManager.SendMessage(msg);
}

void CGUIViewControl::UpdateView()
{
//  CLog::Log(LOGDEBUG,"UpdateView: %i", m_currentView);
  if (m_currentView < 0 || m_currentView >= (int)m_visibleViews.size())
    return; // no valid current view!

  CGUIControl *pControl = m_visibleViews[m_currentView];
  // get the currently selected item
  int item = GetSelectedItem(pControl);
  UpdateContents(pControl, item < 0 ? 0 : item);
}

int CGUIViewControl::GetSelectedItem(const CGUIControl *control) const
{
  if (!control || !m_fileItems) return -1;
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, m_parentWindow, control->GetID());
  g_windowManager.SendMessage(msg);

  int iItem = msg.GetParam1();
  if (iItem >= m_fileItems->Size())
    return -1;
  return iItem;
}

int CGUIViewControl::GetSelectedItem() const
{
  if (m_currentView < 0 || m_currentView >= (int)m_visibleViews.size())
    return -1; // no valid current view!

  return GetSelectedItem(m_visibleViews[m_currentView]);
}

void CGUIViewControl::SetSelectedItem(int item)
{
  if (!m_fileItems || item < 0 || item >= m_fileItems->Size())
    return;

  if (m_currentView < 0 || m_currentView >= (int)m_visibleViews.size())
    return; // no valid current view!

  CGUIMessage msg(GUI_MSG_ITEM_SELECT, m_parentWindow, m_visibleViews[m_currentView]->GetID(), item);
  g_windowManager.SendMessage(msg);
}

void CGUIViewControl::SetSelectedItem(const CStdString &itemPath)
{
  if (!m_fileItems || itemPath.IsEmpty())
    return;

  CStdString comparePath(itemPath);
  URIUtils::RemoveSlashAtEnd(comparePath);

  int item = -1;
  for (int i = 0; i < m_fileItems->Size(); ++i)
  {
    CStdString strPath =(*m_fileItems)[i]->GetPath();
    URIUtils::RemoveSlashAtEnd(strPath);
    if (strPath.CompareNoCase(comparePath) == 0)
    {
      item = i;
      break;
    }
  }
  SetSelectedItem(item);
}

void CGUIViewControl::SetFocused()
{
  if (m_currentView < 0 || m_currentView >= (int)m_visibleViews.size())
    return; // no valid current view!

  CGUIMessage msg(GUI_MSG_SETFOCUS, m_parentWindow, m_visibleViews[m_currentView]->GetID(), 0);
  g_windowManager.SendMessage(msg);
}

bool CGUIViewControl::HasControl(int viewControlID) const
{
  // run through our controls, checking for the id
  for (ciViews it = m_allViews.begin(); it != m_allViews.end(); it++)
  {
    if ((*it)->GetID() == viewControlID)
      return true;
  }
  return false;
}

int CGUIViewControl::GetCurrentControl() const
{
  if (m_currentView < 0 || m_currentView >= (int)m_visibleViews.size())
    return -1; // no valid current view!

  return m_visibleViews[m_currentView]->GetID();
}

// returns the number-th view's viewmode (type and id)
int CGUIViewControl::GetViewModeNumber(int number) const
{
  CGUIBaseContainer *nextView = NULL;
  if (number >= 0 && number < (int)m_visibleViews.size())
    nextView = (CGUIBaseContainer *)m_visibleViews[number];
  else if (m_visibleViews.size())
    nextView = (CGUIBaseContainer *)m_visibleViews[0];
  if (nextView)
    return (nextView->GetType() << 16) | nextView->GetID();
  return 0;  // no view modes :(
}

int CGUIViewControl::GetViewModeByID(int id) const
{
  for (unsigned int i = 0; i < m_visibleViews.size(); ++i)
  {
    CGUIBaseContainer *view = (CGUIBaseContainer *)m_visibleViews[i];
    if (view->GetID() == id)
      return (view->GetType() << 16) | view->GetID();
  }
  return 0;  // no view modes :(
}

// returns the next viewmode in the cycle
int CGUIViewControl::GetNextViewMode(int direction) const
{
  if (!m_visibleViews.size())
    return 0; // no view modes :(

  int viewNumber = (m_currentView + direction) % (int)m_visibleViews.size();
  if (viewNumber < 0) viewNumber += m_visibleViews.size();
  CGUIBaseContainer *nextView = (CGUIBaseContainer *)m_visibleViews[viewNumber];
  return (nextView->GetType() << 16) | nextView->GetID();
}

void CGUIViewControl::Clear()
{
  if (m_currentView < 0 || m_currentView >= (int)m_visibleViews.size())
    return; // no valid current view!

  CGUIMessage msg(GUI_MSG_LABEL_RESET, m_parentWindow, m_visibleViews[m_currentView]->GetID(), 0);
  g_windowManager.SendMessage(msg);
}

int CGUIViewControl::GetView(VIEW_TYPE type, int id) const
{
  for (int i = 0; i < (int)m_visibleViews.size(); i++)
  {
    CGUIBaseContainer *view = (CGUIBaseContainer *)m_visibleViews[i];
    if ((type == VIEW_TYPE_NONE || type == view->GetType()) && (!id || view->GetID() == id))
      return i;
  }
  return -1;
}

void CGUIViewControl::UpdateViewAsControl(const CStdString &viewLabel)
{
  // the view as control could be a select/spin/dropdown button
  CGUIMessage msg(GUI_MSG_LABEL_RESET, m_parentWindow, m_viewAsControl);
  g_windowManager.SendMessage(msg);
  for (unsigned int i = 0; i < m_visibleViews.size(); i++)
  {
    CGUIBaseContainer *view = (CGUIBaseContainer *)m_visibleViews[i];
    CGUIMessage msg(GUI_MSG_LABEL_ADD, m_parentWindow, m_viewAsControl, i);
    CStdString label;
    label.Format(g_localizeStrings.Get(534).c_str(), view->GetLabel().c_str()); // View: %s
    msg.SetLabel(label);
    g_windowManager.SendMessage(msg);
  }
  CGUIMessage msgSelect(GUI_MSG_ITEM_SELECT, m_parentWindow, m_viewAsControl, m_currentView);
  g_windowManager.SendMessage(msgSelect);

  // otherwise it's just a normal button
  CStdString label;
  label.Format(g_localizeStrings.Get(534).c_str(), viewLabel.c_str()); // View: %s
  CGUIMessage msgSet(GUI_MSG_LABEL_SET, m_parentWindow, m_viewAsControl);
  msgSet.SetLabel(label);
  g_windowManager.SendMessage(msgSet);
}

void CGUIViewControl::UpdateViewVisibility()
{
  // first reset our infomanager cache, as it's likely that the vis conditions
  // used for views (i.e. based on contenttype) may have changed
  g_infoManager.ResetCache();
  m_visibleViews.clear();
  for (unsigned int i = 0; i < m_allViews.size(); i++)
  {
    CGUIControl *view = m_allViews[i];
    if (view->GetVisibleCondition())
    {
      view->UpdateVisibility();
      if (view->IsVisibleFromSkin())
        m_visibleViews.push_back(view);
    }
    else
      m_visibleViews.push_back(view);
  }
}

