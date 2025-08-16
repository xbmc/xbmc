/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "video/VideoInfoScanner.h"

#include <gtest/gtest.h>

using namespace KODI;
using ::testing::Test;
using ::testing::WithParamInterface;
using ::testing::ValuesIn;

struct TestEntry
{
  const char* path;
  std::vector<VIDEO::EPISODE> ranges;
  std::vector<VIDEO::EPISODE> noranges;
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
    {"foo.S02E03-special.mkv", {{2, 3}}, {{2, 3}}}, // comment (starting with s)
    {"foo.S02E03-extended.mkv", {{2, 3}}, {{2, 3}}}, // comment (starting with e)
};

class TestVideoInfoScanner : public Test,
                             public WithParamInterface<TestEntry>
{
};

TEST_P(TestVideoInfoScanner, EnumerateEpisodeItem)
{
  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  const TestEntry& entry = GetParam();

  CFileItem item(entry.path, false);
  VIDEO::CVideoInfoScanner scanner;
  VIDEO::EPISODELIST result;

  advancedSettings->m_disableEpisodeRanges = false;
  ASSERT_TRUE(scanner.EnumerateEpisodeItem(&item, result));
  EXPECT_EQ(entry.ranges, result);

  advancedSettings->m_disableEpisodeRanges = true;
  result.clear();
  ASSERT_TRUE(scanner.EnumerateEpisodeItem(&item, result));
  EXPECT_EQ(entry.noranges, result);
}

INSTANTIATE_TEST_SUITE_P(VideoInfoScanner, TestVideoInfoScanner, ValuesIn(TestData));

TEST(TestVideoInfoScanner, EnumerateEpisodeItemByTitle)
{
  VIDEO::CVideoInfoScanner scanner;
  CFileItem item("/foo.special.mp4", false);
  VIDEO::EPISODELIST result;
  ASSERT_TRUE(scanner.EnumerateEpisodeItem(&item, result));
  ASSERT_EQ(result.size(), 1);
  ASSERT_EQ(result[0].strTitle, "foo");
}
