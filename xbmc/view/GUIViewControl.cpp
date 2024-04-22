/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIViewControl.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/IGUIContainer.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <utility>

CGUIViewControl::CGUIViewControl(void)
{
  m_viewAsControl = -1;
  m_parentWindow = WINDOW_INVALID;
  m_fileItems = nullptr;
  Reset();
}

CGUIViewControl::~CGUIViewControl(void) = default;

void CGUIViewControl::Reset()
{
  m_currentView = -1;
  m_visibleViews.clear();
  m_allViews.clear();
}

void CGUIViewControl::AddView(const CGUIControl *control)
{
  if (!control || !control->IsContainer()) return;
  m_allViews.push_back(const_cast<CGUIControl*>(control));
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
  CGUIControl *previousView = nullptr;
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
  for (ciViews view = m_allViews.begin(); view != m_allViews.end(); ++view)
    (*view)->SetVisible(false);
  pNewView->SetVisible(true);

  if (!bRefresh && pNewView == previousView)
    return; // no need to actually update anything (other than visibility above)

  //  CLog::Log(LOGDEBUG,"SetCurrentView: Oldview: {}, Newview :{}", m_currentView, viewMode);

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
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg, m_parentWindow);
  }

  UpdateViewAsControl(static_cast<IGUIContainer*>(pNewView)->GetLabel());
}

void CGUIViewControl::SetItems(CFileItemList &items)
{
  //  CLog::Log(LOGDEBUG,"SetItems: {}", m_currentView);
  m_fileItems = &items;
  // update our current view control...
  UpdateView();
}

void CGUIViewControl::UpdateContents(const CGUIControl *control, int currentItem) const
{
  if (!control || !m_fileItems) return;
  CGUIMessage msg(GUI_MSG_LABEL_BIND, m_parentWindow, control->GetID(), currentItem, 0, m_fileItems);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg, m_parentWindow);
}

void CGUIViewControl::UpdateView()
{
  //  CLog::Log(LOGDEBUG,"UpdateView: {}", m_currentView);
  if (m_currentView < 0 || m_currentView >= (int)m_visibleViews.size())
    return; // no valid current view!

  CGUIControl *pControl = m_visibleViews[m_currentView];
  // get the currently selected item
  int item = GetSelectedItem(pControl);
  UpdateContents(pControl, item < 0 ? 0 : item);
}

int CGUIViewControl::GetSelectedItem(const CGUIControl *control) const
{
  if (!control || !m_fileItems)
    return -1;

  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, m_parentWindow, control->GetID());
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg, m_parentWindow);

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

std::string CGUIViewControl::GetSelectedItemPath() const
{
  if (m_currentView < 0 || (size_t)m_currentView >= m_visibleViews.size())
    return "";

  int selectedItem = GetSelectedItem(m_visibleViews[m_currentView]);
  if (selectedItem > -1)
  {
    CFileItemPtr fileItem = m_fileItems->Get(selectedItem);
    if (fileItem)
      return fileItem->GetPath();
  }

  return "";
}

void CGUIViewControl::SetSelectedItem(int item)
{
  if (!m_fileItems || item < 0 || item >= m_fileItems->Size())
    return;

  if (m_currentView < 0 || m_currentView >= (int)m_visibleViews.size())
    return; // no valid current view!

  CGUIMessage msg(GUI_MSG_ITEM_SELECT, m_parentWindow, m_visibleViews[m_currentView]->GetID(), item);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg, m_parentWindow);
}

void CGUIViewControl::SetSelectedItem(const std::string &itemPath)
{
  if (!m_fileItems || itemPath.empty())
    return;

  std::string comparePath(itemPath);
  URIUtils::RemoveSlashAtEnd(comparePath);

  int item = -1;
  for (int i = 0; i < m_fileItems->Size(); ++i)
  {
    std::string strPath =(*m_fileItems)[i]->GetPath();
    URIUtils::RemoveSlashAtEnd(strPath);
    if (strPath == comparePath)
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
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg, m_parentWindow);
}

bool CGUIViewControl::HasControl(int viewControlID) const
{
  // run through our controls, checking for the id
  for (ciViews it = m_allViews.begin(); it != m_allViews.end(); ++it)
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
  IGUIContainer *nextView = nullptr;
  if (number >= 0 && number < (int)m_visibleViews.size())
    nextView = static_cast<IGUIContainer*>(m_visibleViews[number]);
  else if (m_visibleViews.size())
    nextView = static_cast<IGUIContainer*>(m_visibleViews[0]);
  if (nextView)
    return (nextView->GetType() << 16) | nextView->GetID();
  return 0;  // no view modes :(
}

// returns the amount of visible views
int CGUIViewControl::GetViewModeCount() const
{
  return static_cast<int>(m_visibleViews.size());
}

int CGUIViewControl::GetViewModeByID(int id) const
{
  for (unsigned int i = 0; i < m_visibleViews.size(); ++i)
  {
    IGUIContainer *view = static_cast<IGUIContainer*>(m_visibleViews[i]);
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
  IGUIContainer *nextView = static_cast<IGUIContainer*>(m_visibleViews[viewNumber]);
  return (nextView->GetType() << 16) | nextView->GetID();
}

void CGUIViewControl::Clear()
{
  if (m_currentView < 0 || m_currentView >= (int)m_visibleViews.size())
    return; // no valid current view!

  CGUIMessage msg(GUI_MSG_LABEL_RESET, m_parentWindow, m_visibleViews[m_currentView]->GetID(), 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg, m_parentWindow);
}

int CGUIViewControl::GetView(VIEW_TYPE type, int id) const
{
  for (int i = 0; i < (int)m_visibleViews.size(); i++)
  {
    IGUIContainer *view = static_cast<IGUIContainer*>(m_visibleViews[i]);
    if ((type == VIEW_TYPE_NONE || type == view->GetType()) && (!id || view->GetID() == id))
      return i;
  }
  return -1;
}

void CGUIViewControl::UpdateViewAsControl(const std::string &viewLabel)
{
  // the view as control could be a select/spin/dropdown button
  std::vector< std::pair<std::string, int> > labels;
  for (unsigned int i = 0; i < m_visibleViews.size(); i++)
  {
    IGUIContainer *view = static_cast<IGUIContainer*>(m_visibleViews[i]);
    std::string label = StringUtils::Format(g_localizeStrings.Get(534),
                                            view->GetLabel()); // View: {}
    labels.emplace_back(std::move(label), i);
  }
  CGUIMessage msg(GUI_MSG_SET_LABELS, m_parentWindow, m_viewAsControl, m_currentView);
  msg.SetPointer(&labels);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg, m_parentWindow);

  // otherwise it's just a normal button
  std::string label = StringUtils::Format(g_localizeStrings.Get(534), viewLabel); // View: {}
  CGUIMessage msgSet(GUI_MSG_LABEL_SET, m_parentWindow, m_viewAsControl);
  msgSet.SetLabel(label);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msgSet, m_parentWindow);
}

void CGUIViewControl::UpdateViewVisibility()
{
  // first reset our infomanager cache, as it's likely that the vis conditions
  // used for views (i.e. based on contenttype) may have changed
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  infoMgr.ResetCache();
  infoMgr.GetInfoProviders().GetGUIControlsInfoProvider().ResetContainerMovingCache();
  m_visibleViews.clear();
  for (unsigned int i = 0; i < m_allViews.size(); i++)
  {
    CGUIControl *view = m_allViews[i];
    if (view->HasVisibleCondition())
    {
      view->UpdateVisibility(nullptr);
      if (view->IsVisibleFromSkin())
        m_visibleViews.push_back(view);
    }
    else
      m_visibleViews.push_back(view);
  }
}
