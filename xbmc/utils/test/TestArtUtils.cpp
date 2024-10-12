/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "platform/Filesystem.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/SettingsManager.h"
#include "utils/ArtUtils.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"

#include <array>
#include <fstream>
#include <random>

#include <fmt/format.h>
#include <gtest/gtest.h>

using namespace KODI;

namespace
{

std::string unique_path(const std::string& input)
{
  std::random_device rd;
  std::mt19937 gen(rd());
  auto randchar = [&gen]()
  {
    const std::string set = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<> select(0, set.size() - 1);
    return set[select(gen)];
  };

  std::string ret;
  ret.reserve(input.size());
  std::transform(input.begin(), input.end(), std::back_inserter(ret),
                 [&randchar](const char c) { return (c == '%') ? randchar() : c; });

  return ret;
}

class AdvancedSettingsResetBase : public testing::Test
{
public:
  AdvancedSettingsResetBase();
};

AdvancedSettingsResetBase::AdvancedSettingsResetBase()
{
  // Force all advanced settings to be reset to defaults
  const auto settings = CServiceBroker::GetSettingsComponent();
  CSettingsManager* settingsMgr = settings->GetSettings()->GetSettingsManager();
  settings->GetAdvancedSettings()->Uninitialize(*settingsMgr);
  settings->GetAdvancedSettings()->Initialize(*settingsMgr);
}

struct ArtFilenameTest
{
  std::string path;
  std::string result;
  bool isFolder = false;
  bool result_folder = false;
  bool force_use_folder = false;
};

class GetLocalArtBaseFilenameTest : public testing::WithParamInterface<ArtFilenameTest>,
                                    public testing::Test
{
};

const auto local_art_filename_tests = std::array{
    ArtFilenameTest{"/home/user/foo.avi", "/home/user/foo.avi"},
    ArtFilenameTest{"stack:///home/user/foo-cd1.avi , /home/user/foo-cd2.avi",
                    "/home/user/foo.avi"},
    ArtFilenameTest{"zip://%2fhome%2fuser%2fbar.zip/foo.avi", "/home/user/foo.avi"},
    ArtFilenameTest{"multipath://%2fhome%2fuser%2fbar%2f/%2fhome%2fuser%2ffoo%2f",
                    "/home/user/bar/", true, true},
    ArtFilenameTest{"/home/user/VIDEO_TS/VIDEO_TS.IFO", "/home/user/", false, true},
    ArtFilenameTest{"/home/user/BDMV/index.bdmv", "/home/user/", false, true},
    ArtFilenameTest{"/home/user/foo.avi", "/home/user/", false, true, true},
};

struct FanartTest
{
  std::string path;
  std::string result;
};

class GetLocalFanartTest : public testing::WithParamInterface<FanartTest>, public testing::Test
{
};

const auto local_fanart_tests = std::array{
    FanartTest{"stack://#DIRECTORY#foo-cd1.avi , #DIRECTORY#foo-cd2.avi", "foo-fanart.jpg"},
    FanartTest{"stack://#DIRECTORY#foo-cd1.avi , #DIRECTORY#foo-cd2.avi", "foo-cd1-fanart.jpg"},
    FanartTest{"zip://#URLENCODED_DIRECTORY#bar.zip/foo.avi", "foo-fanart.jpg"},
    FanartTest{"ftp://some.where/foo.avi", ""},
    FanartTest{"https://some.where/foo.avi", ""},
    FanartTest{"upnp://some.where/123", ""},
    FanartTest{"bluray://1", ""},
    FanartTest{"/home/user/1.pvr", ""},
    FanartTest{"plugin://random.video/1", ""},
    FanartTest{"addons://plugins/video/1", ""},
    FanartTest{"dvd://1", ""},
    FanartTest{"", ""},
    FanartTest{"foo.avi", ""},
    FanartTest{"foo.avi", "foo-fanart.jpg"},
    FanartTest{"videodb://movies/1", ""},
    FanartTest{"videodb://movies/1", "foo-fanart.jpg"},
};

struct IconTest
{
  std::string path;
  std::string icon;
  std::string overlay{};
  bool isFolder = false;
  bool valid = true;
  bool no_overlay = false;
};

class FillInDefaultIconTest : public testing::WithParamInterface<IconTest>, public testing::Test
{
};

const auto icon_tests = std::array{
    IconTest{"pvr://guide", "", "", false, false},
    IconTest{"/home/user/test.pvr", "DefaultTVShows.png"},
    IconTest{"/home/user/test.zip", "DefaultFile.png"},
    IconTest{"/home/user/test.mp3", "DefaultAudio.png"},
    IconTest{"/home/user/test.avi", "DefaultVideo.png"},
    IconTest{"/home/user/test.jpg", "DefaultPicture.png"},
    IconTest{"/home/user/test.m3u", "DefaultPlaylist.png"},
    IconTest{"/home/user/test.xsp", "DefaultPlaylist.png"},
    IconTest{"/home/user/test.py", "DefaultScript.png"},
    IconTest{"favourites://1", "DefaultFavourites.png"},
    IconTest{"/home/user/test.fil", "DefaultFile.png"},
    IconTest{"/home/user/test.m3u", "DefaultPlaylist.png", "", true},
    IconTest{"/home/user/test.xsp", "DefaultPlaylist.png", "", true},
    IconTest{"..", "DefaultFolderBack.png", "", true},
    IconTest{"/home/user/test/", "DefaultFolder.png", "", true},
    IconTest{"zip://%2fhome%2fuser%2fbar.zip/foo.avi", "DefaultVideo.png", "OverlayZIP.png"},
    IconTest{"zip://%2fhome%2fuser%2fbar.zip/foo.avi", "DefaultVideo.png", "", false, true, true},
    IconTest{"rar://%2fhome%2fuser%2fbar.rar/foo.avi", "DefaultVideo.png", "OverlayRAR.png"},
    IconTest{"rar://%2fhome%2fuser%2fbar.rar/foo.avi", "DefaultVideo.png", "", false, true, true},
};

struct TbnTest
{
  std::string path;
  std::string result;
  bool isFolder = false;
};

class GetTbnTest : public testing::WithParamInterface<TbnTest>, public testing::Test
{
};

struct FolderTest
{
  std::string path;
  std::string thumb;
  std::string result;
};

const auto folder_thumb_tests = std::array{
    FolderTest{"c:\\dir\\", "art.jpg", "c:\\dir\\art.jpg"},
    FolderTest{"/home/user/", "folder.jpg", "/home/user/folder.jpg"},
    FolderTest{"plugin://plugin.video.foo/", "folder.jpg", ""},
    FolderTest{"stack:///home/user/bar/foo-cd1.avi , /home/user/bar/foo-cd2.avi", "folder.jpg",
               "/home/user/bar/folder.jpg"},
    FolderTest{"stack:///home/user/cd1/foo-cd1.avi , /home/user/cd2/foo-cd2.avi", "artist.jpg",
               "/home/user/artist.jpg"},
    FolderTest{"zip://%2fhome%2fuser%2fbar.zip/foo.avi", "cover.png", "/home/user/cover.png"},
    FolderTest{"multipath://%2fhome%2fuser%2fbar%2f/%2fhome%2fuser%2ffoo%2f", "folder.jpg",
               "/home/user/bar/folder.jpg"},
};

class FolderThumbTest : public testing::WithParamInterface<FolderTest>, public testing::Test
{
};

struct LocalArtTest
{
  std::string file;
  std::string art;
  bool use_folder;
  std::string base;
};

const auto local_art_tests = std::array{
    LocalArtTest{"c:\\dir\\filename.avi", "art.jpg", false, "c:\\dir\\filename-art.jpg"},
    LocalArtTest{"c:\\dir\\filename.avi", "art.jpg", true, "c:\\dir\\art.jpg"},
    LocalArtTest{"/dir/filename.avi", "art.jpg", false, "/dir/filename-art.jpg"},
    LocalArtTest{"/dir/filename.avi", "art.jpg", true, "/dir/art.jpg"},
    LocalArtTest{"smb://somepath/file.avi", "art.jpg", false, "smb://somepath/file-art.jpg"},
    LocalArtTest{"smb://somepath/file.avi", "art.jpg", true, "smb://somepath/art.jpg"},
    LocalArtTest{"stack:///path/to/movie-cd1.avi , /path/to/movie-cd2.avi", "art.jpg", false,
                 "/path/to/movie-art.jpg"},
    LocalArtTest{"stack:///path/to/movie-cd1.avi , /path/to/movie-cd2.avi", "art.jpg", true,
                 "/path/to/art.jpg"},
    LocalArtTest{
        "stack:///path/to/movie_name/cd1/some_file1.avi , /path/to/movie_name/cd2/some_file2.avi",
        "art.jpg", true, "/path/to/movie_name/art.jpg"},
    LocalArtTest{"/home/user/TV Shows/Dexter/S1/1x01.avi", "art.jpg", false,
                 "/home/user/TV Shows/Dexter/S1/1x01-art.jpg"},
    LocalArtTest{"/home/user/TV Shows/Dexter/S1/1x01.avi", "art.jpg", true,
                 "/home/user/TV Shows/Dexter/S1/art.jpg"},
    LocalArtTest{"zip://g%3a%5cmultimedia%5cmovies%5cSphere%2ezip/Sphere.avi", "art.jpg", false,
                 "g:\\multimedia\\movies\\Sphere-art.jpg"},
    LocalArtTest{"zip://g%3a%5cmultimedia%5cmovies%5cSphere%2ezip/Sphere.avi", "art.jpg", true,
                 "g:\\multimedia\\movies\\art.jpg"},
    LocalArtTest{"/home/user/movies/movie_name/video_ts/VIDEO_TS.IFO", "art.jpg", false,
                 "/home/user/movies/movie_name/art.jpg"},
    LocalArtTest{"/home/user/movies/movie_name/video_ts/VIDEO_TS.IFO", "art.jpg", true,
                 "/home/user/movies/movie_name/art.jpg"},
    LocalArtTest{"/home/user/movies/movie_name/BDMV/index.bdmv", "art.jpg", false,
                 "/home/user/movies/movie_name/art.jpg"},
    LocalArtTest{"/home/user/movies/movie_name/BDMV/index.bdmv", "art.jpg", true,
                 "/home/user/movies/movie_name/art.jpg"},
    LocalArtTest{"c:\\dir\\filename.avi", "", false, "c:\\dir\\filename.tbn"},
    LocalArtTest{"/dir/filename.avi", "", false, "/dir/filename.tbn"},
    LocalArtTest{"smb://somepath/file.avi", "", false, "smb://somepath/file.tbn"},
    LocalArtTest{"/home/user/TV Shows/Dexter/S1/1x01.avi", "", false,
                 "/home/user/TV Shows/Dexter/S1/1x01.tbn"},
    LocalArtTest{"zip://g%3a%5cmultimedia%5cmovies%5cSphere%2ezip/Sphere.avi", "", false,
                 "g:\\multimedia\\movies\\Sphere.tbn"},
};

class TestLocalArt : public AdvancedSettingsResetBase,
                     public testing::WithParamInterface<LocalArtTest>
{
};

} // namespace

TEST_P(FillInDefaultIconTest, FillInDefaultIcon)
{
  CFileItem item(GetParam().path, GetParam().isFolder);
  if (!GetParam().valid)
    item.SetArt("icon", "InvalidImage.png");
  item.SetLabel(GetParam().path);
  if (GetParam().no_overlay)
    item.SetProperty("icon_never_overlay", true);
  ART::FillInDefaultIcon(item);
  EXPECT_EQ(item.GetArt("icon"), GetParam().valid ? GetParam().icon : "InvalidImage.png");
  EXPECT_EQ(item.GetOverlayImage(), GetParam().overlay);
}

INSTANTIATE_TEST_SUITE_P(TestArtUtils, FillInDefaultIconTest, testing::ValuesIn(icon_tests));

TEST_P(FolderThumbTest, GetFolderThumb)
{
  CFileItem item(GetParam().path, true);
  const std::string thumb = ART::GetFolderThumb(item, GetParam().thumb);
  EXPECT_EQ(thumb, GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(TestArtUtils, FolderThumbTest, testing::ValuesIn(folder_thumb_tests));

TEST_P(TestLocalArt, GetLocalArt)
{
  CFileItem item;
  item.SetPath(GetParam().file);
  std::string path = CURL(ART::GetLocalArt(item, GetParam().art, GetParam().use_folder)).Get();
  std::string compare = CURL(GetParam().base).Get();
  EXPECT_EQ(compare, path);
}

INSTANTIATE_TEST_SUITE_P(TestArtUtils, TestLocalArt, testing::ValuesIn(local_art_tests));

TEST_P(GetLocalArtBaseFilenameTest, GetLocalArtBaseFilename)
{
  CFileItem item(GetParam().path, GetParam().isFolder);
  bool useFolder = GetParam().force_use_folder ? true : GetParam().isFolder;
  const std::string res = ART::GetLocalArtBaseFilename(item, useFolder);
  EXPECT_EQ(res, GetParam().result);
  EXPECT_EQ(useFolder, GetParam().result_folder);
}

INSTANTIATE_TEST_SUITE_P(TestArtUtils,
                         GetLocalArtBaseFilenameTest,
                         testing::ValuesIn(local_art_filename_tests));

TEST_P(GetLocalFanartTest, GetLocalFanart)
{
  std::string path, file_path, uniq;
  if (GetParam().result.empty())
    file_path = GetParam().path;
  else
  {
    std::error_code ec;
    auto tmpdir = KODI::PLATFORM::FILESYSTEM::temp_directory_path(ec);
    ASSERT_TRUE(!ec);
    uniq = unique_path("FanartTest%%%%%");
    path = URIUtils::AddFileToFolder(tmpdir, uniq);
    URIUtils::AddSlashAtEnd(path);
    XFILE::CDirectory::Create(path);
    std::ofstream of(URIUtils::AddFileToFolder(path, GetParam().result), std::ios::out);
    if (GetParam().path.find("#DIRECTORY#") != std::string::npos)
    {
      file_path = GetParam().path;
      StringUtils::Replace(file_path, "#DIRECTORY#", path);
    }
    else if (GetParam().path.find("#URLENCODED_DIRECTORY#") != std::string::npos)
    {
      file_path = GetParam().path;
      StringUtils::Replace(file_path, "#URLENCODED_DIRECTORY#", CURL::Encode(path));
    }
    else if (GetParam().path.starts_with("videodb://"))
    {
      file_path = GetParam().path;
    }
    else
      file_path =
          URIUtils::AddFileToFolder(URIUtils::AddFileToFolder(tmpdir, uniq), GetParam().path);
  }

  CFileItem item(file_path, false);
  if (GetParam().path.starts_with("videodb://") && !GetParam().result.empty())
  {
    item.GetVideoInfoTag()->m_strFileNameAndPath = URIUtils::AddFileToFolder(path, "foo.avi");
  }
  const std::string res = ART::GetLocalFanart(item);

  EXPECT_EQ(URIUtils::GetFileName(res), GetParam().result);

  if (!GetParam().result.empty())
    XFILE::CDirectory::RemoveRecursive(path);
}

INSTANTIATE_TEST_SUITE_P(TestArtUtils, GetLocalFanartTest, testing::ValuesIn(local_fanart_tests));

TEST_P(GetTbnTest, TbnTest)
{
  EXPECT_EQ(ART::GetTBNFile(CFileItem(GetParam().path, GetParam().isFolder)), GetParam().result);
}

const auto tbn_tests = std::array{
    TbnTest{"/home/user/video.avi", "/home/user/video.tbn"},
    TbnTest{"/home/user/video/", "/home/user/video.tbn", true},
    TbnTest{"/home/user/bar.xbt", "/home/user/bar.tbn", true},
    TbnTest{"zip://%2fhome%2fuser%2fbar.zip/foo.avi", "/home/user/foo.tbn"},
    TbnTest{"stack:///home/user/foo-cd1.avi , /home/user/foo-cd2.avi", "/home/user/foo.tbn"}};

INSTANTIATE_TEST_SUITE_P(TestArtUtils, GetTbnTest, testing::ValuesIn(tbn_tests));

TEST(TestArtUtils, GetTbnStack)
{
  std::error_code ec;
  auto path = KODI::PLATFORM::FILESYSTEM::temp_directory_path(ec);
  ASSERT_TRUE(!ec);
  const auto file_path = URIUtils::AddFileToFolder(path, "foo-cd1.tbn");
  {
    std::ofstream of(file_path, std::ios::out);
  }
  const std::string stackPath =
      fmt::format("stack://{} , {}", URIUtils::AddFileToFolder(path, "foo-cd1.avi"),
                  URIUtils::AddFileToFolder(path, "foo-cd2.avi"));
  CFileItem item(stackPath, false);
  EXPECT_EQ(ART::GetTBNFile(item), file_path);
  CFileUtils::DeleteItem(file_path);
}
