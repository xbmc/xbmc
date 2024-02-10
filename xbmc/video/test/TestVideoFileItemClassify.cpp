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
#include "music/tags/MusicInfoTag.h"
#include "pictures/PictureInfoTag.h"
#include "utils/FileExtensionProvider.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

#include <array>

#include <gtest/gtest.h>

using namespace KODI;

struct VideoClassifyTest
{
  VideoClassifyTest(const std::string& path,
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
        ;
        break;
      default:
        break;
    }
  }

  CFileItem item;
  bool result;
};

class VideoTest : public testing::WithParamInterface<VideoClassifyTest>, public testing::Test
{
};

TEST_P(VideoTest, IsVideo)
{
  EXPECT_EQ(VIDEO::IsVideo(GetParam().item), GetParam().result);
}

const auto video_tests = std::array{
    VideoClassifyTest{"/home/user/video.avi", true, "video/avi"},
    VideoClassifyTest{"/home/user/video.avi", true, "", 1},
    VideoClassifyTest{"/home/user/video.avi", false, "", 2},
    VideoClassifyTest{"/home/user/video.avi", false, "", 3},
    VideoClassifyTest{"/home/user/video.avi", false, "", 4},
    VideoClassifyTest{"pvr://recordings/tv/1", true},
    VideoClassifyTest{"pvr://123", false},
    VideoClassifyTest{"dvd://VIDEO_TS/video_ts.ifo", true},
    VideoClassifyTest{"dvd://1", true},
    VideoClassifyTest{"/home/user/video.not", true, "application/ogg"},
    VideoClassifyTest{"/home/user/video.not", true, "application/mp4"},
    VideoClassifyTest{"/home/user/video.not", true, "application/mxf"},
};

INSTANTIATE_TEST_SUITE_P(TestVideoFileItemClassify, VideoTest, testing::ValuesIn(video_tests));

TEST(TestVideoFileItemClassify, VideoExtensions)
{
  const auto& exts = CServiceBroker::GetFileExtensionProvider().GetVideoExtensions();
  for (const auto& ext : StringUtils::Split(exts, "|"))
  {
    if (!ext.empty())
      EXPECT_TRUE(VIDEO::IsVideo(CFileItem(ext, false)));
  }
}
