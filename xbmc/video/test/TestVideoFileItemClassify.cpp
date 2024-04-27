/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "games/tags/GameInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "pictures/PictureInfoTag.h"
#include "test/TestUtils.h"
#include "utils/FileExtensionProvider.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

#include <array>

#include <gtest/gtest.h>

using namespace KODI;

struct StubDefinition
{
  StubDefinition(const std::string& path,
                 bool res = true,
                 const std::string& tagPath = "",
                 bool isFolder = false)
    : item(path, isFolder), result(res)
  {
    if (!tagPath.empty())
    {
      if (isFolder)
        item.GetVideoInfoTag()->m_strPath = tagPath;
      else
        item.GetVideoInfoTag()->m_strFileNameAndPath = tagPath;
    }
  }

  CFileItem item;
  bool result;
};

class DiscStubTest : public testing::WithParamInterface<StubDefinition>, public testing::Test
{
};

TEST_P(DiscStubTest, IsDiscStub)
{
  EXPECT_EQ(VIDEO::IsDiscStub(GetParam().item), GetParam().result);
}

const auto discstub_tests = std::array{
    StubDefinition{"/home/user/test.disc"},
    StubDefinition{"videodb://foo/bar", true, "/home/user/test.disc"},
    StubDefinition{"videodb://foo/bar", false, "/home/user/test.disc", true},
    StubDefinition{"/home/user/test.avi", false},
};

INSTANTIATE_TEST_SUITE_P(TestVideoFileItemClassify,
                         DiscStubTest,
                         testing::ValuesIn(discstub_tests));

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
    {
      EXPECT_TRUE(VIDEO::IsVideo(CFileItem(ext, false)));
    }
  }
}

TEST(TestVideoFileItemClassify, IsBDFile)
{
  EXPECT_TRUE(VIDEO::IsBDFile(CFileItem("/home/foo/index.BDMV", false)));
  EXPECT_TRUE(VIDEO::IsBDFile(CFileItem("smb://foo/bar/index.bdm", false)));
  EXPECT_TRUE(VIDEO::IsBDFile(CFileItem("ftp://foo:bar@foobar.com/movieobject.BDMV", false)));
  EXPECT_TRUE(VIDEO::IsBDFile(CFileItem("https://foobar.com/movieobj.bdm", false)));
  EXPECT_FALSE(VIDEO::IsBDFile(CFileItem("https://foobar.com/movieobject.not", false)));
}

TEST(TestVideoFileItemClassify, IsDVDFile)
{
  EXPECT_TRUE(VIDEO::IsDVDFile(CFileItem("/home/foo/video_ts.vob", false), true, false));
  EXPECT_TRUE(VIDEO::IsDVDFile(CFileItem("/home/foo/video_ts.VOB", false), true, true));
  EXPECT_FALSE(VIDEO::IsDVDFile(CFileItem("/home/foo/video_TS.vob", false), false, false));
  EXPECT_FALSE(VIDEO::IsDVDFile(CFileItem("/home/foo/video_ts.VOb", false), false, true));

  EXPECT_TRUE(VIDEO::IsDVDFile(CFileItem("/home/foo/vts_yo_0.vob", false), true, false));
  EXPECT_TRUE(VIDEO::IsDVDFile(CFileItem("/home/foo/vts_0_ifo.vob", false), true, true));
  EXPECT_FALSE(VIDEO::IsDVDFile(CFileItem("/home/foo/VTS_123456_0.vob", false), false, false));
  EXPECT_FALSE(VIDEO::IsDVDFile(CFileItem("/home/foo/VTS_qwerty_0.VOB", false), false, true));

  EXPECT_TRUE(VIDEO::IsDVDFile(CFileItem("/home/foo/video_ts.IFO", false), false, true));
  EXPECT_TRUE(VIDEO::IsDVDFile(CFileItem("/home/foo/video_ts.IFO", false), true, true));
  EXPECT_FALSE(VIDEO::IsDVDFile(CFileItem("/home/foo/video_ts.IFO", false), false, false));
  EXPECT_FALSE(VIDEO::IsDVDFile(CFileItem("/home/foo/video_ts.IFO", false), true, false));

  EXPECT_TRUE(VIDEO::IsDVDFile(CFileItem("/home/foo/vts_ab_0.ifo", false), false, true));
  EXPECT_TRUE(VIDEO::IsDVDFile(CFileItem("/home/foo/vts_ab_0.ifo", false), false, true));
  EXPECT_FALSE(VIDEO::IsDVDFile(CFileItem("/home/foo/VTS_ab_0.ifo", false), false, false));
  EXPECT_FALSE(VIDEO::IsDVDFile(CFileItem("/home/foo/VTS_ab_0.ifo", false), false, false));
}

TEST(TestVideoFileItemClassify, IsProtectedBlurayDisc)
{
  const auto temp_file = CXBMCTestUtils::Instance().CreateTempFile("bluraytest");
  const std::string dir = CXBMCTestUtils::Instance().TempFileDirectory(temp_file);
  CFileUtils::DeleteItem(URIUtils::AddFileToFolder(dir, "AACS", "Unit_Key_RO.inf"));
  EXPECT_FALSE(VIDEO::IsProtectedBlurayDisc(CFileItem(dir, true)));
  XFILE::CDirectory::Create(URIUtils::AddFileToFolder(dir, "AACS"));
  XFILE::CFile inf_file;
  inf_file.OpenForWrite(URIUtils::AddFileToFolder(dir, "AACS", "Unit_Key_RO.inf"));
  inf_file.Close();
  EXPECT_TRUE(VIDEO::IsProtectedBlurayDisc(CFileItem(dir, true)));
  CFileUtils::DeleteItem(URIUtils::AddFileToFolder(dir, "AACS", "Unit_Key_RO.inf"));
  CXBMCTestUtils::Instance().DeleteTempFile(temp_file);
}

TEST(TestVideoFileItemClassify, IsSubtitle)
{
  const auto& exts = CServiceBroker::GetFileExtensionProvider().GetSubtitleExtensions();
  for (const auto& ext : StringUtils::Split(exts, "|"))
  {
    if (!ext.empty())
    {
      EXPECT_TRUE(VIDEO::IsSubtitle(CFileItem("random" + ext, false)));
    }
  }

  EXPECT_FALSE(VIDEO::IsSubtitle(CFileItem("random.notasub", false)));
}

TEST(TestVideoFileItemClassify, IsVideoAssetsFile)
{
  EXPECT_TRUE(VIDEO::IsVideoAssetFile(CFileItem("videodb://foo/bar?videoversionid=1", false)));
  EXPECT_FALSE(VIDEO::IsVideoAssetFile(CFileItem("videodb://foo/bar?videoversionid=1", true)));
  EXPECT_FALSE(VIDEO::IsVideoAssetFile(CFileItem("videodb://foo/bar", false)));
}

TEST(TestVideoFileItemClassify, IsVideoDb)
{
  EXPECT_TRUE(VIDEO::IsVideoDb(CFileItem("videodb://1/2/3", false)));
  EXPECT_TRUE(VIDEO::IsVideoDb(CFileItem("videodb://1/2/", true)));
  EXPECT_FALSE(VIDEO::IsVideoDb(CFileItem("/videodb/home/foo/Extraordinary/", true)));
}

TEST(TestVideoFileItemClassify, IsVideoExtrasFolder)
{
  EXPECT_TRUE(VIDEO::IsVideoExtrasFolder(CFileItem("/home/foo/Extras/", true)));
  EXPECT_TRUE(VIDEO::IsVideoExtrasFolder(CFileItem("/home/foo/extras/", true)));
  EXPECT_FALSE(VIDEO::IsVideoExtrasFolder(CFileItem("/home/foo/Extraordinary/", true)));
  EXPECT_FALSE(VIDEO::IsVideoExtrasFolder(CFileItem("/home/foo/Extras/abc.mkv", false)));
}
