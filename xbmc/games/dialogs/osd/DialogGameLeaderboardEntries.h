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
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace KODI
{
namespace GAME
{

class CDialogGameLeaderboardEntries : public CGUIDialog
{
public:
  CDialogGameLeaderboardEntries();
  ~CDialogGameLeaderboardEntries() override = default;

  // Implementation of CGUIControl via CGUIDialog
  bool OnAction(const CAction& action) override;
  bool OnMessage(CGUIMessage& message) override;

protected:
  // Implementation of CGUIWindow via CGUIDialog
  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  void OnInitWindow() override;

private:
  // Dialog functions
  void FetchEntriesAsync();
  void RefreshList();

  // Utility function
  std::string FormatScore(const std::string& format, unsigned int score) const;

  // Dialog parameters
  CFileItemList m_items;
  CGUIViewControl m_viewControl;

  // Synchronization parameters
  std::thread m_fetchThread;
  std::atomic<bool> m_stopFetch{false};
  std::atomic<bool> m_playerFoundInEntries{false};
  std::mutex m_fetchMutex;
  std::vector<std::shared_ptr<CFileItem>> m_fetchedEntries;

  // Leaderboard parameters
  unsigned int m_leaderboardId{0};
  unsigned int m_totalEntries{0};
  std::string m_leaderboardTitle;
  std::string m_leaderboardFormat;
  std::string m_username;
  std::string m_token;
};

} // namespace GAME
} // namespace KODI
