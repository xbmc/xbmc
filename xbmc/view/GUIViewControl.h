/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "windowing/GraphicContext.h" // for VIEW_TYPE

#include <string>
#include <vector>

class CGUIControl;
class CFileItemList;

class CGUIViewControl
{
public:
  CGUIViewControl();
  virtual ~CGUIViewControl();

  void Reset();
  void SetParentWindow(int window);
  void AddView(const CGUIControl *control);
  void SetViewControlID(int control);

  void SetCurrentView(int viewMode, bool bRefresh = false);

  void SetItems(CFileItemList &items);

  void SetSelectedItem(int item);
  void SetSelectedItem(const std::string &itemPath);

  int GetSelectedItem() const;
  std::string GetSelectedItemPath() const;
  void SetFocused();

  bool HasControl(int controlID) const;
  int GetNextViewMode(int direction = 1) const;
  int GetViewModeNumber(int number) const;
  int GetViewModeCount() const;
  int GetViewModeByID(int id) const;

  int GetCurrentControl() const;

  void Clear();

protected:
  int GetSelectedItem(const CGUIControl *control) const;
  void UpdateContents(const CGUIControl *control, int currentItem) const;
  void UpdateView();
  void UpdateViewAsControl(const std::string &viewLabel);
  void UpdateViewVisibility();
  int GetView(VIEW_TYPE type, int id) const;

  std::vector<CGUIControl*> m_allViews;
  std::vector<CGUIControl*> m_visibleViews;
  typedef std::vector<CGUIControl*>::const_iterator ciViews;

  CFileItemList* m_fileItems;
  int m_viewAsControl;
  int m_parentWindow;
  int m_currentView;
};
