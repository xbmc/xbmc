/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "playlists/PlayListFileItemClassify.h"
#include "utils/Variant.h"

#include <array>

#include <gtest/gtest.h>

using namespace KODI;

struct PlayListClassifyTest
{
  PlayListClassifyTest(const std::string& path, bool res, const std::string& mime = "")
    : item(path, false), result(res)
  {
    if (!mime.empty())
      item.SetMimeType(mime);
  }

  CFileItem item;
  bool result;
};

class PlayListTest : public testing::WithParamInterface<PlayListClassifyTest>, public testing::Test
{
};

TEST_P(PlayListTest, IsPlayList)
{
  EXPECT_EQ(PLAYLIST::IsPlayList(GetParam().item), GetParam().result);
}

const auto playlist_tests = std::array{
    PlayListClassifyTest{"/home/user/video.avi", false},
    PlayListClassifyTest{"/home/user/video.avi", false, "video/avi"},
    PlayListClassifyTest{"https://some.where/foo.m3u8", false},
    PlayListClassifyTest{"https://some.where/something", true, "audio/x-pn-realaudio"},
    PlayListClassifyTest{"https://some.where/something", true, "playlist"},
    PlayListClassifyTest{"https://some.where/something", true, "audio/x-mpegurl"},
    PlayListClassifyTest{"/home/user/video.m3u", true},
    PlayListClassifyTest{"/home/user/video.m3u8", true},
    PlayListClassifyTest{"/home/user/video.b4s", true},
    PlayListClassifyTest{"/home/user/video.pls", true},
    PlayListClassifyTest{"/home/user/video.strm", true},
    PlayListClassifyTest{"/home/user/video.wpl", true},
    PlayListClassifyTest{"/home/user/video.asx", true},
    PlayListClassifyTest{"/home/user/video.ram", true},
    PlayListClassifyTest{"/home/user/video.url", true},
    PlayListClassifyTest{"/home/user/video.pxml", true},
    PlayListClassifyTest{"/home/user/video.xspf", true},
};

INSTANTIATE_TEST_SUITE_P(TestPlayListFileItemClassify,
                         PlayListTest,
                         testing::ValuesIn(playlist_tests));

TEST(TestPlayListFileItemClassify, IsSmartPlayList)
{
  CFileItem item("/some/where.avi", false);
  EXPECT_FALSE(PLAYLIST::IsSmartPlayList(item));
  item.SetProperty("library.smartplaylist", true);
  EXPECT_TRUE(PLAYLIST::IsSmartPlayList(item));

  CFileItem item2("/some/where.xsp", false);
  EXPECT_TRUE(PLAYLIST::IsSmartPlayList(item2));
  CFileItem item3("/some/where.xsp", true);
  EXPECT_TRUE(PLAYLIST::IsSmartPlayList(item3));
}
