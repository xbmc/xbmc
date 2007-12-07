/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIViewControl.h"
#include "Util.h"

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
  m_vecViews.clear();
}

void CGUIViewControl::AddView(const CGUIControl *control)
{
  if (!control) return;
  m_vecViews.push_back((CGUIControl *)control);
}

void CGUIViewControl::SetViewControlID(int control)
{
  m_viewAsControl = control;
}

void CGUIViewControl::SetParentWindow(int window)
{
  m_parentWindow = window;
}

void CGUIViewControl::SetCurrentView(int viewMode)
{
  // viewMode is of the form TYPE << 16 | ID
  VIEW_TYPE type = (VIEW_TYPE)(viewMode >> 16);
  int id = viewMode & 0xffff;

  // first find a view that matches this view, if possible...
  int newView = GetView(type, id);
  if (newView < 0) // no suitable view that matches both id and type, so try just type
    newView = GetView(type, 0);
  if (newView < 0 && type == VIEW_TYPE_BIG_ICON) // try icon view if they want big icon
    newView = GetView(VIEW_TYPE_ICON, 0);
  if (newView < 0) // try a list view
    newView = GetView(VIEW_TYPE_LIST, 0);
  if (newView < 0) // try anything!
    newView = GetView(VIEW_TYPE_NONE, 0);

  if (newView < 0 || m_currentView == newView)
    return;

//  CLog::DebugLog("SetCurrentView: Oldview: %i, Newview :%i", m_currentView, viewMode);
  CGUIControl *pNewView = m_vecViews[newView];

  bool hasFocus(false);
  int item = -1;
  if (m_currentView >= 0 && m_currentView < (int)m_vecViews.size())
  { // have an old view - let's clear it out and hide it.
    CGUIControl *pControl = m_vecViews[m_currentView];
    hasFocus = pControl->HasFocus();
    item = GetSelectedItem(pControl);
    CGUIMessage msg(GUI_MSG_LABEL_RESET, m_parentWindow, pControl->GetID(), 0, 0, NULL);
    pControl->OnMessage(msg);
  }
  m_currentView = newView;
  // make only current control visible...
  for (ciViews view = m_vecViews.begin(); view != m_vecViews.end(); view++)
    (*view)->SetVisible(false);
  pNewView->SetVisible(true);

  // and focus if necessary
  if (hasFocus)
  {
    CGUIMessage msg(GUI_MSG_SETFOCUS, m_parentWindow, pNewView->GetID(), 0);
    g_graphicsContext.SendMessage(msg);
  }

  // Update it with the contents
  UpdateContents(pNewView, item);

  // Update our view control
  UpdateViewAsControl(((CGUIBaseContainer *)pNewView)->GetLabel());
}

void CGUIViewControl::SetItems(CFileItemList &items)
{
//  CLog::DebugLog("SetItems: %i", m_currentView);
  m_fileItems = &items;
  // update our current view control...
  UpdateView();
}

void CGUIViewControl::UpdateContents(const CGUIControl *control, int currentItem)
{
  if (!control || !m_fileItems) return;
  CGUIMessage msg(GUI_MSG_LABEL_BIND, m_parentWindow, control->GetID(), currentItem, 0, (void*)m_fileItems);
  g_graphicsContext.SendMessage(msg);
}

void CGUIViewControl::UpdateView()
{
//  CLog::DebugLog("UpdateView: %i", m_currentView);
  if (m_currentView < 0 || m_currentView >= (int)m_vecViews.size())
    return; // no valid current view!

  CGUIControl *pControl = m_vecViews[m_currentView];
  // get the currently selected item
  int item = GetSelectedItem(pControl);
  UpdateContents(pControl, item < 0 ? 0 : item);
}

int CGUIViewControl::GetSelectedItem(const CGUIControl *control) const
{
  if (!control || !m_fileItems) return -1;
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, m_parentWindow, control->GetID(), 0, 0, NULL);
  g_graphicsContext.SendMessage(msg);

  int iItem = msg.GetParam1();
  if (iItem >= m_fileItems->Size())
    return -1;
  return iItem;
}

int CGUIViewControl::GetSelectedItem() const
{
  if (m_currentView < 0 || m_currentView >= (int)m_vecViews.size())
    return -1; // no valid current view!

  return GetSelectedItem(m_vecViews[m_currentView]);
}

void CGUIViewControl::SetSelectedItem(int item)
{
  if (!m_fileItems || item < 0 || item >= m_fileItems->Size())
    return;

  if (m_currentView < 0 || m_currentView >= (int)m_vecViews.size())
    return; // no valid current view!

  CGUIMessage msg(GUI_MSG_ITEM_SELECT, m_parentWindow, m_vecViews[m_currentView]->GetID(), item);
  g_graphicsContext.SendMessage(msg);
}

void CGUIViewControl::SetSelectedItem(const CStdString &itemPath)
{
  if (!m_fileItems || itemPath.IsEmpty())
    return;

  CStdString comparePath(itemPath);
  CUtil::RemoveSlashAtEnd(comparePath);

  int item = -1;
  for (int i = 0; i < m_fileItems->Size(); ++i)
  {
    CStdString strPath =(*m_fileItems)[i]->m_strPath;
    CUtil::RemoveSlashAtEnd(strPath);
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
  if (m_currentView < 0 || m_currentView >= (int)m_vecViews.size())
    return; // no valid current view!

  CGUIMessage msg(GUI_MSG_SETFOCUS, m_parentWindow, m_vecViews[m_currentView]->GetID(), 0);
  g_graphicsContext.SendMessage(msg);
}

bool CGUIViewControl::HasControl(int viewControlID) const
{
  // run through our controls, checking for the id
  for (ciViews it = m_vecViews.begin(); it != m_vecViews.end(); it++)
  {
    if ((*it)->GetID() == viewControlID)
      return true;
  }
  return false;
}

int CGUIViewControl::GetCurrentControl() const
{
  if (m_currentView < 0 || m_currentView >= (int)m_vecViews.size())
    return -1; // no valid current view!

  return m_vecViews[m_currentView]->GetID();
}

// returns the number-th view's viewmode (type and id)
int CGUIViewControl::GetViewModeNumber(int number) const
{
  CGUIBaseContainer *nextView = NULL;
  if (number >= 0 || number < (int)m_vecViews.size())
    nextView = (CGUIBaseContainer *)m_vecViews[number];
  else if (m_vecViews.size())
    nextView = (CGUIBaseContainer *)m_vecViews[0];
  if (nextView)
    return (nextView->GetType() << 16) | nextView->GetID();
  return 0;  // no view modes :(
}

int CGUIViewControl::GetViewModeByID(int id) const
{
  for (unsigned int i = 0; i < m_vecViews.size(); ++i)
  {
    CGUIBaseContainer *view = (CGUIBaseContainer *)m_vecViews[i];
    if (view->GetID() == id)
      return (view->GetType() << 16) | view->GetID();
  }
  return 0;  // no view modes :(
}

// returns the next viewmode in the cycle
int CGUIViewControl::GetNextViewMode(int direction) const
{
  if (!m_vecViews.size())
    return 0; // no view modes :(

  int viewNumber = (m_currentView + direction) % (int)m_vecViews.size();
  if (viewNumber < 0) viewNumber += m_vecViews.size();
  CGUIBaseContainer *nextView = (CGUIBaseContainer *)m_vecViews[viewNumber];
  return (nextView->GetType() << 16) | nextView->GetID();
}

void CGUIViewControl::Clear()
{
  if (m_currentView < 0 || m_currentView >= (int)m_vecViews.size())
    return; // no valid current view!

  CGUIMessage msg(GUI_MSG_LABEL_RESET, m_parentWindow, m_vecViews[m_currentView]->GetID(), 0);
  g_graphicsContext.SendMessage(msg);
}

int CGUIViewControl::GetView(VIEW_TYPE type, int id) const
{
  for (int i = 0; i < (int)m_vecViews.size(); i++)
  {
    CGUIBaseContainer *view = (CGUIBaseContainer *)m_vecViews[i];
    if ((type == VIEW_TYPE_NONE || type == view->GetType()) && (!id || view->GetID() == id))
    {
      return i;
    }
  }
  return -1;
}

void CGUIViewControl::UpdateViewAsControl(const CStdString &viewLabel)
{
  // the view as control could be a select/spin/dropdown button
  CGUIMessage msg(GUI_MSG_LABEL_RESET, m_parentWindow, m_viewAsControl);
  g_graphicsContext.SendMessage(msg);
  for (unsigned int i = 0; i < m_vecViews.size(); i++)
  {
    CGUIBaseContainer *view = (CGUIBaseContainer *)m_vecViews[i];
    CGUIMessage msg(GUI_MSG_LABEL_ADD, m_parentWindow, m_viewAsControl, i);
    CStdString label;
    label.Format(g_localizeStrings.Get(534).c_str(), view->GetLabel().c_str()); // View: %s
    msg.SetLabel(label);
    g_graphicsContext.SendMessage(msg);
  }
  CGUIMessage msgSelect(GUI_MSG_ITEM_SELECT, m_parentWindow, m_viewAsControl, m_currentView);
  g_graphicsContext.SendMessage(msgSelect);

  // otherwise it's just a normal button
  CStdString label;
  label.Format(g_localizeStrings.Get(534).c_str(), viewLabel.c_str()); // View: %s
  CGUIMessage msgSet(GUI_MSG_LABEL_SET, m_parentWindow, m_viewAsControl);
  msgSet.SetLabel(label);
  g_graphicsContext.SendMessage(msgSet);
}