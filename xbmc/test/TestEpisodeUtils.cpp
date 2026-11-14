/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/EpisodeUtils.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

using ::testing::Test;
using ::testing::ValuesIn;
using ::testing::WithParamInterface;

struct TestEntry
{
  const char* path;
  std::vector<KODI::VIDEO::EPISODE> ranges;
  std::vector<KODI::VIDEO::EPISODE> noranges;
};

static const TestEntry TestData[] = {
    //season+episode
    {"foo.S02E03.mkv", {{2, 3}}, {{2, 3}}},
    {"foo.S2E3.mkv", {{2, 3}}, {{2, 3}}},
    {"foo.S02.E03.mkv", {{2, 3}}, {{2, 3}}},
    {"foo.S02_E03.mkv", {{2, 3}}, {{2, 3}}},
    {"foo.S02xE03.mkv", {{2, 3}}, {{2, 3}}},
    {"foo.2x03.mkv", {{2, 3}}, {{2, 3}}},
    {"foo.203.mkv", {{2, 3}}, {{2, 3}}},
    {"24 S01E01 12:00AM - 1:00AM.mkv", {{1, 1}}, {{1, 1}}},
    {"Miracle Workers S01E02 13 Days.mkv", {{1, 2}}, {{1, 2}}},
    //episode only
    {"foo.Ep03.mkv", {{1, 3}}, {{1, 3}}},
    {"foo.Ep_03.mkv", {{1, 3}}, {{1, 3}}},
    {"foo.Part.III.mkv", {{1, 3}}, {{1, 3}}},
    {"foo.Part.3.mkv", {{1, 3}}, {{1, 3}}},
    {"foo.pt.III.mkv", {{1, 3}}, {{1, 3}}},
    {"foo.pt_IV.mkv", {{1, 4}}, {{1, 4}}},
    {"foo.E03.mkv", {{1, 3}}, {{1, 3}}},
    {"foo.2009.E03.mkv", {{1, 3}}, {{1, 3}}},
    // multi-episode (not ranges)
    {"The Legend of Korra - S01E01-02 - Welcome to Republic City & A Leaf in the Wind.mkv",
     {{1, 1}, {1, 2}},
     {{1, 1}, {1, 2}}},
    {"foo s01e01-episode1.title-s01e02-episode2.title.mkv", {{1, 1}, {1, 2}}, {{1, 1}, {1, 2}}},
    {"foo.S01E01E02.mkv", {{1, 1}, {1, 2}}, {{1, 1}, {1, 2}}},
    {"foo.S01E03E04E05.mkv", {{1, 3}, {1, 4}, {1, 5}}, {{1, 3}, {1, 4}, {1, 5}}},
    {"foo.S02E01-S02E02.mkv", {{2, 1}, {2, 2}}, {{2, 1}, {2, 2}}},
    {"foo S02E01-bar-S02E02-bar.mkv", {{2, 1}, {2, 2}}, {{2, 1}, {2, 2}}},
    {"foo S02E01-S02E02-S02E03.mkv", {{2, 1}, {2, 2}, {2, 3}}, {{2, 1}, {2, 2}, {2, 3}}},
    {"foo 2x01-2x02.mkv", {{2, 1}, {2, 2}}, {{2, 1}, {2, 2}}},
    {"foo ep01-ep02.mkv", {{1, 1}, {1, 2}}, {{1, 1}, {1, 2}}},
    {"foo S02E01-02-03.mkv", {{2, 1}, {2, 2}, {2, 3}}, {{2, 1}, {2, 2}, {2, 3}}},
    {"foo 2x01x02.mkv", {{2, 1}, {2, 2}}, {{2, 1}, {2, 2}}},
    {"foo ep01-02.mkv", {{1, 1}, {1, 2}}, {{1, 1}, {1, 2}}},
    {"foo.S02E01S02E03.mkv", {{2, 1}, {2, 3}}, {{2, 1}, {2, 3}}},
    // spanning episode ranges
    {"foo.ep03-ep05.mkv", {{1, 3}, {1, 4}, {1, 5}}, {{1, 3}, {1, 5}}},
    {"foo.S05E03-E05.mkv", {{5, 3}, {5, 4}, {5, 5}}, {{5, 3}, {5, 5}}},
    {"foo.S05E03-S05E05.mkv", {{5, 3}, {5, 4}, {5, 5}}, {{5, 3}, {5, 5}}},
    {"foo.S05E01-E03-E05.mkv", {{5, 1}, {5, 2}, {5, 3}, {5, 4}, {5, 5}}, {{5, 1}, {5, 3}, {5, 5}}},
    {"foo.S05E01-S05E03-S05E05.mkv",
     {{5, 1}, {5, 2}, {5, 3}, {5, 4}, {5, 5}},
     {{5, 1}, {5, 3}, {5, 5}}},
    {"foo.5x03-5x05.mkv", {{5, 3}, {5, 4}, {5, 5}}, {{5, 3}, {5, 5}}},
    // range with single episode after
    {"foo.S01E01-E03E05.mkv", {{1, 1}, {1, 2}, {1, 3}, {1, 5}}, {{1, 1}, {1, 3}, {1, 5}}},
    {"foo.S01E01-E03S01E05.mkv", {{1, 1}, {1, 2}, {1, 3}, {1, 5}}, {{1, 1}, {1, 3}, {1, 5}}},
    // not ranges
    {"foo.E03-05.mkv", {{1, 3}, {1, 5}}, {{1, 3}, {1, 5}}},
    {"foo.ep03-05.mkv", {{1, 3}, {1, 5}}, {{1, 3}, {1, 5}}},
    {"foo.S05E03-05.mkv", {{5, 3}, {5, 5}}, {{5, 3}, {5, 5}}},
    // multi-season
    {"foo.S00E100S05E03E04E05.mkv",
     {{0, 100}, {5, 3}, {5, 4}, {5, 5}},
     {{0, 100}, {5, 3}, {5, 4}, {5, 5}}},
    {"foo.S01E01E02S02E01.mkv", {{1, 1}, {1, 2}, {2, 1}}, {{1, 1}, {1, 2}, {2, 1}}},
    {"foo.S00E01-E04S05E03-E06.mkv",
     {{0, 1}, {0, 2}, {0, 3}, {0, 4}, {5, 3}, {5, 4}, {5, 5}, {5, 6}},
     {{0, 1}, {0, 4}, {5, 3}, {5, 6}}},
    {"foo.S02E01S03E05E09-E12.mkv",
     {{2, 1}, {3, 5}, {3, 9}, {3, 10}, {3, 11}, {3, 12}},
     {{2, 1}, {3, 5}, {3, 9}, {3, 12}}},
    // expected (partial) range failures
    {"foo.S01E01-E100.mkv", {}, {{1, 1}, {1, 100}}}, // episode range too large
    {"foo.S01E01-S02E03.mkv", {}, {{1, 1}, {2, 3}}}, // cannot determine number of episodes in S01
    {"foo.S01E03-S01E01.mkv", {}, {{1, 3}, {1, 1}}}, // range backwards
    {"foo.S01E03-E01.mkv", {}, {{1, 3}, {1, 1}}}, // range backwards
    {"foo S00E01E03-S01E01.mkv", {{0, 1}}, {{0, 1}, {0, 3}, {1, 1}}}, // invalid range
    {"foo.S00E01-E04S01E01-S02E03.mkv",
     {{0, 1}, {0, 2}, {0, 3}, {0, 4}},
     {{0, 1}, {0, 4}, {1, 1}, {2, 3}}}, // second range invalid
    {"foo.S01E03-S02E02S03E01-E04.mkv",
     {{3, 1}, {3, 2}, {3, 3}, {3, 4}},
     {{1, 3}, {2, 2}, {3, 1}, {3, 4}}}, // first range invalid
    {"foo.S02E03-1080p.mkv", {{2, 3}}, {{2, 3}}}, // resolution
    {"foo.S02E03_20231105123456.mkv", {{2, 3}}, {{2, 3}}}, // timestamp
    {"foo.S02E03-special.mkv", {{2, 3}}, {{2, 3}}}, // comment (starting with s)
    {"foo.S02E03-extended.mkv", {{2, 3}}, {{2, 3}}}, // comment (starting with e)
    {"foo.S01E01_2023-11-05.mkv", {{1, 1}}, {{1, 1}}}, // date
    {"foo.S01E01-2023-11-05.mkv", {{1, 1}}, {{1, 1}}}, // date
    {"foo.S01E01_x264.mkv", {{1, 1}}, {{1, 1}}}, // codec
    {"foo.S01E01-x264.mkv", {{1, 1}}, {{1, 1}}}, // codec
};

class TestEnumerateEpisodes : public Test, public WithParamInterface<TestEntry>
{
};

TEST_P(TestEnumerateEpisodes, EnumerateEpisodeItem)
{
  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  const TestEntry& entry = GetParam();

  CFileItem item(entry.path, false);
  KODI::VIDEO::EPISODELIST result;

  const bool saveState{advancedSettings->m_disableEpisodeRanges};
  advancedSettings->m_disableEpisodeRanges = false;
  ASSERT_TRUE(CEpisodeUtils::EnumerateEpisodeItem(&item, result));
  EXPECT_EQ(entry.ranges, result);

  advancedSettings->m_disableEpisodeRanges = true;
  result.clear();
  ASSERT_TRUE(CEpisodeUtils::EnumerateEpisodeItem(&item, result));
  EXPECT_EQ(entry.noranges, result);
  advancedSettings->m_disableEpisodeRanges = saveState;
}

INSTANTIATE_TEST_SUITE_P(EnumerateEpisodeItem, TestEnumerateEpisodes, ValuesIn(TestData));

struct DateTestEntry
{
  const char* path;
  KODI::VIDEO::EPISODE episode;
  //! yyyy-mm-dd, or empty for an episode found by its number
  const char* date;
};

static const DateTestEntry DateTestData[] = {
    // in the file's name
    {"/Show/Show.2023-11-05.mkv", {}, "2023-11-05"},
    {"/Show/Show.11.05.2023.mkv", {}, "2023-11-05"},
    {"/Show/2023-11-04/Show.2023-11-05.mkv", {}, "2023-11-05"},
    // in a folder's name, for a file whose name holds neither a date nor an episode
    {"/Show/2023-11-05/episode.mkv", {}, "2023-11-05"},
    {"/Show/11-05-2023/episode.mkv", {}, "2023-11-05"},
    // in a folder's name, for a file whose name holds only a bare number, which may be a year
    {"/Show/Recorded 2023-11-05/Show 105.mkv", {}, "2023-11-05"},
    {"/Show/2023-11-05/Show_BBC_2023_11_05.wtv", {}, "2023-11-05"},
    // in a folder's name, for a file whose name holds an episode
    {"/Show/Recorded 2023-11-05/Show S01E05.mkv", {1, 5}, ""},
    {"/Show/Recorded 2023-11-05/Show 1x05.mkv", {1, 5}, ""},
    {"/Show/Recorded 11-05-2023/Show 1x05.mkv", {1, 5}, ""},
    {"/Show/Recorded 2023-11-05/Show.Part.5.mkv", {1, 5}, ""},
    {"C:\\Show\\Recorded 2023-11-05\\Show 1x05.mkv", {1, 5}, ""},
    {"/Show/Recorded 2023-11-05/Part 5.mkv", {1, 5}, ""},
    {"C:\\Show\\Recorded 2023-11-05\\Part 5.mkv", {1, 5}, ""},
    // a Part in a folder's name is no episode
    {"/Show/2023-11-05/Show.Part.5.extra/video.mkv", {}, "2023-11-05"},
    {"C:\\Show\\2023-11-05\\Show.Part.5.extra\\video.mkv", {}, "2023-11-05"},
};

class TestEnumerateEpisodesByDate : public Test, public WithParamInterface<DateTestEntry>
{
};

TEST_P(TestEnumerateEpisodesByDate, EnumerateEpisodeItem)
{
  const DateTestEntry& entry = GetParam();
  SCOPED_TRACE(entry.path);

  CFileItem item(entry.path, false);
  KODI::VIDEO::EPISODELIST result;
  ASSERT_TRUE(CEpisodeUtils::EnumerateEpisodeItem(&item, result));
  ASSERT_EQ(1U, result.size());
  EXPECT_EQ(entry.episode, result[0]);
  EXPECT_EQ(entry.date, result[0].cDate.IsValid() ? result[0].cDate.GetAsDBDate() : "");
}

INSTANTIATE_TEST_SUITE_P(EnumerateEpisodeItem, TestEnumerateEpisodesByDate, ValuesIn(DateTestData));

TEST(TestEnumerateEpisodes, EnumerateEpisodeItemByTitle)
{
  CFileItem item("/foo.special.mp4", false);
  KODI::VIDEO::EPISODELIST result;
  ASSERT_TRUE(CEpisodeUtils::EnumerateEpisodeItem(&item, result));
  ASSERT_EQ(result.size(), 1);
  ASSERT_EQ(result[0].strTitle, "foo");
}
