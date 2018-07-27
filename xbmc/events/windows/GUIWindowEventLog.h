/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "windows/GUIMediaWindow.h"

class CGUIWindowEventLog : public CGUIMediaWindow
{
public:
  CGUIWindowEventLog();
  ~CGUIWindowEventLog() override;

  // specialization of CGUIControl
  bool OnMessage(CGUIMessage& message) override;

protected:
  // specialization of CGUIMediaWindow
  bool OnSelect(int item) override;
  void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  void UpdateButtons() override;
  bool GetDirectory(const std::string &strDirectory, CFileItemList &items) override;
  std::string GetRootPath() const override { return "events://"; }

  bool OnSelect(CFileItemPtr item);
  bool OnDelete(CFileItemPtr item);
  bool OnExecute(CFileItemPtr item);

  void OnEventAdded(CFileItemPtr item);
  void OnEventRemoved(CFileItemPtr item);
};
