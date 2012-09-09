#pragma once

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

#include "GUIViewState.h"

#include "guilib/GUIBaseContainer.h"

class CGUIViewControl
{
public:
  CGUIViewControl(void);
  virtual ~CGUIViewControl(void);

  void Reset();
  void SetParentWindow(int window);
  void AddView(const CGUIControl *control);
  void SetViewControlID(int control);

  void SetCurrentView(int viewMode, bool bRefresh = false);

  void SetItems(CFileItemList &items);

  void SetSelectedItem(int item);
  void SetSelectedItem(const CStdString &itemPath);

  int GetSelectedItem() const;
  void SetFocused();

  bool HasControl(int controlID) const;
  int GetNextViewMode(int direction = 1) const;
  int GetViewModeNumber(int number) const;
  int GetViewModeByID(int id) const;

  int GetCurrentControl() const;

  void Clear();

protected:
  int GetSelectedItem(const CGUIControl *control) const;
  void UpdateContents(const CGUIControl *control, int currentItem);
  void UpdateView();
  void UpdateViewAsControl(const CStdString &viewLabel);
  void UpdateViewVisibility();
  int GetView(VIEW_TYPE type, int id) const;

  std::vector<CGUIControl *> m_allViews;
  std::vector<CGUIControl *> m_visibleViews;
  typedef std::vector<CGUIControl *>::const_iterator ciViews;

  CFileItemList*        m_fileItems;
  int                   m_viewAsControl;
  int                   m_parentWindow;
  int                   m_currentView;
};
