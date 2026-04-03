/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"

#include <algorithm>
#include <string>

#include <gtest/gtest.h>

extern "C"
{
#include <libavformat/avformat.h>
}

using namespace std::chrono_literals;

struct TestAVChapter
{
  int timebasenum;
  int timebaseden;
  int64_t start;
  int64_t end;
  std::string title;
};

struct TestChapter
{
  std::vector<TestAVChapter> input;
  std::vector<ChapterFFmpeg> expected;
};

// clang-format off
const TestChapter testChapters[] = {
  {{{{1, 1, 0, 1, "A"}, {1, 1, 1, 2, "B"}}},
    {{{0s, 1s, "A"}, {1s, 2s, "B"}}}},
  // Test timebase
  {{{{1, 1000, 0, 10000, "A"}, {20, 500, 250, 500, "B"}}},
    {{{0s, 10s, "A"}, {10s, 20s, "B"}}}},
  // Overlapping chapters
  {{{{1, 1, 0, 11, "A"}, {1, 1, 10, 20, "B"}}},
    {{{0s, 11s, "A"}, {10s, 20s, "B"}}}},
  // Chapter markers
  {{{{1, 1, 0, 0, "A"}, {1, 1, 10, 10, "B"}}},
    {{{0s, 0s, "A"}, {10s, 10s, "B"}}}},
  {{{{1, 1, 0, 0, "A"}, {1, 1, 10, 0, "B"}}},
    {{{0s, 0s, "A"}, {10s, 0s, "B"}}}},
  // Out of order chapters
  {{{{1, 1, 10, 20, "B"}, {1, 1, 0, 10, "A"}}},
    {{{0s, 10s, "A"}, {10s, 20s, "B"}}}},
  // 1st chapter is not at 00:00:00
  {{{{1, 1, 1, 2, "A"}}},
    {{{0s, 0s, ""}, {1s, 2s, "A"}}}},
};
// clang-format on

class ChapterLoaderTester : public testing::Test, public testing::WithParamInterface<TestChapter>
{
};

TEST_P(ChapterLoaderTester, LoadChapters)
{
  auto& param = GetParam();

  // Build a temporary AVChapter**, as it would be found in AVFormatContext::chapters
  std::vector<AVChapter> avChapters;
  avChapters.reserve(param.input.size());

  std::ranges::transform(param.input, std::back_inserter(avChapters),
                         [](const TestAVChapter& c)
                         {
                           AVChapter avc{.id = 0,
                                         .time_base{.num = c.timebasenum, .den = c.timebaseden},
                                         .start = c.start,
                                         .end = c.end,
                                         .metadata = nullptr};
                           av_dict_set(&avc.metadata, "title", c.title.c_str(), 0);
                           return avc;
                         });

  std::vector<AVChapter*> ptrAvChapters;
  ptrAvChapters.reserve(avChapters.size());

  std::ranges::transform(avChapters, std::back_inserter(ptrAvChapters),
                         [](AVChapter& c) { return &c; });

  std::vector<ChapterFFmpeg> actual =
      CDVDDemuxUtils::LoadChapters(std::span<AVChapter*>{ptrAvChapters});

  EXPECT_EQ(param.expected, actual);

  // Cleanup ffmpeg allocated memory
  std::ranges::for_each(avChapters, [](AVChapter& c) { av_dict_free(&c.metadata); });
}

INSTANTIATE_TEST_SUITE_P(TestDVDDemuxUtils, ChapterLoaderTester, testing::ValuesIn(testChapters));

TEST(TestDVDDemuxUtils, ReadChaptersInvalid)
{
  AVChapter** noChapters = nullptr;
  std::vector<ChapterFFmpeg> output =
      CDVDDemuxUtils::LoadChapters(std::span<AVChapter*>{noChapters, 1});
  EXPECT_TRUE(output.empty());

  AVChapter avc;
  AVChapter* avcptr = &avc;
  output = CDVDDemuxUtils::LoadChapters(std::span<AVChapter*>{&avcptr, 0});
  EXPECT_TRUE(output.empty());
}
