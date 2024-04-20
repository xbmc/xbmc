/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "video/VideoInfoScanner.h"

#include <gtest/gtest.h>

using namespace KODI;
using ::testing::Test;
using ::testing::WithParamInterface;
using ::testing::ValuesIn;

typedef struct
{
  const char* path;
  int season;
  int episode[4]; // for multi-episodes
} TestEntry;

static const TestEntry TestData[] = {
  //season+episode
  {"foo.S02E03.mkv",   2, {3} },
  {"foo.203.mkv",      2, {3} },
  //episode only
  {"foo.Ep03.mkv",     1, {3} },
  {"foo.Ep_03.mkv",    1, {3} },
  {"foo.Part.III.mkv", 1, {3} },
  {"foo.Part.3.mkv",   1, {3} },
  {"foo.E03.mkv",      1, {3} },
  {"foo.2009.E03.mkv", 1, {3} },
  // multi-episode
  {"The Legend of Korra - S01E01-02 - Welcome to Republic City & A Leaf in the Wind.mkv", 1, { 1, 2 } },
  {"foo.S01E01E02.mkv", 1, {1,2} },
  {"foo.S01E03E04E05.mkv", 1, {3,4,5} }
};

class TestVideoInfoScanner : public Test,
                             public WithParamInterface<TestEntry>
{
};

TEST_P(TestVideoInfoScanner, EnumerateEpisodeItem)
{
  const TestEntry& entry = GetParam();
  VIDEO::CVideoInfoScanner scanner;
  CFileItem item(entry.path, false);
  VIDEO::EPISODELIST expected;
  for (int i = 0; i < 3 && entry.episode[i]; i++)
    expected.emplace_back(entry.season, entry.episode[i], 0, false);

  VIDEO::EPISODELIST result;
  ASSERT_TRUE(scanner.EnumerateEpisodeItem(&item, result));
  EXPECT_EQ(expected.size(), result.size());
  for (size_t i = 0; i < expected.size(); i++)
    EXPECT_EQ(expected[i], result[i]);
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
