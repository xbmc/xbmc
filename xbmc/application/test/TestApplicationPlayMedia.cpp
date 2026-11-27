/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayLists.h"
#include "application/PlaybackAnnouncer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;

namespace
{
// Hears only the failures the playlists report.
class CFailureObserver : public CApplicationPlayLists::IObserver
{
public:
  void OnStarted(const std::shared_ptr<CFileItem>& started) override {}
  void OnListChanged(PLAYLIST::Type type, const PLAYLIST::PlayListChange& change) override {}
  void OnShuffled(PLAYLIST::Type type, bool shuffled) override {}
  void OnRepeat(PLAYLIST::Type type, PLAYLIST::Repeat repeat) override {}
  void OnFeed(bool playing) override {}
  void OnFailed(const std::shared_ptr<const CFileItem>& item, PLAYLIST::FailReason reason) override
  {
    failed.emplace_back(item->GetPath(), reason);
  }

  std::vector<std::pair<std::string, PLAYLIST::FailReason>> failed;
};
} // namespace

// Resolving an add-on item looks the add-on up and offers to install it, which reaches the window
// manager the test environment does not stand up. Disabled and skipped so that neither plain runs
// nor --gtest_also_run_disabled_tests can execute it until it does.
TEST(TestApplicationPlayMedia, DISABLED_AnAddonItemThatDoesNotResolveIsReported)
{
  GTEST_SKIP() << "resolving an add-on item needs the window manager";

  const auto playLists = CServiceBroker::GetPlayLists();
  CFailureObserver observer;
  playLists->SetObserver(&observer);

  const bool played = g_application.PlayMedia(CFileItem("plugin://plugin.video.absent/", false));
  playLists->SetObserver(
      CServiceBroker::GetAppComponents().GetComponent<CPlaybackAnnouncer>().get());

  EXPECT_FALSE(played);
  ASSERT_EQ(1u, observer.failed.size());
  EXPECT_EQ("plugin://plugin.video.absent/", observer.failed.front().first);
  EXPECT_EQ(PLAYLIST::FailReason::Unresolved, observer.failed.front().second);
}
