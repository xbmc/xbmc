/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/LanguageTag.h"
#include "video/VideoStreamSelect.h"

#include <gtest/gtest.h>

using namespace KODI::VIDEO;
using KODI::UTILS::CLanguageTag;

namespace
{

// ---------------------------------------------------------------------------
// Video tests
// ---------------------------------------------------------------------------

struct MockVideoStreamInfo
{
  int streamId;
  std::string language;
  std::string codecName;
  StreamHdrType hdrType{StreamHdrType::HDR_TYPE_NONE};
  float fps{0.0f}; // stored directly in VideoStreamInfoExt (pre-computed)
  int height{0};
  int width{0};
  int bitrate{0};
};

struct VideoSortTestParam
{
  std::string name;
  std::vector<MockVideoStreamInfo> inputStreams;
  CVideoStreamSelect::TrackOrder order;
  std::vector<int> expectedStreamIds;
};

std::ostream& operator<<(std::ostream& os, const VideoSortTestParam& param)
{
  return os << param.name;
}

const VideoSortTestParam VideoSortTests[] = {
    // MEDIA order: streams remain in their original sequence
    {"MediaOrder",
     {{0, "fre", "h264"}, {1, "eng", "h264"}},
     CVideoStreamSelect::TrackOrder::MEDIA,
     {0, 1}},

    // SORTED order: sort streams using their attributes
    {"LanguageAlphabetical",
     {{0, "fre", "h264"}, {1, "eng", "h264"}},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},

    {"CodecName",
     {{0, "eng", "hevc"}, {1, "eng", "h264"}},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},

    {"HdrType",
     {{0, "eng", "hevc", StreamHdrType::HDR_TYPE_HDR10},
      {1, "eng", "hevc", StreamHdrType::HDR_TYPE_NONE}},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},

    {"Fps",
     {{0, "eng", "hevc", StreamHdrType::HDR_TYPE_NONE, /*fps=*/60.0f},
      {1, "eng", "hevc", StreamHdrType::HDR_TYPE_NONE, /*fps=*/24.0f}},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},

    {"Height",
     {{0, "eng", "hevc", StreamHdrType::HDR_TYPE_NONE, 24.0f, /*height=*/2160},
      {1, "eng", "hevc", StreamHdrType::HDR_TYPE_NONE, 24.0f, /*height=*/1080}},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},

    {"Width",
     {{0, "eng", "hevc", StreamHdrType::HDR_TYPE_NONE, 24.0f, 1080, /*width=*/1920},
      {1, "eng", "hevc", StreamHdrType::HDR_TYPE_NONE, 24.0f, 1080, /*width=*/1280}},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},

    {"Bitrate",
     {{0, "eng", "hevc", StreamHdrType::HDR_TYPE_NONE, 24.0f, 1080, 1280, /*bitrate=*/8000000},
      {1, "eng", "hevc", StreamHdrType::HDR_TYPE_NONE, 24.0f, 1080, 1280, /*bitrate=*/4000000}},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},

    // Single stream unchanged
    {"SingleStreamMedia", {{0, "eng", "h264"}}, CVideoStreamSelect::TrackOrder::MEDIA, {0}},
    {"SingleStreamSorted", {{0, "eng", "h264"}}, CVideoStreamSelect::TrackOrder::SORTED, {0}},

    // Already-sorted input must remain stable.
    {"AlreadySorted",
     {{0, "eng", "h264"}, {1, "fre", "h264"}},
     CVideoStreamSelect::TrackOrder::SORTED,
     {0, 1}},
};
} // namespace

class VideoStreamSelectVideoOrderTest : public testing::Test,
                                        public testing::WithParamInterface<VideoSortTestParam>
{
};

TEST_P(VideoStreamSelectVideoOrderTest, OrderVideo)
{
  const auto& params = GetParam();

  // Instantiate SubtitleStreamInfoExt at execution time to avoid possible g_LangCodeExpander
  // static initialization order issues.
  std::vector<VideoStreamInfoExt> streams;
  streams.reserve(params.inputStreams.size());
  for (const auto& mock : params.inputStreams)
  {
    VideoStreamInfo info;
    info.language = CLanguageTag::Parse(mock.language);
    info.codecName = mock.codecName;
    info.hdrType = mock.hdrType;
    info.height = mock.height;
    info.width = mock.width;
    info.bitrate = mock.bitrate;
    // Encode fps as fpsRate with fpsScale=1 so VideoStreamInfoExt computes the
    // right float without floating-point error.
    info.fpsRate = static_cast<uint32_t>(mock.fps);
    info.fpsScale = 1;

    streams.emplace_back(mock.streamId, info);
  }

  CVideoStreamSelect::OrderVideoStreams(streams, params.order);

  ASSERT_EQ(params.expectedStreamIds.size(), streams.size());
  //! @todo C++23 use zip algorithm
  for (size_t i = 0; i < streams.size(); ++i)
  {
    EXPECT_EQ(params.expectedStreamIds[i], streams[i].streamId);
  }
}

INSTANTIATE_TEST_SUITE_P(TestVideoStreamSelect,
                         VideoStreamSelectVideoOrderTest,
                         testing::ValuesIn(VideoSortTests));

// ---------------------------------------------------------------------------
// Audio tests
// ---------------------------------------------------------------------------

namespace
{
struct MockAudioStreamInfo
{
  int streamId;
  std::string language;
  StreamFlags flags;
  int channels{0};
  int bitrate{0};
  int samplerate{0};
  std::string codecName;
};

// work around -Werror=missing-field-initializers on some platform builds
MockAudioStreamInfo MakeAudioStream(int streamId,
                                    std::string language,
                                    StreamFlags flags,
                                    int channels = 0,
                                    int bitrate = 0,
                                    int samplerate = 0,
                                    std::string codecName = "")
{
  return {streamId,   std::move(language), flags, channels, bitrate,
          samplerate, std::move(codecName)};
}

struct AudioSortTestParam
{
  std::string name;
  std::vector<MockAudioStreamInfo> inputStreams;
  CVideoStreamSelect::TrackOrder order;
  std::vector<int> expectedStreamIds;
};

std::ostream& operator<<(std::ostream& os, const AudioSortTestParam& param)
{
  return os << param.name;
}

const AudioSortTestParam AudioSortTests[] = {
    // MEDIA order: streams remain in their original sequence
    {"MediaOrder",
     {MakeAudioStream(0, "fre", FLAG_NONE), MakeAudioStream(1, "eng", FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::MEDIA,
     {0, 1}},
    // SORTED order: sort streams using their attributes
    {"LanguageAlphabetical",
     {MakeAudioStream(0, "fre", FLAG_NONE), MakeAudioStream(1, "eng", FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    {"OriginalFlag",
     {MakeAudioStream(0, "eng", FLAG_ORIGINAL), MakeAudioStream(1, "eng", FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    {"HearingImpaired",
     {MakeAudioStream(0, "eng", FLAG_HEARING_IMPAIRED), MakeAudioStream(1, "eng", FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    {"VisualImpaired",
     {MakeAudioStream(0, "eng", FLAG_VISUAL_IMPAIRED), MakeAudioStream(1, "eng", FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    {"Forced",
     {MakeAudioStream(0, "eng", FLAG_FORCED), MakeAudioStream(1, "eng", FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    {"Channels",
     {MakeAudioStream(0, "en", FLAG_NONE, /*channels=*/6),
      MakeAudioStream(1, "en", FLAG_NONE, /*channels=*/2)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    {"Bitrate",
     {MakeAudioStream(0, "en", FLAG_NONE, 2, /*bitrate=*/320000),
      MakeAudioStream(1, "en", FLAG_NONE, 2, /*bitrate=*/128000)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    {"Samplerate",
     {MakeAudioStream(0, "en", FLAG_NONE, 2, 128000, /*samplerate=*/48000),
      MakeAudioStream(1, "en", FLAG_NONE, 2, 128000, /*samplerate=*/44100)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    // Same samplerate: alphabetical codec name.
    {"CodecName",
     {MakeAudioStream(0, "en", FLAG_NONE, 2, 128000, 44100, "dts"),
      MakeAudioStream(1, "en", FLAG_NONE, 2, 128000, 44100, "aac")},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    // Single stream unchanged
    {"SingleStreamMedia",
     {MakeAudioStream(0, "en", FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::MEDIA,
     {0}},
    {"SingleStreamSorted",
     {MakeAudioStream(0, "en", FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {0}},
    // Already-sorted input must remain stable.
    {"AlreadySorted",
     {MakeAudioStream(0, "en", FLAG_NONE, 2, 128000, 44100, "aac"),
      MakeAudioStream(1, "fr", FLAG_NONE, 2, 128000, 44100, "aac")},
     CVideoStreamSelect::TrackOrder::SORTED,
     {0, 1}},
};
} // namespace

class VideoStreamSelectAudioOrderTest : public testing::Test,
                                        public testing::WithParamInterface<AudioSortTestParam>
{
};

TEST_P(VideoStreamSelectAudioOrderTest, OrderAudio)
{
  const auto& params = GetParam();

  // Instantiate AudioStreamInfoExt at execution time to avoid possible g_LangCodeExpander
  // static initialization order issues.
  std::vector<AudioStreamInfoExt> streams;
  streams.reserve(params.inputStreams.size());
  for (const auto& mock : params.inputStreams)
  {
    AudioStreamInfo info;
    info.language = CLanguageTag::Parse(mock.language);
    info.codecName = mock.codecName;
    info.flags = mock.flags;
    info.channels = mock.channels;
    info.samplerate = mock.samplerate;
    info.bitrate = mock.bitrate;

    streams.emplace_back(mock.streamId, info);
  }

  CVideoStreamSelect::OrderAudioStreams(streams, params.order);

  ASSERT_EQ(params.expectedStreamIds.size(), streams.size());
  //! @todo C++23 use zip algorithm
  for (size_t i = 0; i < streams.size(); ++i)
  {
    EXPECT_EQ(params.expectedStreamIds[i], streams[i].streamId);
  }
}

INSTANTIATE_TEST_SUITE_P(TestVideoStreamSelect,
                         VideoStreamSelectAudioOrderTest,
                         testing::ValuesIn(AudioSortTests));

// ---------------------------------------------------------------------------
// Subtitle tests
// ---------------------------------------------------------------------------
namespace
{
struct MockSubtitleStreamInfo
{
  int streamId;
  std::string language;
  bool isExternal;
  StreamFlags flags;
  std::string codecName;
};

// work around -Werror=missing-field-initializers on some platform builds
MockSubtitleStreamInfo MakeSubtitleStream(int streamId,
                                          std::string language,
                                          bool isExternal,
                                          StreamFlags flags,
                                          std::string codecName = "")
{
  return {streamId, std::move(language), isExternal, flags, std::move(codecName)};
}

struct SubtitleSortTestParam
{
  std::string name;
  std::vector<MockSubtitleStreamInfo> inputStreams;
  CVideoStreamSelect::TrackOrder order;
  std::vector<int> expectedStreamIds;
};

std::ostream& operator<<(std::ostream& os, const SubtitleSortTestParam& param)
{
  return os << param.name;
}

const SubtitleSortTestParam SubtitleSortTests[] = {
    // MEDIA order: streams remain in their original sequence
    {"MediaOrder",
     {MakeSubtitleStream(0, "fre", false, FLAG_NONE),
      MakeSubtitleStream(1, "eng", true, FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::MEDIA,
     {0, 1}},
    // SORTED order: sort streams using their attributes
    {"ExternalFirst",
     {MakeSubtitleStream(0, "eng", false, FLAG_NONE),
      MakeSubtitleStream(1, "eng", true, FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    {"LanguageAlphabetical",
     {MakeSubtitleStream(0, "fre", false, FLAG_NONE),
      MakeSubtitleStream(1, "eng", false, FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    {"OriginalFlag",
     {MakeSubtitleStream(0, "eng", false, FLAG_ORIGINAL),
      MakeSubtitleStream(1, "eng", false, FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    {"HearingImpaired",
     {MakeSubtitleStream(0, "eng", false, FLAG_HEARING_IMPAIRED),
      MakeSubtitleStream(1, "eng", false, FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    {"VisualImpaired",
     {MakeSubtitleStream(0, "eng", false, FLAG_VISUAL_IMPAIRED),
      MakeSubtitleStream(1, "eng", false, FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    {"Forced",
     {MakeSubtitleStream(0, "eng", false, FLAG_FORCED),
      MakeSubtitleStream(1, "eng", false, FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    {"CodecName",
     {MakeSubtitleStream(0, "eng", false, FLAG_NONE, "subrip"),
      MakeSubtitleStream(1, "eng", false, FLAG_NONE, "ass")},
     CVideoStreamSelect::TrackOrder::SORTED,
     {1, 0}},
    // Single stream unchanged
    {"SingleStreamMedia",
     {MakeSubtitleStream(0, "en", false, FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::MEDIA,
     {0}},
    {"SingleStreamSorted",
     {MakeSubtitleStream(0, "en", false, FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {0}},
    // Already-sorted input must remain stable.
    {"AlreadySorted",
     {MakeSubtitleStream(0, "en", false, FLAG_NONE), MakeSubtitleStream(1, "fr", false, FLAG_NONE)},
     CVideoStreamSelect::TrackOrder::SORTED,
     {0, 1}},
};
} // namespace

class VideoStreamSelectSubtitleOrderTest : public testing::Test,
                                           public testing::WithParamInterface<SubtitleSortTestParam>
{
};

TEST_P(VideoStreamSelectSubtitleOrderTest, OrderSubtitles)
{
  const auto& params = GetParam();

  // Instantiate SubtitleStreamInfoExt at execution time to avoid possible g_LangCodeExpander
  // static initialization order issues.
  std::vector<SubtitleStreamInfoExt> streams;
  streams.reserve(params.inputStreams.size());
  for (const auto& mock : params.inputStreams)
  {
    SubtitleStreamInfo info;
    info.language = CLanguageTag::Parse(mock.language);
    info.codecName = mock.codecName;
    info.flags = mock.flags;
    info.isExternal = mock.isExternal;

    streams.emplace_back(mock.streamId, info);
  }

  CVideoStreamSelect::OrderSubtitleStreams(streams, params.order);

  ASSERT_EQ(params.expectedStreamIds.size(), streams.size());
  //! @todo C++23 use zip algorithm
  for (size_t i = 0; i < streams.size(); ++i)
  {
    EXPECT_EQ(params.expectedStreamIds[i], streams[i].streamId);
  }
}

INSTANTIATE_TEST_SUITE_P(TestVideoStreamSelect,
                         VideoStreamSelectSubtitleOrderTest,
                         testing::ValuesIn(SubtitleSortTests));
