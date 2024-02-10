/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "games/tags/GameInfoTag.h"
#include "music/MusicFileItemClassify.h"
#include "music/tags/MusicInfoTag.h"
#include "pictures/PictureInfoTag.h"
#include "utils/FileExtensionProvider.h"
#include "video/VideoInfoTag.h"

#include <array>

#include <gtest/gtest.h>

using namespace KODI;

struct AudioClassifyTest
{
  AudioClassifyTest(const std::string& path,
                    bool res = true,
                    const std::string& mime = "",
                    int tag_type = 0)
    : item(path, false), result(res)
  {
    if (!mime.empty())
      item.SetMimeType(mime);
    switch (tag_type)
    {
      case 1:
        item.GetVideoInfoTag()->m_strFileNameAndPath = path;
        break;
      case 2:
        item.GetGameInfoTag()->SetGameClient("some_client");
        break;
      case 3:
        item.GetMusicInfoTag()->SetPlayCount(1);
        break;
      case 4:
        item.GetPictureInfoTag()->SetInfo("foo", "bar");
        break;
      default:
        break;
    }
  }

  CFileItem item;
  bool result;
};

class AudioTest : public testing::WithParamInterface<AudioClassifyTest>, public testing::Test
{
};

TEST_P(AudioTest, IsAudio)
{
  EXPECT_EQ(MUSIC::IsAudio(GetParam().item), GetParam().result);
}

const auto audio_tests = std::array{
    AudioClassifyTest{"/home/user/song.avi", true, "audio/mp3"},
    AudioClassifyTest{"/home/user/song.avi", false, "", 1},
    AudioClassifyTest{"/home/user/song.avi", false, "", 2},
    AudioClassifyTest{"/home/user/song.avi", true, "", 3},
    AudioClassifyTest{"/home/user/song.avi", false, "", 4},
    AudioClassifyTest{"cdda://1"},
    AudioClassifyTest{"/home/user/song.avi", true, "application/ogg"},
    AudioClassifyTest{"/home/user/video.not", true, "application/mp4"},
    AudioClassifyTest{"/home/user/video.not", true, "application/mxf"},
};

INSTANTIATE_TEST_SUITE_P(TestMusicFileItemClassify, AudioTest, testing::ValuesIn(audio_tests));

TEST(TestMusicFileItemClassify, MusicExtensions)
{
  const auto& exts = CServiceBroker::GetFileExtensionProvider().GetMusicExtensions();
  for (const auto& ext : StringUtils::Split(exts, "|"))
  {
    if (!ext.empty())
    {
      EXPECT_TRUE(MUSIC::IsAudio(CFileItem(ext, false)));
    }
  }
}

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

class CDDATest : public testing::WithParamInterface<SimpleDefinition>, public testing::Test
{
};

TEST_P(CDDATest, IsCDDA)
{
  EXPECT_EQ(MUSIC::IsCDDA(CFileItem(GetParam().path, GetParam().folder)), GetParam().result);
}

const auto cdda_tests = std::array{
    SimpleDefinition{"cdda://1", false, true},
    SimpleDefinition{"cdda://1/", true, true},
    SimpleDefinition{"cdda://1/", true, true},
    SimpleDefinition{"/home/foo/yo.cdda", false, false},
};

INSTANTIATE_TEST_SUITE_P(TestMusicFileItemClassify, CDDATest, testing::ValuesIn(cdda_tests));

class MusicDbTest : public testing::WithParamInterface<SimpleDefinition>, public testing::Test
{
};

TEST_P(MusicDbTest, IsMusicDb)
{
  EXPECT_EQ(MUSIC::IsMusicDb(CFileItem(GetParam().path, GetParam().folder)), GetParam().result);
}

const auto musicdb_tests = std::array{
    SimpleDefinition{"musicdb://1", false, true},
    SimpleDefinition{"musicdb://1/", true, true},
    SimpleDefinition{"/home/foo/musicdb/yo.mp3", false, false},
};

INSTANTIATE_TEST_SUITE_P(TestMusicFileItemClassify, MusicDbTest, testing::ValuesIn(musicdb_tests));
