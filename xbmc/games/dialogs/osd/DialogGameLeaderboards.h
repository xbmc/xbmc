/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItemList.h"
#include "guilib/GUIDialog.h"
#include "view/GUIViewControl.h"

#include <atomic>
#include <thread>

namespace KODI
{
namespace GAME
{

class CDialogGameLeaderboards : public CGUIDialog
{
public:
  CDialogGameLeaderboards();
  ~CDialogGameLeaderboards() override = default;

  // Implementation of CGUIControl via CGUIDialog
  bool OnMessage(CGUIMessage& message) override;

protected:
  // Implementation of CGUIWindow via CGUIDialog
  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  void OnInitWindow() override;

private:
  // Dialog functions
  void PopulateList();
  void FetchTopEntriesAsync();

  // Dialog parameters
  CFileItemList m_items;
  CGUIViewControl m_viewControl;
  int m_lastSelectedItem{-1};

  // Synchronization parameters
  std::thread m_fetchThread;
  std::atomic<bool> m_stopFetch{false};
};

} // namespace GAME
} // namespace KODI
