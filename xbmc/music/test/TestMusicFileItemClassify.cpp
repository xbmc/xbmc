/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "music/MusicFileItemClassify.h"

#include <array>

#include <gtest/gtest.h>

using namespace KODI;

struct SimpleDefinition
{
  std::string path;
  bool folder;
  bool result;
};

class AudioBookTest : public testing::WithParamInterface<SimpleDefinition>, public testing::Test
{
};

TEST_P(AudioBookTest, IsAudioBook)
{
  EXPECT_EQ(MUSIC::IsAudioBook(CFileItem(GetParam().path, GetParam().folder)), GetParam().result);
}

const auto audiobook_tests = std::array{
    SimpleDefinition{"/home/user/test.m4b", false, true},
    SimpleDefinition{"/home/user/test.m4b", true, true},
    SimpleDefinition{"/home/user/test.mka", false, true},
    SimpleDefinition{"/home/user/test.mka", true, true},
    SimpleDefinition{"/home/user/test.not", false, false},
};

INSTANTIATE_TEST_SUITE_P(TestMusicFileItemClassify,
                         AudioBookTest,
                         testing::ValuesIn(audiobook_tests));

class CuesheetTest : public testing::WithParamInterface<SimpleDefinition>, public testing::Test
{
};

TEST_P(CuesheetTest, IsCUESheet)
{
  EXPECT_EQ(MUSIC::IsCUESheet(CFileItem(GetParam().path, GetParam().folder)), GetParam().result);
}

const auto cuesheet_tests = std::array{
    SimpleDefinition{"/home/user/test.cue", false, true},
    SimpleDefinition{"/home/user/test.cue/", true, false},
    SimpleDefinition{"/home/user/test.foo", false, false},
};

INSTANTIATE_TEST_SUITE_P(TestMusicFileItemClassify,
                         CuesheetTest,
                         testing::ValuesIn(cuesheet_tests));

class LyricsTest : public testing::WithParamInterface<SimpleDefinition>, public testing::Test
{
};

TEST_P(LyricsTest, IsLyrics)
{
  EXPECT_EQ(MUSIC::IsLyrics(CFileItem(GetParam().path, GetParam().folder)), GetParam().result);
}

const auto lyrics_tests = std::array{
    SimpleDefinition{"/home/user/test.lrc", false, true},
    SimpleDefinition{"/home/user/test.cdg", false, true},
    SimpleDefinition{"/home/user/test.not", false, false},
    SimpleDefinition{"/home/user/test.lrc/", true, false},
};

INSTANTIATE_TEST_SUITE_P(TestMusicFileItemClassify, LyricsTest, testing::ValuesIn(lyrics_tests));
