/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "test/TestUtils.h"
#include "utils/Mp4ChplReader.h"

#include <gtest/gtest.h>

class TestMp4ChplReader : public ::testing::Test
{
};

// Valid chapter file
TEST_F(TestMp4ChplReader, CanParseValidChplAtom)
{
  const CURL url(XBMC_REF_FILE_PATH("xbmc/utils/test/resources/sample_chpl.mp4"));
  std::vector<ChplChapter> chapters;

  ChplChapterResult result = CChplChapterReader::ScanNeroChapters(url, chapters);

  ASSERT_TRUE(result);
  EXPECT_EQ(result.status, chplFound);
  EXPECT_GT(chapters.size(), 0);
}

// MP4 with no chapters
TEST_F(TestMp4ChplReader, ReturnsNoneWhenNoChplAtom)
{
  const CURL url(XBMC_REF_FILE_PATH("xbmc/utils/test/resources/no_chapters.mp4"));
  std::vector<ChplChapter> chapters;

  ChplChapterResult result = CChplChapterReader::ScanNeroChapters(url, chapters);

  EXPECT_EQ(result.status, chplNone);
  EXPECT_TRUE(chapters.empty());
}

// Corrupt or truncated chpl atom
TEST_F(TestMp4ChplReader, FailsGracefullyOnCorruptChplAtom)
{
  const CURL url(XBMC_REF_FILE_PATH("xbmc/utils/test/resources/corrupt_chpl.mp4"));
  std::vector<ChplChapter> chapters;

  ChplChapterResult result = CChplChapterReader::ScanNeroChapters(url, chapters);

  EXPECT_EQ(result.status, chplError);
  EXPECT_TRUE(result.errorMessage.has_value());
}
