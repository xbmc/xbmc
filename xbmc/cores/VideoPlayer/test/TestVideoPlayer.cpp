/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "cores/IPlayerCallback.h"
#include "cores/VideoPlayer/VideoPlayer.h"
#include "jobs/JobManager.h"

#include <gtest/gtest.h>

using namespace std::chrono_literals;

class CFileItem;

class CTestPlayerCallback : public IPlayerCallback
{
public:
  void OnPlayBackEnded() override {}
  void OnPlayBackStarted(const CFileItem& file) override {}
  void OnPlayBackStopped() override {}
  void OnPlayBackError() override {}
  void OnQueueNextItem() override {}
};

class CTestVideoPlayer : public CVideoPlayer
{
public:
  explicit CTestVideoPlayer(IPlayerCallback& c) : CVideoPlayer(c) {}
  virtual ~CTestVideoPlayer() {}

  int InvokeGetPreviousBookmark(std::chrono::milliseconds ts) { return GetPreviousBookmark(ts); }
  int InvokeGetNextBookmark(std::chrono::milliseconds ts) { return GetNextBookmark(ts); }
  std::optional<std::chrono::milliseconds> InvokeGetBookmarkPos(int idx)
  {
    return GetBookmarkPos(idx);
  }
};

class TestVideoPlayer : public testing::Test
{
protected:
  static void SetUpTestSuite()
  {
    CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>());
  }
  static void TearDownTestSuite() { CServiceBroker::UnregisterJobManager(); }
};

TEST_F(TestVideoPlayer, GetPreviousBookmark)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  std::vector<std::chrono::milliseconds> bookmarks{100s, 200s};
  player.SetBookmarks(bookmarks);

  std::chrono::milliseconds ts{0s};
  int idx = player.InvokeGetPreviousBookmark(ts);
  EXPECT_EQ(-1, idx);
  ts = 100s;
  idx = player.InvokeGetPreviousBookmark(ts);
  EXPECT_EQ(-1, idx);
  // 5-second grade delay
  ts = 105s;
  idx = player.InvokeGetPreviousBookmark(ts);
  EXPECT_EQ(-1, idx);
  ts = 106s;
  idx = player.InvokeGetPreviousBookmark(ts);
  EXPECT_EQ(0, idx);
  ts = 200s;
  idx = player.InvokeGetPreviousBookmark(ts);
  EXPECT_EQ(0, idx);
  // 5-second grade delay
  ts = 205s;
  idx = player.InvokeGetPreviousBookmark(ts);
  EXPECT_EQ(0, idx);
  ts = 206s;
  idx = player.InvokeGetPreviousBookmark(ts);
  EXPECT_EQ(1, idx);
}

TEST_F(TestVideoPlayer, GetNextBookmark)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  std::vector<std::chrono::milliseconds> bookmarks{100s, 200s};
  player.SetBookmarks(bookmarks);

  std::chrono::milliseconds ts{0s};
  int idx = player.InvokeGetNextBookmark(ts);
  EXPECT_EQ(0, idx);
  ts = 100s;
  idx = player.InvokeGetNextBookmark(ts);
  EXPECT_EQ(1, idx);
  ts = 101s;
  idx = player.InvokeGetNextBookmark(ts);
  EXPECT_EQ(1, idx);
  ts = 200s;
  idx = player.InvokeGetNextBookmark(ts);
  EXPECT_EQ(-1, idx);
  ts = 201s;
  idx = player.InvokeGetNextBookmark(ts);
  EXPECT_EQ(-1, idx);
}

TEST_F(TestVideoPlayer, GetBookmarkPos)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  std::vector<std::chrono::milliseconds> bookmarks{100s, 200s};
  player.SetBookmarks(bookmarks);

  std::optional<std::chrono::milliseconds> pos = player.InvokeGetBookmarkPos(-1);

  EXPECT_FALSE(pos.has_value());

  pos = player.InvokeGetBookmarkPos(0);
  EXPECT_TRUE(pos.has_value());
  if (pos.has_value())
  {
    // braces to quiet clang warning
    EXPECT_EQ(100s, pos.value());
  }

  pos = player.InvokeGetBookmarkPos(1);
  EXPECT_TRUE(pos.has_value());
  if (pos.has_value())
  {
    // braces to quiet clang warning
    EXPECT_EQ(200s, pos.value());
  }

  pos = player.InvokeGetBookmarkPos(2);
  EXPECT_FALSE(pos.has_value());
}
