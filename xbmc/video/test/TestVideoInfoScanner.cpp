/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "video/VideoInfoScanner.h"
#include "FileItem.h"
#include "gtest/gtest.h"

using namespace VIDEO;
using ::testing::Test;
using ::testing::WithParamInterface;
using ::testing::ValuesIn;

typedef struct
{
  const char* path;
  int season;
  int episode;
} TestEntry;

static const TestEntry TestData[] = {
  //season+episode
  {"foo.S02E03.mkv",   2, 3},
  {"foo.203.mkv",      2, 3},
  //episode only
  {"foo.Ep03.mkv",     1, 3},
  {"foo.Ep_03.mkv",    1, 3},
  {"foo.Part.III.mkv", 1, 3},
  {"foo.Part.3.mkv",   1, 3},
  {"foo.E03.mkv",      1, 3},
  {"foo.2009.E03.mkv", 1, 3},
};

class TestVideoInfoScanner : public Test,
                             public WithParamInterface<TestEntry>
{
};

TEST_P(TestVideoInfoScanner, EnumerateEpisodeItem)
{
  const TestEntry& entry = GetParam();
  CVideoInfoScanner scanner;
  CFileItem item(entry.path, false);
  EPISODE expected(entry.season, entry.episode, 0, false);

  EPISODELIST result;
  ASSERT_TRUE(scanner.EnumerateEpisodeItem(&item, result));
  EXPECT_TRUE(expected == result[0]);
}

INSTANTIATE_TEST_CASE_P(VideoInfoScanner, TestVideoInfoScanner, ValuesIn(TestData));