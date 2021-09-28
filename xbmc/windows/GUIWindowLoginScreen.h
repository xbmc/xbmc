/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "utils/Stopwatch.h"
#include "view/GUIViewControl.h"

class CFileItemList;

class CGUIWindowLoginScreen : public CGUIWindow
{
public:
  CGUIWindowLoginScreen(void);
  ~CGUIWindowLoginScreen(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  bool OnBack(int actionID) override;
  void FrameMove() override;
  bool HasListItems() const override { return true; }
  CFileItemPtr GetCurrentListItem(int offset = 0) override;
  int GetViewContainerID() const override { return m_viewControl.GetCurrentControl(); }

protected:
  void OnInitWindow() override;
  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  void Update();
  void SetLabel(int iControl, const std::string& strLabel);

  bool OnPopupMenu(int iItem);
  CGUIViewControl m_viewControl;
  CFileItemList* m_vecItems;

  int m_iSelectedItem;
  CStopWatch watch;
};
