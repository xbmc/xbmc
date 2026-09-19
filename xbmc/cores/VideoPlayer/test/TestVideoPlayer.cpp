/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "cores/IPlayerCallback.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDInputStream.h"
#include "cores/VideoPlayer/Interface/InputStreamConstants.h"
#include "cores/VideoPlayer/VideoPlayer.h"
#include "jobs/JobManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

#include <stdexcept>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

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
  bool InvokeEvaluateIsStreaming() const { return EvaluateIsStreaming(); }

  void SetCurrentVideoId(int id) { m_CurrentVideo.id = id; }
  void SetCurrentAudioId(int id) { m_CurrentAudio.id = id; }
  void SetHasVideo(bool hasVideo) { m_HasVideo = hasVideo; }
  void SetHasAudio(bool hasAudio) { m_HasAudio = hasAudio; }
  bool GetHasVideo() const { return m_HasVideo; }
  bool GetHasAudio() const { return m_HasAudio; }
  void InvokeUpdateHasVideoAudio() { UpdateHasVideoAudio(); }

  void SetItem(const CFileItem& item) { m_item = item; }
  void SetInputStream(std::shared_ptr<CDVDInputStream> inputStream)
  {
    m_pInputStream = inputStream;
  }
  void SetDemuxer(std::unique_ptr<CDVDDemux> demuxer) { m_pDemuxer = std::move(demuxer); }

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

namespace
{
class CTestInputStream : public CDVDInputStream
{
public:
  CTestInputStream(DVDStreamType type, const CFileItem& item) : CDVDInputStream(type, item) {}
  int Read(uint8_t* buf, int buf_size) override { return 0; }
  int64_t Seek(int64_t offset, int whence) override { return 0; }
  int64_t GetLength() override { return 0; }
  bool IsEOF() override { return false; }
};

class CTestDemux : public CDVDDemux
{
public:
  explicit CTestDemux(bool streaming = false) : m_isStreaming(streaming) {}
  bool Reset() override { return true; }
  void Flush() override {}
  void Abort() override {}
  DemuxPacket* Read() override { return nullptr; }
  bool SeekTime(double time, bool backwards = false, double* startpts = nullptr) override
  {
    return true;
  }
  CDemuxStream* GetStream(int iStreamId) const override { return nullptr; }
  std::vector<CDemuxStream*> GetStreams() const override { return {}; }
  int GetNrOfStreams() const override { return 0; }
  bool IsStreaming() const override { return m_isStreaming; }

private:
  bool m_isStreaming{false};
};
} // namespace

TEST_F(TestVideoPlayer, IsStreamingLocalFileReturnsFalse)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  // Local files
  player.SetItem(CFileItem("C:\\Videos\\movie.mkv", false));
  EXPECT_FALSE(player.InvokeEvaluateIsStreaming());

  player.SetItem(CFileItem("/home/user/video.mp4", false));
  EXPECT_FALSE(player.InvokeEvaluateIsStreaming());

  // Network shares
  player.SetItem(CFileItem("smb://192.168.1.1/movies/video.mkv", false));
  EXPECT_FALSE(player.InvokeEvaluateIsStreaming());

  // Direct HTTP file playback
  player.SetItem(CFileItem("http://example.com/video.mp4", false));
  EXPECT_FALSE(player.InvokeEvaluateIsStreaming());
}

TEST_F(TestVideoPlayer, IsStreamingManifestExtensionsReturnsTrue)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  player.SetItem(CFileItem("http://example.com/playlist.m3u8", false));
  EXPECT_TRUE(player.InvokeEvaluateIsStreaming());

  player.SetItem(CFileItem("https://example.com/manifest.ism", false));
  EXPECT_TRUE(player.InvokeEvaluateIsStreaming());

  player.SetItem(CFileItem("https://example.com/path1/path2.ism/manifest?query=foo", false));
  EXPECT_TRUE(player.InvokeEvaluateIsStreaming());

  player.SetItem(CFileItem("http://example.com/PLAYLIST.MPD", false));
  EXPECT_TRUE(player.InvokeEvaluateIsStreaming());
}

TEST_F(TestVideoPlayer, IsStreamingMimeTypesReturnsTrue)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  CFileItem itemHls("http://example.com/stream", false);
  itemHls.SetMimeType("application/vnd.apple.mpegurl");
  player.SetItem(itemHls);
  EXPECT_TRUE(player.InvokeEvaluateIsStreaming());

  CFileItem itemDash("http://example.com/stream", false);
  itemDash.SetMimeType("application/dash+xml");
  player.SetItem(itemDash);
  EXPECT_TRUE(player.InvokeEvaluateIsStreaming());
}

TEST_F(TestVideoPlayer, IsStreamingProtocolsReturnsTrue)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  player.SetItem(CFileItem("hls://example.com/live", false));
  EXPECT_TRUE(player.InvokeEvaluateIsStreaming());

  player.SetItem(CFileItem("dash://example.com/live", false));
  EXPECT_TRUE(player.InvokeEvaluateIsStreaming());

  player.SetItem(CFileItem("rtp://239.255.0.1:5004", false));
  EXPECT_TRUE(player.InvokeEvaluateIsStreaming());
}

TEST_F(TestVideoPlayer, IsStreamingItemPropertiesReturnsTrue)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  CFileItem itemAddon("http://example.com/stream", false);
  itemAddon.SetProperty(STREAM_PROPERTY_INPUTSTREAM, "inputstream.adaptive");
  player.SetItem(itemAddon);
  EXPECT_TRUE(player.InvokeEvaluateIsStreaming());
}

TEST_F(TestVideoPlayer, IsStreamingInputStreamTypes)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);
  CFileItem item("http://example.com/generic", false);
  player.SetItem(item);

  //! @todo should partially move into InputStream unit tests and leave the test of IsStreaming here

  // File inputstream -> false
  player.SetInputStream(std::make_shared<CTestInputStream>(DVDSTREAM_TYPE_FILE, item));
  EXPECT_FALSE(player.InvokeEvaluateIsStreaming());

  // Addon inputstream (ex. inputstream.adaptive)
  player.SetInputStream(std::make_shared<CTestInputStream>(DVDSTREAM_TYPE_ADDON, item));
  EXPECT_TRUE(player.InvokeEvaluateIsStreaming());

  // FFmpeg inputstream
  player.SetInputStream(std::make_shared<CTestInputStream>(DVDSTREAM_TYPE_FFMPEG, item));
  EXPECT_TRUE(player.InvokeEvaluateIsStreaming());
}

TEST_F(TestVideoPlayer, IsStreamingDemuxerReturnsTrue)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);
  CFileItem item("http://example.com/generic", false);
  player.SetItem(item);

  player.SetDemuxer(std::make_unique<CTestDemux>(false));
  EXPECT_FALSE(player.InvokeEvaluateIsStreaming());

  player.SetDemuxer(std::make_unique<CTestDemux>(true));
  EXPECT_TRUE(player.InvokeEvaluateIsStreaming());
}
