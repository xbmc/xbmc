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
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

#include <stdexcept>

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

enum class TestSeekStep
{
  NORMAL,
  LARGE,
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

  void SetCurrentVideoId(int id) { m_CurrentVideo.id = id; }
  void SetCurrentAudioId(int id) { m_CurrentAudio.id = id; }
  void SetHasVideo(bool hasVideo) { m_HasVideo = hasVideo; }
  void SetHasAudio(bool hasAudio) { m_HasAudio = hasAudio; }
  bool GetHasVideo() const { return m_HasVideo; }
  bool GetHasAudio() const { return m_HasAudio; }
  void InvokeUpdateHasVideoAudio() { UpdateHasVideoAudio(); }

  void SetInitialAudioStream(int stream) { m_playerOptions.initialAudioStream = stream; }
  void InvokePrioritizeInitialAudioStream(std::vector<SelectionStream>& streams)
  {
    PrioritizeInitialAudioStream(streams);
  }

  constexpr static SeekStep ConvertTestSeekStep(TestSeekStep step)
  {
    if (step == TestSeekStep::NORMAL)
      return SeekStep::NORMAL;
    else if (step == TestSeekStep::LARGE)
      return SeekStep::LARGE;
    throw std::out_of_range("missing mapping");
  }

  static int64_t InvokeCalcTimeOrPercentSeekTarget(int64_t time,
                                                   int64_t maxTime,
                                                   Direction direction,
                                                   TestSeekStep step)
  {
    return CalcTimeOrPercentSeekTarget(time, maxTime, direction, ConvertTestSeekStep(step));
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

std::vector<SelectionStream> InitialAudioStreams()
{
  std::vector<SelectionStream> streams(3);
  streams[0].type_index = 2;
  streams[0].demuxerId = 11;
  streams[1].type_index = 0;
  streams[1].demuxerId = 37;
  streams[2].type_index = 1;
  streams[2].demuxerId = 92;
  return streams;
}

std::vector<int> AudioStreamOrdinals(const std::vector<SelectionStream>& streams)
{
  std::vector<int> ordinals;
  ordinals.reserve(streams.size());
  for (const auto& stream : streams)
    ordinals.emplace_back(stream.type_index);
  return ordinals;
}

TEST_F(TestVideoPlayer, InitialAudioStreamPrioritizesRequestedOrdinal)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);
  auto streams = InitialAudioStreams();

  player.SetInitialAudioStream(1);
  player.InvokePrioritizeInitialAudioStream(streams);

  EXPECT_EQ((std::vector<int>{1, 2, 0}), AudioStreamOrdinals(streams));
}

TEST_F(TestVideoPlayer, InitialAudioStreamInvalidRequestKeepsDefaultOrder)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  for (const int requested : {-1, -2, 3})
  {
    auto streams = InitialAudioStreams();
    player.SetInitialAudioStream(requested);
    player.InvokePrioritizeInitialAudioStream(streams);

    EXPECT_EQ((std::vector<int>{2, 0, 1}), AudioStreamOrdinals(streams));
  }
}

TEST_F(TestVideoPlayer, InitialAudioStreamIsConsumedAfterFirstCandidates)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);
  auto streams = InitialAudioStreams();

  player.SetInitialAudioStream(1);
  player.InvokePrioritizeInitialAudioStream(streams);
  EXPECT_EQ((std::vector<int>{1, 2, 0}), AudioStreamOrdinals(streams));

  streams = InitialAudioStreams();
  player.InvokePrioritizeInitialAudioStream(streams);
  EXPECT_EQ((std::vector<int>{2, 0, 1}), AudioStreamOrdinals(streams));
}

TEST_F(TestVideoPlayer, InitialAudioStreamWaitsForCandidates)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);
  std::vector<SelectionStream> streams;

  player.SetInitialAudioStream(1);
  player.InvokePrioritizeInitialAudioStream(streams);

  streams = InitialAudioStreams();
  player.InvokePrioritizeInitialAudioStream(streams);
  EXPECT_EQ((std::vector<int>{1, 2, 0}), AudioStreamOrdinals(streams));
}

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

TEST_F(TestVideoPlayer, UpdateHasVideoAudioClearsStaleVideoFlag)
{
  // simulates a mixed playlist transition from a music video to an audio-only
  // track: the previous item left m_HasVideo true, but the new item has no
  // video stream (m_CurrentVideo.id < 0), so the stale flag must be cleared
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  player.SetHasVideo(true);
  player.SetHasAudio(true);
  player.SetCurrentVideoId(-1);
  player.SetCurrentAudioId(0);

  player.InvokeUpdateHasVideoAudio();

  EXPECT_FALSE(player.GetHasVideo());
  EXPECT_TRUE(player.GetHasAudio());
}

TEST_F(TestVideoPlayer, UpdateHasVideoAudioKeepsFlagsWhenStreamsOpen)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  player.SetHasVideo(true);
  player.SetHasAudio(true);
  player.SetCurrentVideoId(0);
  player.SetCurrentAudioId(0);

  player.InvokeUpdateHasVideoAudio();

  EXPECT_TRUE(player.GetHasVideo());
  EXPECT_TRUE(player.GetHasAudio());
}

TEST_F(TestVideoPlayer, CalcTimeOrPercentSeekTargetCompat)
{
  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  ASSERT_TRUE(advancedSettings != nullptr);

  // Back compatibility mode
  // time based jumps allowed
  advancedSettings->m_videoSmoothPercentToTimeSeeking = false;
  advancedSettings->m_videoUseTimeSeeking = true;

  // ensure video long enough to engage time jumps
  int64_t maxTime = 2000 * advancedSettings->m_videoTimeSeekForwardBig + 1000;

  EXPECT_EQ(advancedSettings->m_videoTimeSeekForwardBig * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::LARGE));

  EXPECT_EQ(advancedSettings->m_videoTimeSeekForward * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::NORMAL));

  EXPECT_EQ(advancedSettings->m_videoTimeSeekBackwardBig * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::LARGE));

  EXPECT_EQ(advancedSettings->m_videoTimeSeekBackward * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::NORMAL));

  // video not long enough => percent based jumps
  maxTime = 2000 * advancedSettings->m_videoTimeSeekForwardBig - 1000;

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekForwardBig / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::LARGE));

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekForward / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::NORMAL));

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekBackwardBig / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::LARGE));

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekBackward / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::NORMAL));
}

TEST_F(TestVideoPlayer, CalcTimeOrPercentSeekTargetPercent)
{
  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  ASSERT_TRUE(advancedSettings != nullptr);

  // Percent based only
  advancedSettings->m_videoSmoothPercentToTimeSeeking = false;
  advancedSettings->m_videoUseTimeSeeking = false;

  // duration that would have engaged time based jumps otherwise
  int64_t maxTime = 2000 * advancedSettings->m_videoTimeSeekForwardBig + 1000;

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekForwardBig / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::LARGE));

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekForward / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::NORMAL));

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekBackwardBig / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::LARGE));

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekBackward / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::NORMAL));
}

TEST_F(TestVideoPlayer, CalcTimeOrPercentSeekTargetSmooth)
{
  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  ASSERT_TRUE(advancedSettings != nullptr);

  // Smooth percent to time based jumps
  advancedSettings->m_videoSmoothPercentToTimeSeeking = true;

  // Tests pattern: find the threshold between percent-based and time-based using
  // the advanced settings, then try a maxTime under and over the threshold

  int64_t threshold = advancedSettings->m_videoTimeSeekForwardBig * 1000 * 100 /
                      advancedSettings->m_videoPercentSeekForwardBig;

  // percent based for small durations
  int64_t maxTime = threshold / 2;

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekForwardBig / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::LARGE));
  // time based for large durations
  maxTime = threshold * 2;

  EXPECT_EQ(advancedSettings->m_videoTimeSeekForwardBig * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::LARGE));

  // Repeat for the other types of jumps
  threshold = advancedSettings->m_videoTimeSeekForward * 1000 * 100 /
              advancedSettings->m_videoPercentSeekForward;

  maxTime = threshold / 2;

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekForward / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::NORMAL));

  maxTime = threshold * 2;

  EXPECT_EQ(advancedSettings->m_videoTimeSeekForward * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::NORMAL));

  threshold = advancedSettings->m_videoTimeSeekBackwardBig * 1000 * 100 /
              advancedSettings->m_videoPercentSeekBackwardBig;

  maxTime = threshold / 2;

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekBackwardBig / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::LARGE));

  maxTime = threshold * 2;

  EXPECT_EQ(advancedSettings->m_videoTimeSeekBackwardBig * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::LARGE));

  threshold = advancedSettings->m_videoTimeSeekBackward * 1000 * 100 /
              advancedSettings->m_videoPercentSeekBackward;

  maxTime = threshold / 2;

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekBackward / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::NORMAL));

  maxTime = threshold * 2;

  EXPECT_EQ(advancedSettings->m_videoTimeSeekBackward * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::NORMAL));
}
