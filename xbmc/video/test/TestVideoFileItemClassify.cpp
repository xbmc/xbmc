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
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoScanner.h"
#include "video/VideoInfoTag.h"
#include "video/VideoUtils.h"

#include <array>
#include <utility>

#include <gtest/gtest.h>

using namespace KODI;

struct StubDefinition
{
  StubDefinition(std::string path, bool res = true, std::string tagPath = "", bool isFolder = false)
    : path(std::move(path)),
      result(res),
      tagPath(std::move(tagPath)),
      isFolder(isFolder)
  {
  }

  std::string path;
  bool result;
  std::string tagPath;
  bool isFolder;
};

class DiscStubTest : public testing::WithParamInterface<StubDefinition>, public testing::Test
{
};

TEST_P(DiscStubTest, IsDiscStub)
{
  const StubDefinition& param = GetParam();

  CFileItem item(param.path, param.isFolder);
  if (!param.tagPath.empty())
  {
    if (param.isFolder)
      item.GetVideoInfoTag()->m_strPath = param.tagPath;
    else
      item.GetVideoInfoTag()->m_strFileNameAndPath = param.tagPath;
  }

  EXPECT_EQ(VIDEO::IsDiscStub(item), param.result);
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
  VideoClassifyTest(std::string path, bool res = true, std::string mime = "", int tag_type = 0)
    : path(std::move(path)),
      result(res),
      mime(std::move(mime)),
      tag_type(tag_type)
  {
  }

  std::string path;
  bool result;
  std::string mime;
  int tag_type;
};

class VideoTest : public testing::WithParamInterface<VideoClassifyTest>, public testing::Test
{
};

TEST_P(VideoTest, IsVideo)
{
  const VideoClassifyTest& param = GetParam();

  CFileItem item(param.path, false);
  if (!param.mime.empty())
    item.SetMimeType(param.mime);

  switch (param.tag_type)
  {
    case 1:
      item.GetVideoInfoTag()->m_strFileNameAndPath = param.path;
      break;
    case 2:
      item.GetGameInfoTag()->SetGameClient("some_client");
      break;
    case 3:
      item.GetMusicInfoTag()->SetPlayCount(1);
      break;
    case 4:
      item.GetPictureInfoTag()->SetResolution(1, 1);
      break;
    default:
      break;
  }

  EXPECT_EQ(VIDEO::IsVideo(item), param.result);
}

const auto video_tests = std::array{
    VideoClassifyTest{"/home/user/video.avi", true, "video/avi"},
    VideoClassifyTest{"/home/user/video.avi", true, "", 1},
    VideoClassifyTest{"/home/user/video.gam", false, "", 2},
    VideoClassifyTest{"/home/user/video.mus", false, "", 3},
    VideoClassifyTest{"/home/user/video.pic", false, "", 4},
    VideoClassifyTest{"pvr://recordings/tv/1", true},
    VideoClassifyTest{"pvr://123", false},
    VideoClassifyTest{"dvd://VIDEO_TS/video_ts.ifo", true},
    VideoClassifyTest{"dvd://1", true},
    VideoClassifyTest{"bluray://%2fhome%2fuser%2fdisc/", true},
    VideoClassifyTest{"bluray://udf%3a%2f%2f%252Fhome%252Fuser%252Fdisc.iso%2f/", true},
    VideoClassifyTest{"dvd://%2fhome%2fuser%2fdisc/", true},
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
      EXPECT_TRUE(VIDEO::IsVideo(CFileItem("test" + ext, false)));
    }
  }
}

TEST(TestVideoFileItemClassify, RecognizeNestedOpticalMediaLikeMovieScanner)
{
  // A show with several optical disc subfolders must detect actual disc media
  // at the show level, not recurse through BDMV/PLAYLIST as if it were TV shows.
  XFILE::CFile* tempFile{CXBMCTestUtils::Instance().CreateTempFile("tvdisc")};
  ASSERT_NE(tempFile, nullptr);
  const std::string root{CXBMCTestUtils::Instance().TempFileDirectory(tempFile)};
  const std::string discFolder{URIUtils::AddFileToFolder(root, "GAVAN D1")};
  const std::string dvdFolder{URIUtils::AddFileToFolder(discFolder, "VIDEO_TS")};
  ASSERT_TRUE(XFILE::CDirectory::Create(discFolder));
  ASSERT_TRUE(XFILE::CDirectory::Create(dvdFolder));

  const std::string dvdEntry{URIUtils::AddFileToFolder(dvdFolder, "VIDEO_TS.IFO")};
  {
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(dvdEntry));
    file.Close();
  }
  EXPECT_EQ(dvdEntry, VIDEO::UTILS::GetOpticalMediaPath(CFileItem(discFolder, true)));
  EXPECT_TRUE(VIDEO::IsInOpticalDiscStructure(dvdEntry));
  EXPECT_FALSE(VIDEO::IsInOpticalDiscStructure(discFolder));

#ifdef HAVE_LIBBLURAY
  const std::string blurayDisc{URIUtils::AddFileToFolder(root, "GAVAN D2")};
  const std::string bdmvFolder{URIUtils::AddFileToFolder(blurayDisc, "BDMV")};
  ASSERT_TRUE(XFILE::CDirectory::Create(blurayDisc));
  ASSERT_TRUE(XFILE::CDirectory::Create(bdmvFolder));
  const std::string blurayEntry{URIUtils::AddFileToFolder(bdmvFolder, "index.bdmv")};
  {
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(blurayEntry));
    file.Close();
  }
  EXPECT_EQ(blurayEntry, VIDEO::UTILS::GetOpticalMediaPath(CFileItem(blurayDisc, true)));
  EXPECT_TRUE(VIDEO::IsInOpticalDiscStructure(blurayEntry));
  XFILE::CDirectory::RemoveRecursive(blurayDisc);
#endif

  XFILE::CDirectory::RemoveRecursive(discFolder);
  CXBMCTestUtils::Instance().DeleteTempFile(tempFile);
}

class OpticalDiscScanner : public VIDEO::CVideoInfoScanner
{
public:
  using VIDEO::CVideoInfoScanner::IsOpticalDiscVolume;
};

TEST(TestVideoFileItemClassify, OpticalDiscVolumesWithReleaseTags)
{
  EXPECT_TRUE(OpticalDiscScanner::IsOpticalDiscVolume(
      "Lady.Oscar.Disc.1.MULTi.COMPLETE.FRA.BLURAY-LEON", "Lady Oscar"));
  EXPECT_TRUE(OpticalDiscScanner::IsOpticalDiscVolume("Show.Disc.12.1080p.BluRay", "Show"));
  EXPECT_TRUE(OpticalDiscScanner::IsOpticalDiscVolume("Show.DVD.2.REMASTERED", "Show"));
  EXPECT_TRUE(OpticalDiscScanner::IsOpticalDiscVolume("Show.D1.MULTi", "Show 1982"));
  EXPECT_TRUE(OpticalDiscScanner::IsOpticalDiscVolume("Disc 1 COMPLETE UHD", "Any show"));
  EXPECT_TRUE(OpticalDiscScanner::IsOpticalDiscVolume("Show Disc 1", "Show"));
}

TEST(TestVideoFileItemClassify, OpticalDiscVolumesRetainShowOwnership)
{
  EXPECT_FALSE(OpticalDiscScanner::IsOpticalDiscVolume("Show A.Disc.1.MULTi", "Show B"));
  EXPECT_FALSE(OpticalDiscScanner::IsOpticalDiscVolume("Show A.Disc.1.MULTi", "Collection"));
  EXPECT_FALSE(OpticalDiscScanner::IsOpticalDiscVolume("Show.Disc.1.Another.Title", "Show"));
  EXPECT_FALSE(OpticalDiscScanner::IsOpticalDiscVolume("Show.Disc.1MULTi", "Show"));
  EXPECT_FALSE(OpticalDiscScanner::IsOpticalDiscVolume("Show.Disc.MULTi", "Show"));
}

TEST(TestVideoFileItemClassify, IsInOpticalDiscStructure)
{
  // Full paths are important here: a previous recursive TV scan incorrectly
  // imported BDMV/PLAYLIST as a show and indexed its .mpls files as episodes.
  const std::array<std::pair<std::string, bool>, 12> paths{{
      {"smb://jedi/Series/Sentai/Gavan/GAVAN D1/BDMV/PLAYLIST/00001.mpls", true},
      {"smb://jedi/Series/Sentai/Gavan/GAVAN D1/BDMV/PLAYLIST/", true},
      {"smb://jedi/Series/Sentai/Gavan/GAVAN D1/bdmv/backup/playlist/", true},
      {"smb://jedi/Series/Sentai/Gavan/GAVAN D1/BDMV/", true},
      {"smb://jedi/Series/Sentai/Gavan/GAVAN D1/", false},
      {"smb://jedi/Series/Animation/Dragon Ball/Dragon Ball Z/Season 01/", false},
      {"/media/tv/show/VIDEO_TS/VTS_01_1.VOB", true},
      {"/media/tv/show/video_ts/", true},
      {"/media/tv/show/", false},
      {"/media/tv/BDMV-rips/show/", false},
      {"/media/tv/PLAYLIST/00001.mpls", false},
      {"/media/tv/BDMV Extras/show/", false},
  }};

  for (const auto& [path, expected] : paths)
    EXPECT_EQ(expected, VIDEO::IsInOpticalDiscStructure(path)) << path;
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

TEST(TestVideoFileItemClassify, IsVideoExtrasFolderName)
{
  EXPECT_TRUE(VIDEO::IsVideoExtrasFolderName("Extras"));
  EXPECT_TRUE(VIDEO::IsVideoExtrasFolderName("extras"));
  EXPECT_TRUE(VIDEO::IsVideoExtrasFolderName("Bonus Disc"));
  EXPECT_TRUE(VIDEO::IsVideoExtrasFolderName("Bonus Disk"));
  EXPECT_TRUE(VIDEO::IsVideoExtrasFolderName("Bonus Content"));
  EXPECT_TRUE(VIDEO::IsVideoExtrasFolderName("Bonus Features"));
  EXPECT_TRUE(VIDEO::IsVideoExtrasFolderName("Bonus Feature"));
  EXPECT_TRUE(VIDEO::IsVideoExtrasFolderName("bonus_discs"));
  EXPECT_TRUE(VIDEO::IsVideoExtrasFolderName("BONUSDISC"));
  EXPECT_FALSE(VIDEO::IsVideoExtrasFolderName("Extra"));
  EXPECT_FALSE(VIDEO::IsVideoExtrasFolderName("Extraordinary"));
  EXPECT_FALSE(VIDEO::IsVideoExtrasFolderName("Bonus"));
  EXPECT_FALSE(VIDEO::IsVideoExtrasFolderName("Featurettes"));
  EXPECT_FALSE(VIDEO::IsVideoExtrasFolderName("Disc 1"));
  EXPECT_FALSE(VIDEO::IsVideoExtrasFolderName(""));
}

TEST(TestVideoFileItemClassify, IsVideoExtrasFolder)
{
  EXPECT_TRUE(VIDEO::IsVideoExtrasFolder(CFileItem("/home/foo/Extras/", true)));
  EXPECT_TRUE(VIDEO::IsVideoExtrasFolder(CFileItem("/home/foo/extras/", true)));
  EXPECT_TRUE(VIDEO::IsVideoExtrasFolder(CFileItem("/home/foo/Bonus Disc/", true)));
  EXPECT_FALSE(VIDEO::IsVideoExtrasFolder(CFileItem("/home/foo/Extraordinary/", true)));
  EXPECT_FALSE(VIDEO::IsVideoExtrasFolder(CFileItem("/home/foo/Extras/abc.mkv", false)));
}
