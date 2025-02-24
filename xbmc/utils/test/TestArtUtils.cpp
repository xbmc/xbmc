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
  ret.resize(input.size());
  std::ranges::transform(input, ret.begin(),
                         [&](const char c) { return c == '%' ? randchar() : c; });

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
} // namespace

// ART::GetLocalArtBaseFilename() tests

struct ArtFilenameTest
{
  std::string path;
  std::string result;
  bool isFolder{false};
  bool result_folder{false};
  bool force_use_folder{false};
  ART::AdditionalIdentifiers additionalIdentifiers{ART::AdditionalIdentifiers::NONE};
  int playlist{-1};
  int season{-1};
  int episode{-1};
};

class GetLocalArtBaseFilenameTest : public testing::WithParamInterface<ArtFilenameTest>,
                                    public testing::Test
{
};

const ArtFilenameTest local_art_filename_tests[] = {
    // Linux path tests
    {"/home/user/movies/movie/movie.avi", "/home/user/movies/movie/movie.avi"},
    {"/home/user/movies/movie/disc 1/movie.avi", "/home/user/movies/movie/disc 1/movie.avi"},
    {"/home/user/movies/movie/video_ts/video_ts.ifo", "/home/user/movies/movie/", false, true},
    {"/home/user/movies/movie/disc 1/video_ts/video_ts.ifo", "/home/user/movies/movie/disc 1/",
     false, true},
    {"/home/user/movies/movie/BDMV/index.bdmv", "/home/user/movies/movie/", false, true},
    {"/home/user/movies/movie/disc 1/BDMV/index.bdmv", "/home/user/movies/movie/disc 1/", false,
     true},
    {"stack:///path/to/movie_name/movie-part-1.avi , /path/to/movie_name/movie-part-2.avi",
     "/path/to/movie_name/movie.avi"},
    {"stack:///path/to/movie/movie part 1/file.avi , /path/to/movie/movie part 2/file.avi",
     "/path/to/movie/movie.avi"},
    // URL path tests with smb://
    {"smb://home/user/movies/movie/movie.avi", "smb://home/user/movies/movie/movie.avi"},
    {"smb://home/user/movies/movie/disc 1/movie.avi",
     "smb://home/user/movies/movie/disc 1/movie.avi"},
    {"smb://home/user/movies/movie/video_ts/video_ts.ifo", "smb://home/user/movies/movie/", false,
     true},
    {"smb://home/user/movies/movie/disc 1/video_ts/video_ts.ifo",
     "smb://home/user/movies/movie/disc 1/", false, true},
    {"smb://home/user/movies/movie/BDMV/index.bdmv", "smb://home/user/movies/movie/", false, true},
    {"smb://home/user/movies/movie/disc 1/BDMV/index.bdmv", "smb://home/user/movies/movie/disc 1/",
     false, true},
    {"stack://smb://path/to/movie_name/movie-part-1.avi , "
     "smb://path/to/movie_name/movie-part-2.avi",
     "smb://path/to/movie_name/movie.avi"},
    {"stack://smb://path/to/movie/movie part 1/file.avi , smb://path/to/movie/movie part "
     "2/file.avi",
     "smb://path/to/movie/movie.avi"},
    // DOS path tests
    {"D:\\Movies\\Movie\\movie.avi", "D:\\Movies\\Movie\\movie.avi"},
    {"D:\\Movies\\Movie\\disc 1\\movie.avi", "D:\\Movies\\Movie\\disc 1\\movie.avi"},
    {"D:\\Movies\\Movie\\video_ts\\video_ts.ifo", "D:\\Movies\\Movie\\", false, true},
    {"D:\\Movies\\Movie\\disc 1\\video_ts\\video_ts.ifo", "D:\\Movies\\Movie\\disc 1\\", false,
     true},
    {"D:\\Movies\\Movie\\BDMV\\index.bdmv", "D:\\Movies\\Movie\\", false, true},
    {"D:\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", "D:\\Movies\\Movie\\disc 1\\", false, true},
    {"stack://D:\\movie_name\\movie-part-1.avi , "
     "D:\\movie_name\\movie-part-2.avi",
     "D:\\movie_name\\movie.avi"},
    {"stack://D:\\movie\\movie part 1\\file.avi , D:\\movie\\movie part 2\\file.avi",
     "D:\\movie\\movie.avi"},
    // Windows server path tests
    {"\\\\Server\\Movies\\Movie\\movie.avi", "\\\\Server\\Movies\\Movie\\movie.avi"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\movie.avi",
     "\\\\Server\\Movies\\Movie\\disc 1\\movie.avi"},
    {"\\\\Server\\Movies\\Movie\\video_ts\\video_ts.ifo", "\\\\Server\\Movies\\Movie\\", false,
     true},
    {"\\\\Server\\Movies\\Movie\\disc 1\\video_ts\\video_ts.ifo",
     "\\\\Server\\Movies\\Movie\\disc 1\\", false, true},
    {"\\\\Server\\Movies\\Movie\\BDMV\\index.bdmv", "\\\\Server\\Movies\\Movie\\", false, true},
    {"\\\\Server\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", "\\\\Server\\Movies\\Movie\\disc 1\\",
     false, true},
    {"stack://\\\\Server\\Movies\\Movie\\movie-part-1.avi , "
     "\\\\Server\\Movies\\Movie\\movie-part-2.avi",
     "\\\\Server\\Movies\\Movie\\movie.avi"},
    {"stack://\\\\Server\\Movies\\Movie\\movie part 1\\file.avi , \\\\Server\\Movies\\Movie\\movie "
     "part 2\\file.avi",
     "\\\\Server\\Movies\\Movie\\movie.avi"},
    // Embedded URL path tests with smb://
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "smb://somepath/movie.iso"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "smb://somepath/disc 1/movie.iso"},
    {"bluray://smb%3a%2f%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls", "smb://somepath/", false, true},
    {"bluray://smb%3a%2f%2fsomepath%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls",
     "smb://somepath/disc 1/", false, true},
    {"zip://smb%3a%2f%2fsomepath%2fmovie.zip/film.mkv", "smb://somepath/movie.avi"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie.zip/BDMV/index.bdmv", "smb://somepath/movie.avi"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie.zip/BDMV/index.bdmv",
     "smb://somepath/movie/disc 1/movie.avi"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie.rar/film.mkv", "smb://somepath/movie.avi"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie.rar/BDMV/index.bdmv", "smb://somepath/movie.avi"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie.rar/BDMV/index.bdmv",
     "smb://somepath/movie/disc 1/movie.avi"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie.tar.gz/film.mkv", "smb://somepath/movie.avi"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", "smb://somepath/movie.avi"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie.tar.gz/BDMV/index.bdmv",
     "smb://somepath/movie/disc 1/movie.avi"},
    {"multipath://smb%3a%2f%2fhome%2fuser%2fbar%2f/smb%3a%2f%2fhome%2fuser%2ffoo%2f",
     "smb://home/user/bar/"},
    // Embedded linux path tests
    {"bluray://udf%3a%2f%2f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "/somepath/movie.iso"},
    {"bluray://udf%3a%2f%2f%252fsomepath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "/somepath/disc 1/movie.iso"},
    {"bluray://%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls", "/somepath/", false, true},
    {"bluray://%2fsomepath%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls", "/somepath/disc 1/", false,
     true},
    {"zip://%2fsomepath%2fmovie.zip/film.mkv", "/somepath/movie.avi"},
    {"zip://%2fsomepath%2fmovie.zip/BDMV/index.bdmv", "/somepath/movie.avi"},
    {"zip://%2fsomepath%2fmovie%2fdisc%201%2fmovie.zip/BDMV/index.bdmv",
     "/somepath/movie/disc 1/movie.avi"},
    {"rar://%2fsomepath%2fmovie.rar/film.mkv", "/somepath/movie.avi"},
    {"rar://%2fsomepath%2fmovie.rar/BDMV/index.bdmv", "/somepath/movie.avi"},
    {"rar://%2fsomepath%2fmovie%2fdisc%201%2fmovie.rar/BDMV/index.bdmv",
     "/somepath/movie/disc 1/movie.avi"},
    {"archive://%2fsomepath%2fmovie.tar.gz/film.mkv", "/somepath/movie.avi"},
    {"archive://%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", "/somepath/movie.avi"},
    {"archive://%2fsomepath%2fmovie%2fdisc%201%2fmovie.tar.gz/BDMV/index.bdmv",
     "/somepath/movie/disc 1/movie.avi"},
    {"multipath://%2fhome%2fuser%2fbar%2f/%2fhome%2fuser%2ffoo%2f", "/home/user/bar/"},
    // Embedded DOS path tests
    {"bluray://udf%3a%2f%2fD%253a%255csomepath%255cmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "D:\\somepath\\movie.iso"},
    {"bluray://udf%3a%2f%2fD%253a%255csomepath%255cdisc%25201%255cmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "D:\\somepath\\disc 1\\movie.iso"},
    {"bluray://D%3a%5csomepath%5c/BDMV/PLAYLIST/00800.mpls", "D:\\somepath\\", false, true},
    {"bluray://D%3a%5csomepath%5cdisc%201%5c/BDMV/PLAYLIST/00800.mpls", "D:\\somepath\\disc 1\\",
     false, true},
    {"zip://D%3a%5csomepath%5cmovie.zip/film.mkv", "D:\\somepath\\movie.avi"},
    {"zip://D%3a%5csomepath%5cmovie.zip/BDMV/index.bdmv", "D:\\somepath\\movie.avi"},
    {"zip://D%3a%5csomepath%5cmovie%5cdisc%201%5cmovie.zip/BDMV/index.bdmv",
     "D:\\somepath\\movie\\disc 1\\movie.avi"},
    {"rar://D%3a%5csomepath%5cmovie.rar/film.mkv", "D:\\somepath\\movie.avi"},
    {"rar://D%3a%5csomepath%5cmovie.rar/BDMV/index.bdmv", "D:\\somepath\\movie.avi"},
    {"rar://D%3a%5csomepath%5cmovie%5cdisc%201%5cmovie.rar/BDMV/index.bdmv",
     "D:\\somepath\\movie\\disc 1\\movie.avi"},
    {"archive://D%3a%5csomepath%5cmovie.tar.gz/film.mkv", "D:\\somepath\\movie.avi"},
    {"archive://D%3a%5csomepath%5cmovie.tar.gz/BDMV/index.bdmv", "D:\\somepath\\movie.avi"},
    {"archive://D%3a%5csomepath%5cmovie%5cdisc%201%5cmovie.tar.gz/BDMV/index.bdmv",
     "D:\\somepath\\movie\\disc 1\\movie.avi"},
    {"multipath://D%3a%5chome%5cuser%5cbar%5c/D%3a%5chome%5cuser%5cfoo%5c",
     "D:\\home\\user\\bar\\"},
    // Embedded windows server path tests
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "\\\\Server\\Movies\\movie.iso"},
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cMovie%255cdisc%25201%255cmovie.iso%2f/"
     "BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "\\\\Server\\Movies\\Movie\\disc 1\\movie.iso"},
    {"bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls", "\\\\Server\\Movies\\", false,
     true},
    {"bluray://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5c/BDMV/PLAYLIST/00800.mpls",
     "\\\\Server\\Movies\\Movie\\disc 1\\", false, true},
    {"zip://%5c%5cServer%5cMovies%5cmovie.zip/film.mkv", "\\\\Server\\Movies\\movie.avi"},
    {"zip://%5c%5cServer%5cMovies%5cmovie.zip/BDMV/index.bdmv", "\\\\Server\\Movies\\movie.avi"},
    {"zip://%5c%5cServer%5cMovies%5cmovie%5cdisc%201%5cmovie.zip/BDMV/index.bdmv",
     "\\\\Server\\Movies\\movie\\disc 1\\movie.avi"},
    {"rar://%5c%5cServer%5cMovies%5cmovie.rar/film.mkv", "\\\\Server\\Movies\\movie.avi"},
    {"rar://%5c%5cServer%5cMovies%5cmovie.rar/BDMV/index.bdmv", "\\\\Server\\Movies\\movie.avi"},
    {"rar://%5c%5cServer%5cMovies%5cmovie%5cdisc%201%5cmovie.rar/BDMV/index.bdmv",
     "\\\\Server\\Movies\\movie\\disc 1\\movie.avi"},
    {"archive://%5c%5cServer%5cMovies%5cmovie.tar.gz/film.mkv", "\\\\Server\\Movies\\movie.avi"},
    {"archive://%5c%5cServer%5cMovies%5cmovie.tar.gz/BDMV/index.bdmv",
     "\\\\Server\\Movies\\movie.avi"},
    {"archive://%5c%5cServer%5cMovies%5cmovie%5cdisc%201%5cmovie.tar.gz/BDMV/index.bdmv",
     "\\\\Server\\Movies\\movie\\disc 1\\movie.avi"},
    {"multipath://%5c%5chome%5cuser%5cbar%5c/%5c%5chome%5cuser%5cfoo%5c", "\\\\home\\user\\bar\\"},
    // Special tests
    // Episodes
    {"/home/user/foo.avi", "/home/user/foo-S03E04.avi", false, false, false,
     ART::AdditionalIdentifiers::SEASON_AND_EPISODE, -1, 3, 4},
    {"/home/user/BDMV/index.bdmv", "/home/user/BDMV/index-S03E04.bdmv", false, false, false,
     ART::AdditionalIdentifiers::SEASON_AND_EPISODE, -1, 3, 4},
    {"smb://home/user/foo.avi", "smb://home/user/foo-S03E04.avi", false, false, false,
     ART::AdditionalIdentifiers::SEASON_AND_EPISODE, -1, 3, 4},
    {"D:\\home\\user\\foo.avi", "D:\\home\\user\\foo-S03E04.avi", false, false, false,
     ART::AdditionalIdentifiers::SEASON_AND_EPISODE, -1, 3, 4},
    {"\\\\Server\\user\\foo.avi", "\\\\Server\\user\\foo-S03E04.avi", false, false, false,
     ART::AdditionalIdentifiers::SEASON_AND_EPISODE, -1, 3, 4},
    {"bluray://smb%3a%2f%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls",
     "smb://somepath/BDMV/index-S03E04.bdmv", false, false, false,
     ART::AdditionalIdentifiers::SEASON_AND_EPISODE, -1, 3, 4},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252ftvshow.iso%2f/BDMV/"
     "PLAYLIST/00800.mpls",
     "smb://somepath/tvshow-S03E04.iso", false, false, false,
     ART::AdditionalIdentifiers::SEASON_AND_EPISODE, -1, 3, 4},
    // Playlists
    {"bluray://smb%3a%2f%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls",
     "smb://somepath/BDMV/index-00800.bdmv", false, false, false,
     ART::AdditionalIdentifiers::PLAYLIST, 800},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie.iso%2f/BDMV/"
     "PLAYLIST/00800.mpls",
     "smb://somepath/movie-00800.iso", false, false, false, ART::AdditionalIdentifiers::PLAYLIST,
     800}};

TEST_P(GetLocalArtBaseFilenameTest, GetLocalArtBaseFilename)
{
  CFileItem item(GetParam().path, GetParam().isFolder);
  CVideoInfoTag* tag{item.GetVideoInfoTag()};
  tag->m_iSeason = GetParam().season;
  tag->m_iEpisode = GetParam().episode;
  tag->m_iTrack = GetParam().playlist;
  bool useFolder = GetParam().force_use_folder ? true : GetParam().isFolder;

  const std::string res =
      ART::GetLocalArtBaseFilename(item, useFolder, GetParam().additionalIdentifiers);
  EXPECT_EQ(res, GetParam().result);
  EXPECT_EQ(useFolder, GetParam().result_folder);
}

INSTANTIATE_TEST_SUITE_P(TestArtUtils,
                         GetLocalArtBaseFilenameTest,
                         testing::ValuesIn(local_art_filename_tests));

// ART::GetLocalFanart() tests

struct FanartTest
{
  std::string path;
  std::string result;
};

class GetLocalFanartTest : public testing::WithParamInterface<FanartTest>, public testing::Test
{
};

const FanartTest local_fanart_tests[] = {
    {"stack://#DIRECTORY#foo-cd1.avi , #DIRECTORY#foo-cd2.avi", "foo-fanart.jpg"},
    {"stack://#DIRECTORY#foo-cd1.avi , #DIRECTORY#foo-cd2.avi", "foo-cd1-fanart.jpg"},
    {"stack://#DIRECTORY#movie/movie part 1/foo.avi , #DIRECTORY#movie/movie part 2/foo.avi",
     "movie/movie-fanart.jpg"},
    {"stack://#DIRECTORY#movie/movie part 1/foo.avi , #DIRECTORY#movie/movie part 2/foo.avi",
     "movie/movie part 1/foo-fanart.jpg"},
    {"zip://#URLENCODED_DIRECTORY#bar.zip/foo.avi", "bar-fanart.jpg"},
    {"rar://#URLENCODED_DIRECTORY#bar.rar/foo.avi", "bar-fanart.jpg"},
    {"archive://#URLENCODED_DIRECTORY#bar.tar.gz/foo.avi", "bar-fanart.jpg"},
    {"ftp://some.where/foo.avi", ""},
    {"https://some.where/foo.avi", ""},
    {"upnp://some.where/123", ""},
    {"bluray://1", ""},
    {"/home/user/1.pvr", ""},
    {"plugin://random.video/1", ""},
    {"addons://plugins/video/1", ""},
    {"dvd://1", ""},
    {"", ""},
    {"foo.avi", ""},
    {"foo.avi", "foo-fanart.jpg"},
    {"videodb://movies/1", ""},
    {"videodb://movies/1", "foo-fanart.jpg"}};

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
    if (const std::string dir{URIUtils::GetDirectory(GetParam().result)}; !dir.empty())
      XFILE::CDirectory::Create(URIUtils::AddFileToFolder(path, dir, ""));
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

  EXPECT_EQ(URIUtils::GetFileName(res), URIUtils::GetFileName(GetParam().result));

  if (!GetParam().result.empty())
    XFILE::CDirectory::RemoveRecursive(path);
}

INSTANTIATE_TEST_SUITE_P(TestArtUtils, GetLocalFanartTest, testing::ValuesIn(local_fanart_tests));

// ART::FillInDefaultIcon() tests

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

const IconTest icon_tests[] = {
    {"pvr://guide", "", "", false, false},
    {"/home/user/test.pvr", "DefaultTVShows.png"},
    {"/home/user/test.zip", "DefaultFile.png"},
    {"/home/user/test.mp3", "DefaultAudio.png"},
    {"/home/user/test.avi", "DefaultVideo.png"},
    {"/home/user/test.jpg", "DefaultPicture.png"},
    {"/home/user/test.m3u", "DefaultPlaylist.png"},
    {"/home/user/test.xsp", "DefaultPlaylist.png"},
    {"/home/user/test.py", "DefaultScript.png"},
    {"favourites://1", "DefaultFavourites.png"},
    {"/home/user/test.fil", "DefaultFile.png"},
    {"/home/user/test.m3u", "DefaultPlaylist.png", "", true},
    {"/home/user/test.xsp", "DefaultPlaylist.png", "", true},
    {"..", "DefaultFolderBack.png", "", true},
    {"/home/user/test/", "DefaultFolder.png", "", true},
    {"zip://%2fhome%2fuser%2fbar.zip/foo.avi", "DefaultVideo.png", "OverlayZIP.png"},
    {"zip://%2fhome%2fuser%2fbar.zip/foo.avi", "DefaultVideo.png", "", false, true, true},
    {"rar://%2fhome%2fuser%2fbar.rar/foo.avi", "DefaultVideo.png", "OverlayRAR.png"},
    {"rar://%2fhome%2fuser%2fbar.rar/foo.avi", "DefaultVideo.png", "", false, true, true},
    {"archive://%2fhome%2fuser%2fbar.tar.gz/foo.avi", "DefaultVideo.png", "OverlayRAR.png"},
    {"archive://%2fhome%2fuser%2fbar.tar.gz/foo.avi", "DefaultVideo.png", "", false, true, true}};

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

// ART::GetFolderThumb() tests

struct FolderTest
{
  std::string path;
  std::string thumb;
  std::string result;
};

class FolderThumbTest : public testing::WithParamInterface<FolderTest>, public testing::Test
{
};

const FolderTest folder_thumb_tests[] = {
    // Linux path tests
    {"/home/user/movies/movie/", "folder.jpg", "/home/user/movies/movie/folder.jpg"},
    {"/home/user/movies/movie/disc 1/", "folder.jpg", "/home/user/movies/movie/disc 1/folder.jpg"},
    {"/home/user/movies/movie/video_ts/", "folder.jpg", "/home/user/movies/movie/folder.jpg"},
    {"/home/user/movies/movie/disc 1/video_ts/", "folder.jpg",
     "/home/user/movies/movie/disc 1/folder.jpg"},
    {"/home/user/movies/movie/BDMV/", "folder.jpg", "/home/user/movies/movie/folder.jpg"},
    {"/home/user/movies/movie/disc 1/BDMV/", "folder.jpg",
     "/home/user/movies/movie/disc 1/folder.jpg"},
    {"stack:///path/to/movie_name/part-1.avi , /path/to/movie_name/part-2.avi", "folder.jpg",
     "/path/to/movie_name/folder.jpg"},
    // URL path tests using smb://
    {"smb://home/user/movies/movie/", "folder.jpg", "smb://home/user/movies/movie/folder.jpg"},
    {"smb://home/user/movies/movie/disc 1/", "folder.jpg",
     "smb://home/user/movies/movie/disc 1/folder.jpg"},
    {"smb://home/user/movies/movie/video_ts/", "folder.jpg",
     "smb://home/user/movies/movie/folder.jpg"},
    {"smb://home/user/movies/movie/disc 1/video_ts/", "folder.jpg",
     "smb://home/user/movies/movie/disc 1/folder.jpg"},
    {"smb://home/user/movies/movie/BDMV/", "folder.jpg", "smb://home/user/movies/movie/folder.jpg"},
    {"smb://home/user/movies/movie/disc 1/BDMV/", "folder.jpg",
     "smb://home/user/movies/movie/disc 1/folder.jpg"},
    {"stack://smb://path/to/movie_name/part-1.avi , "
     "smb://path/to/movie_name/part-2.avi",
     "folder.jpg", "smb://path/to/movie_name/folder.jpg"},
    // DOS path tests
    {"D:\\movies\\movie\\", "folder.jpg", "D:\\movies\\movie\\folder.jpg"},
    {"D:\\movies\\movie\\disc 1\\", "folder.jpg", "D:\\movies\\movie\\disc 1\\folder.jpg"},
    {"D:\\movies\\movie\\video_ts\\", "folder.jpg", "D:\\movies\\movie\\folder.jpg"},
    {"D:\\movies\\movie\\disc 1\\video_ts\\", "folder.jpg",
     "D:\\movies\\movie\\disc 1\\folder.jpg"},
    {"D:\\movies\\movie\\BDMV\\", "folder.jpg", "D:\\movies\\movie\\folder.jpg"},
    {"D:\\movies\\movie\\disc 1\\BDMV\\", "folder.jpg", "D:\\movies\\movie\\disc 1\\folder.jpg"},
    {"stack://D:\\movies\\movie_name\\part-1.avi , D:\\movies\\movie_name\\part-2.avi",
     "folder.jpg", "D:\\movies\\movie_name\\folder.jpg"},
    // Windows server path tests
    {"\\\\Server\\Movies\\Movie\\", "folder.jpg", "\\\\Server\\Movies\\Movie\\folder.jpg"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\", "folder.jpg",
     "\\\\Server\\Movies\\Movie\\disc 1\\folder.jpg"},
    {"\\\\Server\\Movies\\Movie\\video_ts\\", "folder.jpg",
     "\\\\Server\\Movies\\Movie\\folder.jpg"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\video_ts\\", "folder.jpg",
     "\\\\Server\\Movies\\Movie\\disc 1\\folder.jpg"},
    {"\\\\Server\\Movies\\Movie\\BDMV\\", "folder.jpg", "\\\\Server\\Movies\\Movie\\folder.jpg"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\BDMV\\", "folder.jpg",
     "\\\\Server\\Movies\\Movie\\disc 1\\folder.jpg"},
    {"stack://\\\\Server\\Movies\\Movie_name\\part-1.avi , "
     "\\\\Server\\Movies\\Movie_name\\part-2.avi",
     "folder.jpg", "\\\\Server\\Movies\\Movie_name\\folder.jpg"},
    // Embedded URL path tests with smb://
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "folder.jpg", "smb://somepath/folder.jpg"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "folder.jpg", "smb://somepath/disc 1/folder.jpg"},
    {"bluray://smb%3a%2f%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls", "folder.jpg",
     "smb://somepath/folder.jpg"},
    {"bluray://smb%3a%2f%2fsomepath%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls", "folder.jpg",
     "smb://somepath/disc 1/folder.jpg"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie.zip/film.mkv", "folder.jpg", "smb://somepath/folder.jpg"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie.zip/BDMV/index.bdmv", "folder.jpg",
     "smb://somepath/folder.jpg"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie_disc.zip/BDMV/index.bdmv", "folder.jpg",
     "smb://somepath/movie/disc 1/folder.jpg"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie.rar/film.mkv", "folder.jpg", "smb://somepath/folder.jpg"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie.rar/BDMV/index.bdmv", "folder.jpg",
     "smb://somepath/folder.jpg"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie_disc.rar/BDMV/index.bdmv", "folder.jpg",
     "smb://somepath/movie/disc 1/folder.jpg"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie.tar.gz/film.mkv", "folder.jpg",
     "smb://somepath/folder.jpg"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", "folder.jpg",
     "smb://somepath/folder.jpg"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie_disc.tar.gz/BDMV/index.bdmv",
     "folder.jpg", "smb://somepath/movie/disc 1/folder.jpg"},
    {"multipath://smb%3a%2f%2fhome%2fuser%2fbar%2f/smb%3a%2f%2fhome%2fuser%2ffoo%2f", "folder.jpg",
     "smb://home/user/bar/folder.jpg"},
    // Embedded linux path tests
    {"bluray://udf%3a%2f%2f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "folder.jpg", "/somepath/folder.jpg"},
    {"bluray://udf%3a%2f%2f%252fsomepath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "folder.jpg", "/somepath/disc 1/folder.jpg"},
    {"bluray://%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls", "folder.jpg", "/somepath/folder.jpg"},
    {"bluray://%2fsomepath%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls", "folder.jpg",
     "/somepath/disc 1/folder.jpg"},
    {"zip://%2fsomepath%2fmovie.zip/BDMV/index.bdmv", "folder.jpg", "/somepath/folder.jpg"},
    {"zip://%2fsomepath%2fmovie%2fdisc%201%2fmovie_disc.zip/BDMV/index.bdmv", "folder.jpg",
     "/somepath/movie/disc 1/folder.jpg"},
    {"rar://%2fsomepath%2fmovie.rar/BDMV/index.bdmv", "folder.jpg", "/somepath/folder.jpg"},
    {"rar://%2fsomepath%2fmovie%2fdisc%201%2fmovie_disc.rar/BDMV/index.bdmv", "folder.jpg",
     "/somepath/movie/disc 1/folder.jpg"},
    {"archive://%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", "folder.jpg", "/somepath/folder.jpg"},
    {"archive://%2fsomepath%2fmovie%2fdisc%201%2fmovie_disc.tar.gz/BDMV/index.bdmv", "folder.jpg",
     "/somepath/movie/disc 1/folder.jpg"},
    {"multipath://%2fhome%2fuser%2fbar%2f/%2fhome%2fuser%2ffoo%2f", "folder.jpg",
     "/home/user/bar/folder.jpg"},
    // Embedded DOS path tests
    {"bluray://udf%3a%2f%2fD%253a%255csomepath%255cmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "folder.jpg", "D:\\somepath\\folder.jpg"},
    {"bluray://udf%3a%2f%2fD%253a%255csomepath%255cdisc%25201%255cmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "folder.jpg", "D:\\somepath\\disc 1\\folder.jpg"},
    {"bluray://D%3a%5csomepath%5c/BDMV/PLAYLIST/00800.mpls", "folder.jpg",
     "D:\\somepath\\folder.jpg"},
    {"bluray://D%3a%5csomepath%5cdisc%201%5c/BDMV/PLAYLIST/00800.mpls", "folder.jpg",
     "D:\\somepath\\disc 1\\folder.jpg"},
    {"zip://D%3a%5csomepath%5cmovie.zip/BDMV/index.bdmv", "folder.jpg", "D:\\somepath\\folder.jpg"},
    {"zip://D%3a%5csomepath%5cmovie%5cdisc%201%5cmovie_disc.zip/BDMV/index.bdmv", "folder.jpg",
     "D:\\somepath\\movie\\disc 1\\folder.jpg"},
    {"rar://D%3a%5csomepath%5cmovie.rar/BDMV/index.bdmv", "folder.jpg", "D:\\somepath\\folder.jpg"},
    {"rar://D%3a%5csomepath%5cmovie%5cdisc%201%5cmovie_disc.rar/BDMV/index.bdmv", "folder.jpg",
     "D:\\somepath\\movie\\disc 1\\folder.jpg"},
    {"archive://D%3a%5csomepath%5cmovie.tar.gz/BDMV/index.bdmv", "folder.jpg",
     "D:\\somepath\\folder.jpg"},
    {"archive://D%3a%5csomepath%5cmovie%5cdisc%201%5cmovie_disc.tar.gz/BDMV/index.bdmv",
     "folder.jpg", "D:\\somepath\\movie\\disc 1\\folder.jpg"},
    {"multipath://D%3a%5chome%5cuser%5cbar%5c/D%3a%5chome%5cuser%5cfoo%5c", "folder.jpg",
     "D:\\home\\user\\bar\\folder.jpg"},
    // Embedded Windows server path tests
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cMovie%255cmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "folder.jpg", "\\\\Server\\Movies\\Movie\\folder.jpg"},
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cMovie%255cdisc%25201%255cmovie.iso%2f/"
     "BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "folder.jpg", "\\\\Server\\Movies\\Movie\\disc 1\\folder.jpg"},
    {"bluray://%5c%5cServer%5cMovies%5cMovie%5c/BDMV/PLAYLIST/00800.mpls", "folder.jpg",
     "\\\\Server\\Movies\\Movie\\folder.jpg"},
    {"bluray://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5c/BDMV/PLAYLIST/00800.mpls", "folder.jpg",
     "\\\\Server\\Movies\\Movie\\disc 1\\folder.jpg"},
    {"zip://%5c%5cServer%5cMovies%5cMovie%5cmovie.zip/BDMV/index.bdmv", "folder.jpg",
     "\\\\Server\\Movies\\Movie\\folder.jpg"},
    {"zip://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5cmovie_disc.zip/BDMV/index.bdmv",
     "folder.jpg", "\\\\Server\\Movies\\Movie\\disc 1\\folder.jpg"},
    {"rar://%5c%5cServer%5cMovies%5cMovie%5cmovie.rar/BDMV/index.bdmv", "folder.jpg",
     "\\\\Server\\Movies\\Movie\\folder.jpg"},
    {"rar://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5cmovie_disc.rar/BDMV/index.bdmv",
     "folder.jpg", "\\\\Server\\Movies\\Movie\\disc 1\\folder.jpg"},
    {"archive://%5c%5cServer%5cMovies%5cMovie%5cmovie.tar.gz/BDMV/index.bdmv", "folder.jpg",
     "\\\\Server\\Movies\\Movie\\folder.jpg"},
    {"archive://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5cmovie_disc.tar.gz/BDMV/index.bdmv",
     "folder.jpg", "\\\\Server\\Movies\\Movie\\disc 1\\folder.jpg"},
    {"multipath://%5c%5chome%5cuser%5cbar%5c/5c%5chome%5cuser%5cfoo%5c", "folder.jpg",
     "\\\\home\\user\\bar\\folder.jpg"}};

TEST_P(FolderThumbTest, GetFolderThumb)
{
  CFileItem item(GetParam().path, true);
  const std::string thumb = ART::GetFolderThumb(item, GetParam().thumb);
  EXPECT_EQ(thumb, GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(TestArtUtils, FolderThumbTest, testing::ValuesIn(folder_thumb_tests));

// ART::GetLocalArt() tests

struct LocalArtTest
{
  std::string file;
  std::string art;
  bool use_folder;
  std::string base;
  int season{-1};
  int episode{-1};
  int playlist{-1};
  ART::AdditionalIdentifiers additionalIdentifiers{ART::AdditionalIdentifiers::NONE};
};

class TestLocalArt : public AdvancedSettingsResetBase,
                     public testing::WithParamInterface<LocalArtTest>
{
};

const LocalArtTest local_art_tests[] = {
    // Linux path tests
    {"/home/user/movies/movie/movie.iso", "art.jpg", false,
     "/home/user/movies/movie/movie-art.jpg"},
    {"/home/user/movies/movie/file.iso", "art.jpg", true, "/home/user/movies/movie/art.jpg"},
    {"/home/user/movies/movie/disc 1/movie.iso", "art.jpg", false,
     "/home/user/movies/movie/disc 1/movie-art.jpg"},
    {"/home/user/movies/movie/disc 1/file.iso", "art.jpg", true,
     "/home/user/movies/movie/disc 1/art.jpg"},
    {"/home/user/movies/movie/video_ts/VIDEO_TS.IFO", "art.jpg", false,
     "/home/user/movies/movie/art.jpg"},
    {"/home/user/movies/movie/video_ts/VIDEO_TS.IFO", "art.jpg", true,
     "/home/user/movies/movie/art.jpg"},
    {"/home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", "art.jpg", false,
     "/home/user/movies/movie/disc 1/art.jpg"},
    {"/home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", "art.jpg", true,
     "/home/user/movies/movie/disc 1/art.jpg"},
    {"/home/user/movies/movie/BDMV/index.bdmv", "art.jpg", false,
     "/home/user/movies/movie/art.jpg"},
    {"/home/user/movies/movie/BDMV/index.bdmv", "art.jpg", true, "/home/user/movies/movie/art.jpg"},
    {"/home/user/movies/movie/disc 1/BDMV/index.bdmv", "art.jpg", false,
     "/home/user/movies/movie/disc 1/art.jpg"},
    {"/home/user/movies/movie/disc 1/BDMV/index.bdmv", "art.jpg", true,
     "/home/user/movies/movie/disc 1/art.jpg"},
    {"stack:///path/to/movie_name/movie-part-1.avi , /path/to/movie_name/movie-part-2.avi",
     "art.jpg", false, "/path/to/movie_name/movie-art.jpg"},
    {"stack:///path/to/movie/cd1/some_file1.avi , /path/to/movie/cd2/some_file2.avi", "art.jpg",
     true, "/path/to/movie/art.jpg"},
    // URL path tests using smb://
    {"smb://home/user/movies/movie/movie.iso", "art.jpg", false,
     "smb://home/user/movies/movie/movie-art.jpg"},
    {"smb://home/user/movies/movie/file.iso", "art.jpg", true,
     "smb://home/user/movies/movie/art.jpg"},
    {"smb://home/user/movies/movie/disc 1/movie.iso", "art.jpg", false,
     "smb://home/user/movies/movie/disc 1/movie-art.jpg"},
    {"smb://home/user/movies/movie/disc 1/file.iso", "art.jpg", true,
     "smb://home/user/movies/movie/disc 1/art.jpg"},
    {"smb://home/user/movies/movie/video_ts/VIDEO_TS.IFO", "art.jpg", false,
     "smb://home/user/movies/movie/art.jpg"},
    {"smb://home/user/movies/movie/video_ts/VIDEO_TS.IFO", "art.jpg", true,
     "smb://home/user/movies/movie/art.jpg"},
    {"smb://home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", "art.jpg", false,
     "smb://home/user/movies/movie/disc 1/art.jpg"},
    {"smb://home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", "art.jpg", true,
     "smb://home/user/movies/movie/disc 1/art.jpg"},
    {"smb://home/user/movies/movie/BDMV/index.bdmv", "art.jpg", false,
     "smb://home/user/movies/movie/art.jpg"},
    {"smb://home/user/movies/movie/BDMV/index.bdmv", "art.jpg", true,
     "smb://home/user/movies/movie/art.jpg"},
    {"smb://home/user/movies/movie/disc 1/BDMV/index.bdmv", "art.jpg", false,
     "smb://home/user/movies/movie/disc 1/art.jpg"},
    {"smb://home/user/movies/movie/disc 1/BDMV/index.bdmv", "art.jpg", true,
     "smb://home/user/movies/movie/disc 1/art.jpg"},
    {"stack://smb://path/to/movie_name/movie-part-1.avi , "
     "smb://path/to/movie_name/movie-part-2.avi",
     "art.jpg", false, "smb://path/to/movie_name/movie-art.jpg"},
    {"stack://smb://path/to/movie/cd1/some_file1.avi , smb://path/to/movie/cd2/some_file2.avi",
     "art.jpg", true, "smb://path/to/movie/art.jpg"},
    // DOS path tests
    {"D:\\movies\\movie\\movie.iso", "art.jpg", false, "D:\\movies\\movie\\movie-art.jpg"},
    {"D:\\movies\\movie\\file.iso", "art.jpg", true, "D:\\movies\\movie\\art.jpg"},
    {"D:\\movies\\movie\\disc 1\\movie.iso", "art.jpg", false,
     "D:\\movies\\movie\\disc 1\\movie-art.jpg"},
    {"D:\\movies\\movie\\disc 1\\file.iso", "art.jpg", true, "D:\\movies\\movie\\disc 1\\art.jpg"},
    {"D:\\movies\\movie\\video_ts\\VIDEO_TS.IFO", "art.jpg", false, "D:\\movies\\movie\\art.jpg"},
    {"D:\\movies\\movie\\video_ts\\VIDEO_TS.IFO", "art.jpg", true, "D:\\movies\\movie\\art.jpg"},
    {"D:\\movies\\movie\\disc 1\\video_ts\\VIDEO_TS.IFO", "art.jpg", false,
     "D:\\movies\\movie\\disc 1\\art.jpg"},
    {"D:\\movies\\movie\\disc 1\\video_ts\\VIDEO_TS.IFO", "art.jpg", true,
     "D:\\movies\\movie\\disc 1\\art.jpg"},
    {"D:\\movies\\movie\\BDMV\\index.bdmv", "art.jpg", false, "D:\\movies\\movie\\art.jpg"},
    {"D:\\movies\\movie\\BDMV/index.bdmv", "art.jpg", true, "D:\\movies\\movie\\art.jpg"},
    {"D:\\movies\\movie\\disc 1\\BDMV\\index.bdmv", "art.jpg", false,
     "D:\\movies\\movie\\disc 1\\art.jpg"},
    {"D:\\movies\\movie\\disc 1\\BDMV\\index.bdmv", "art.jpg", true,
     "D:\\movies\\movie\\disc 1\\art.jpg"},
    {"stack://D:\\movies\\movie_name\\movie-part-1.avi , D:\\movies\\movie_name\\movie-part-2.avi",
     "art.jpg", false, "D:\\movies\\movie_name\\movie-art.jpg"},
    {"stack://D:\\movies\\movie\\cd1\\some_file1.avi , D:\\movies\\movie\\cd2\\some_file2.avi",
     "art.jpg", true, "D:\\movies\\movie\\art.jpg"},
    // Windows server path tests
    {"\\\\Server\\Movies\\Movie\\movie.iso", "art.jpg", false,
     "\\\\Server\\Movies\\Movie\\movie-art.jpg"},
    {"\\\\Server\\Movies\\Movie\\file.iso", "art.jpg", true, "\\\\Server\\Movies\\Movie\\art.jpg"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\movie.iso", "art.jpg", false,
     "\\\\Server\\Movies\\Movie\\disc 1\\movie-art.jpg"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\file.iso", "art.jpg", true,
     "\\\\Server\\Movies\\Movie\\disc 1\\art.jpg"},
    {"\\\\Server\\Movies\\Movie\\video_ts\\VIDEO_TS.IFO", "art.jpg", false,
     "\\\\Server\\Movies\\Movie\\art.jpg"},
    {"\\\\Server\\Movies\\Movie\\video_ts\\VIDEO_TS.IFO", "art.jpg", true,
     "\\\\Server\\Movies\\Movie\\art.jpg"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\video_ts\\VIDEO_TS.IFO", "art.jpg", false,
     "\\\\Server\\Movies\\Movie\\disc 1\\art.jpg"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\video_ts\\VIDEO_TS.IFO", "art.jpg", true,
     "\\\\Server\\Movies\\Movie\\disc 1\\art.jpg"},
    {"\\\\Server\\Movies\\Movie\\BDMV\\index.bdmv", "art.jpg", false,
     "\\\\Server\\Movies\\Movie\\art.jpg"},
    {"\\\\Server\\Movies\\Movie\\BDMV/index.bdmv", "art.jpg", true,
     "\\\\Server\\Movies\\Movie\\art.jpg"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", "art.jpg", false,
     "\\\\Server\\Movies\\Movie\\disc 1\\art.jpg"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", "art.jpg", true,
     "\\\\Server\\Movies\\Movie\\disc 1\\art.jpg"},
    {"stack://\\\\Server\\Movies\\Movie_name\\movie-part-1.avi , "
     "\\\\Server\\Movies\\Movie_name\\movie-part-2.avi",
     "art.jpg", false, "\\\\Server\\Movies\\Movie_name\\movie-art.jpg"},
    {"stack://\\\\Server\\Movies\\Movie\\cd1\\some_file1.avi , "
     "\\\\Server\\Movies\\Movie\\cd2\\some_file2.avi",
     "art.jpg", true, "\\\\Server\\Movies\\Movie\\art.jpg"},
    // Embedded URL path tests with smb://
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "art.jpg", false, "smb://somepath/movie-art.jpg"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "art.jpg", true, "smb://somepath/art.jpg"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "art.jpg", true, "smb://somepath/disc 1/art.jpg"},
    {"bluray://smb%3a%2f%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls", "art.jpg", false,
     "smb://somepath/art.jpg"},
    {"bluray://smb%3a%2f%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls", "art.jpg", true,
     "smb://somepath/art.jpg"},
    {"bluray://smb%3a%2f%2fsomepath%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls", "art.jpg", true,
     "smb://somepath/disc 1/art.jpg"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie.zip/BDMV/index.bdmv", "art.jpg", false,
     "smb://somepath/movie-art.jpg"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie.zip/BDMV/index.bdmv", "art.jpg", true,
     "smb://somepath/art.jpg"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie_disc.zip/BDMV/index.bdmv", "art.jpg",
     true, "smb://somepath/movie/disc 1/art.jpg"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie.rar/BDMV/index.bdmv", "art.jpg", false,
     "smb://somepath/movie-art.jpg"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie.rar/BDMV/index.bdmv", "art.jpg", true,
     "smb://somepath/art.jpg"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie_disc.rar/BDMV/index.bdmv", "art.jpg",
     true, "smb://somepath/movie/disc 1/art.jpg"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", "art.jpg", false,
     "smb://somepath/movie-art.jpg"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", "art.jpg", true,
     "smb://somepath/art.jpg"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie_disc.tar.gz/BDMV/index.bdmv",
     "art.jpg", true, "smb://somepath/movie/disc 1/art.jpg"},
    // Embedded linux path tests
    {"bluray://udf%3a%2f%2f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "art.jpg", false, "/somepath/movie-art.jpg"},
    {"bluray://udf%3a%2f%2f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "art.jpg", true, "/somepath/art.jpg"},
    {"bluray://udf%3a%2f%2f%252fsomepath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "art.jpg", true, "/somepath/disc 1/art.jpg"},
    {"bluray://%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls", "art.jpg", false, "/somepath/art.jpg"},
    {"bluray://%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls", "art.jpg", true, "/somepath/art.jpg"},
    {"bluray://%2fsomepath%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls", "art.jpg", true,
     "/somepath/disc 1/art.jpg"},
    {"zip://%2fsomepath%2fmovie.zip/BDMV/index.bdmv", "art.jpg", false, "/somepath/movie-art.jpg"},
    {"zip://%2fsomepath%2fmovie.zip/BDMV/index.bdmv", "art.jpg", true, "/somepath/art.jpg"},
    {"zip://%2fsomepath%2fmovie%2fdisc%201%2fmovie_disc.zip/BDMV/index.bdmv", "art.jpg", true,
     "/somepath/movie/disc 1/art.jpg"},
    {"rar://%2fsomepath%2fmovie.rar/BDMV/index.bdmv", "art.jpg", false, "/somepath/movie-art.jpg"},
    {"rar://%2fsomepath%2fmovie.rar/BDMV/index.bdmv", "art.jpg", true, "/somepath/art.jpg"},
    {"rar://%2fsomepath%2fmovie%2fdisc%201%2fmovie_disc.rar/BDMV/index.bdmv", "art.jpg", true,
     "/somepath/movie/disc 1/art.jpg"},
    {"archive://%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", "art.jpg", false,
     "/somepath/movie-art.jpg"},
    {"archive://%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", "art.jpg", true, "/somepath/art.jpg"},
    {"archive://%2fsomepath%2fmovie%2fdisc%201%2fmovie_disc.tar.gz/BDMV/index.bdmv", "art.jpg",
     true, "/somepath/movie/disc 1/art.jpg"},
    // Embedded DOS path tests
    {"bluray://udf%3a%2f%2fD%253a%255csomepath%255cmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "art.jpg", false, "D:\\somepath\\movie-art.jpg"},
    {"bluray://udf%3a%2f%2fD%253a%255csomepath%255cmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "art.jpg", true, "D:\\somepath\\art.jpg"},
    {"bluray://udf%3a%2f%2fD%253a%255csomepath%255cdisc%25201%255cmovie.iso%5c/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "art.jpg", true, "D:\\somepath\\disc 1\\art.jpg"},
    {"bluray://D%3a%5csomepath%5c/BDMV/PLAYLIST/00800.mpls", "art.jpg", false,
     "D:\\somepath\\art.jpg"},
    {"bluray://D%3a%5csomepath%5c/BDMV/PLAYLIST/00800.mpls", "art.jpg", true,
     "D:\\somepath\\art.jpg"},
    {"bluray://D%3a%5csomepath%5cdisc%201%5c/BDMV/PLAYLIST/00800.mpls", "art.jpg", true,
     "D:\\somepath\\disc 1\\art.jpg"},
    {"zip://D%3a%5csomepath%5cmovie.zip/BDMV/index.bdmv", "art.jpg", false,
     "D:\\somepath\\movie-art.jpg"},
    {"zip://D%3a%5csomepath%5cmovie.zip/BDMV/index.bdmv", "art.jpg", true, "D:\\somepath\\art.jpg"},
    {"zip://D%3a%5csomepath%5cmovie%5cdisc%201%5cmovie_disc.zip/BDMV/index.bdmv", "art.jpg", true,
     "D:\\somepath\\movie\\disc 1\\art.jpg"},
    {"rar://D%3a%5csomepath%5cmovie.rar/BDMV/index.bdmv", "art.jpg", false,
     "D:\\somepath\\movie-art.jpg"},
    {"rar://D%3a%5csomepath%5cmovie.rar/BDMV/index.bdmv", "art.jpg", true, "D:\\somepath\\art.jpg"},
    {"rar://D%3a%5csomepath%5cmovie%5cdisc%201%5cmovie_disc.rar/BDMV/index.bdmv", "art.jpg", true,
     "D:\\somepath\\movie\\disc 1\\art.jpg"},
    {"archive://D%3a%5csomepath%5cmovie.tar.gz/BDMV/index.bdmv", "art.jpg", false,
     "D:\\somepath\\movie-art.jpg"},
    {"archive://D%3a%5csomepath%5cmovie.tar.gz/BDMV/index.bdmv", "art.jpg", true,
     "D:\\somepath\\art.jpg"},
    {"archive://D%3a%5csomepath%5cmovie%5cdisc%201%5cmovie_disc.tar.gz/BDMV/index.bdmv", "art.jpg",
     true, "D:\\somepath\\movie\\disc 1\\art.jpg"},
    // Embedded Windows server path tests
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "art.jpg", false, "\\\\Server\\Movies\\movie-art.jpg"},
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cMovie%255cmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "art.jpg", true, "\\\\Server\\Movies\\Movie\\art.jpg"},
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cMovie%255cdisc%25201%255cmovie.iso%2f/"
     "BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "art.jpg", true, "\\\\Server\\Movies\\Movie\\disc 1\\art.jpg"},
    {"bluray://%5c%5cServer%5cMovies%5cMovie%5c/BDMV/PLAYLIST/00800.mpls", "art.jpg", false,
     "\\\\Server\\Movies\\Movie\\art.jpg"},
    {"bluray://%5c%5cServer%5cMovies%5cMovie%5c/BDMV/PLAYLIST/00800.mpls", "art.jpg", true,
     "\\\\Server\\Movies\\Movie\\art.jpg"},
    {"bluray://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5c/BDMV/PLAYLIST/00800.mpls", "art.jpg",
     true, "\\\\Server\\Movies\\Movie\\disc 1\\art.jpg"},
    {"zip://%5c%5cServer%5cMovies%5cMovie%5cmovie.zip/BDMV/index.bdmv", "art.jpg", false,
     "\\\\Server\\Movies\\Movie\\movie-art.jpg"},
    {"zip://%5c%5cServer%5cMovies%5cMovie%5cmovie.zip/BDMV/index.bdmv", "art.jpg", true,
     "\\\\Server\\Movies\\Movie\\art.jpg"},
    {"zip://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5cmovie_disc.zip/BDMV/index.bdmv", "art.jpg",
     true, "\\\\Server\\Movies\\Movie\\disc 1\\art.jpg"},
    {"rar://%5c%5cServer%5cMovies%5cMovie%5cmovie.rar/BDMV/index.bdmv", "art.jpg", false,
     "\\\\Server\\Movies\\Movie\\movie-art.jpg"},
    {"rar://%5c%5cServer%5cMovies%5cMovie%5cmovie.rar/BDMV/index.bdmv", "art.jpg", true,
     "\\\\Server\\Movies\\Movie\\art.jpg"},
    {"rar://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5cmovie_disc.rar/BDMV/index.bdmv", "art.jpg",
     true, "\\\\Server\\Movies\\Movie\\disc 1\\art.jpg"},
    {"archive://%5c%5cServer%5cMovies%5cMovie%5cmovie.tar.gz/BDMV/index.bdmv", "art.jpg", false,
     "\\\\Server\\Movies\\Movie\\movie-art.jpg"},
    {"archive://%5c%5cServer%5cMovies%5cMovie%5cmovie.tar.gz/BDMV/index.bdmv", "art.jpg", true,
     "\\\\Server\\Movies\\Movie\\art.jpg"},
    {"archive://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5cmovie_disc.tar.gz/BDMV/index.bdmv",
     "art.jpg", true, "\\\\Server\\Movies\\Movie\\disc 1\\art.jpg"},
    // Special tests
    // Blank art file
    {"/home/user/movies/movie/movie.iso", "", false, "/home/user/movies/movie/movie.tbn"},
    {"D:\\movies\\movie\\movie.iso", "", false, "D:\\movies\\movie\\movie.tbn"},
    // Episodes
    {"/home/user/movies/movie_name/BDMV/index.bdmv", "thumb.jpg", false,
     "/home/user/movies/movie_name/BDMV/index-S03E04-thumb.jpg", 3, 4, -1,
     ART::AdditionalIdentifiers::SEASON_AND_EPISODE},
    {"/home/user/tv_show/tv_show.iso", "thumb.jpg", false,
     "/home/user/tv_show/tv_show-S03E04-thumb.jpg", 3, 4, -1,
     ART::AdditionalIdentifiers::SEASON_AND_EPISODE},
    // Playlists
    {"/home/user/movies/movie_name/BDMV/index.bdmv", "thumb.jpg", false,
     "/home/user/movies/movie_name/BDMV/index-00800-thumb.jpg", -1, -1, 800,
     ART::AdditionalIdentifiers::PLAYLIST},
    {"/home/user/movie/movie.iso", "thumb.jpg", false, "/home/user/movie/movie-00800-thumb.jpg", -1,
     -1, 800, ART::AdditionalIdentifiers::PLAYLIST}};

TEST_P(TestLocalArt, GetLocalArt)
{
  CFileItem item;
  item.SetPath(GetParam().file);
  CVideoInfoTag* tag{item.GetVideoInfoTag()};
  tag->m_iSeason = GetParam().season;
  tag->m_iEpisode = GetParam().episode;
  tag->m_iTrack = GetParam().playlist;
  std::string path = ART::GetLocalArt(item, GetParam().art, GetParam().use_folder,
                                      GetParam().additionalIdentifiers);
  EXPECT_EQ(path, GetParam().base);
}

INSTANTIATE_TEST_SUITE_P(TestArtUtils, TestLocalArt, testing::ValuesIn(local_art_tests));

// ART::GetTBNFile() tests

struct TbnTest
{
  std::string path;
  std::string result;
  bool isFolder{false};
  int season{-1};
  int episode{-1};
};

class GetTbnTest : public testing::WithParamInterface<TbnTest>, public testing::Test
{
};

const TbnTest tbn_tests[] = {
    // Linux path tests
    {"/home/user/movies/movie/movie.avi", "/home/user/movies/movie/movie.tbn"},
    {"/home/user/movies/movie/movie.xbt", "/home/user/movies/movie/movie.tbn", true}, // File folder
    {"/home/user/movies/movie/", "/home/user/movies/movie.tbn", true},
    {"/home/user/movies/movie/disc 1/movie.avi", "/home/user/movies/movie/disc 1/movie.tbn"},
    {"/home/user/movies/movie/video_ts/video_ts.ifo",
     "/home/user/movies/movie/video_ts/video_ts.tbn"},
    {"/home/user/movies/movie/disc 1/video_ts/video_ts.ifo",
     "/home/user/movies/movie/disc 1/video_ts/video_ts.tbn"},
    {"/home/user/movies/movie/BDMV/index.bdmv", "/home/user/movies/movie/BDMV/index.tbn"},
    {"/home/user/movies/movie/disc 1/BDMV/index.bdmv",
     "/home/user/movies/movie/disc 1/BDMV/index.tbn"},
    {"stack:///home/user/movies/movie_name/movie-part-1.avi , "
     "/home/user/movies/movie_name/movie-part-2.avi",
     "/home/user/movies/movie_name/movie.tbn"},
    {"stack:///home/user/movies/movie/movie part 1/file.avi , /home/user/movies/movie/movie part "
     "2/file.avi",
     "/home/user/movies/movie/movie.tbn"},
    // URL path tests using smb://
    {"smb://home/user/movies/movie/movie.avi", "smb://home/user/movies/movie/movie.tbn"},
    {"smb://home/user/movies/movie/movie.xbt", "smb://home/user/movies/movie/movie.tbn",
     true}, // File folder
    {"smb://home/user/movies/movie/", "smb://home/user/movies/movie.tbn", true},
    {"smb://home/user/movies/movie/disc 1/movie.avi",
     "smb://home/user/movies/movie/disc 1/movie.tbn"},
    {"smb://home/user/movies/movie/video_ts/video_ts.ifo",
     "smb://home/user/movies/movie/video_ts/video_ts.tbn"},
    {"smb://home/user/movies/movie/disc 1/video_ts/video_ts.ifo",
     "smb://home/user/movies/movie/disc 1/video_ts/video_ts.tbn"},
    {"smb://home/user/movies/movie/BDMV/index.bdmv", "smb://home/user/movies/movie/BDMV/index.tbn"},
    {"smb://home/user/movies/movie/disc 1/BDMV/index.bdmv",
     "smb://home/user/movies/movie/disc 1/BDMV/index.tbn"},
    // DOS path tests
    {"D:\\Movies\\movies\\movie\\movie.avi", "D:\\Movies\\movies\\movie\\movie.tbn"},
    {"D:\\Movies\\movies\\movie\\movie.xbt", "D:\\Movies\\movies\\movie\\movie.tbn",
     true}, // File folder
    {"D:\\Movies\\movies\\movie\\", "D:\\Movies\\movies\\movie.tbn", true},
    {"D:\\Movies\\movies\\movie\\disc 1\\movie.avi",
     "D:\\Movies\\movies\\movie\\disc 1\\movie.tbn"},
    {"D:\\Movies\\movies\\movie\\video_ts\\video_ts.ifo",
     "D:\\Movies\\movies\\movie\\video_ts\\video_ts.tbn"},
    {"D:\\Movies\\movies\\movie\\disc 1\\video_ts\\video_ts.ifo",
     "D:\\Movies\\movies\\movie\\disc 1\\video_ts\\video_ts.tbn"},
    {"D:\\Movies\\movies\\movie\\BDMV\\index.bdmv", "D:\\Movies\\movies\\movie\\BDMV\\index.tbn"},
    {"D:\\Movies\\movies\\movie\\disc 1\\BDMV\\index.bdmv",
     "D:\\Movies\\movies\\movie\\disc 1\\BDMV\\index.tbn"},
    {"stack://D:\\Movies\\movies\\movie_name\\movie-part-1.avi , "
     "D:\\Movies\\movies\\movie_name\\movie-part-2.avi",
     "D:\\Movies\\movies\\movie_name\\movie.tbn"},
    {"stack://D:\\Movies\\movies\\movie\\movie part 1\\file.avi , "
     "D:\\Movies\\movies\\movie\\movie part 2\\file.avi",
     "D:\\Movies\\movies\\movie\\movie.tbn"},
    // Windows server path tests
    {"\\\\Server\\Movies\\movies\\movie\\movie.avi",
     "\\\\Server\\Movies\\movies\\movie\\movie.tbn"},
    {"\\\\Server\\Movies\\movies\\movie\\movie.xbt", "\\\\Server\\Movies\\movies\\movie\\movie.tbn",
     true}, // File folder
    {"\\\\Server\\Movies\\movies\\movie\\", "\\\\Server\\Movies\\movies\\movie.tbn", true},
    {"\\\\Server\\Movies\\movies\\movie\\disc 1\\movie.avi",
     "\\\\Server\\Movies\\movies\\movie\\disc 1\\movie.tbn"},
    {"\\\\Server\\Movies\\movies\\movie\\video_ts\\video_ts.ifo",
     "\\\\Server\\Movies\\movies\\movie\\video_ts\\video_ts.tbn"},
    {"\\\\Server\\Movies\\movies\\movie\\disc 1\\video_ts\\video_ts.ifo",
     "\\\\Server\\Movies\\movies\\movie\\disc 1\\video_ts\\video_ts.tbn"},
    {"\\\\Server\\Movies\\movies\\movie\\BDMV\\index.bdmv",
     "\\\\Server\\Movies\\movies\\movie\\BDMV\\index.tbn"},
    {"\\\\Server\\Movies\\movies\\movie\\disc 1\\BDMV\\index.bdmv",
     "\\\\Server\\Movies\\movies\\movie\\disc 1\\BDMV\\index.tbn"},
    {"stack://\\\\Server\\Movies\\movies\\movie_name\\movie-part-1.avi , "
     "\\\\Server\\Movies\\movies\\movie_name\\movie-part-2.avi",
     "\\\\Server\\Movies\\movies\\movie_name\\movie.tbn"},
    {"stack://\\\\Server\\Movies\\movies\\movie\\movie part 1\\file.avi , "
     "\\\\Server\\Movies\\movies\\movie\\movie part 2\\file.avi",
     "\\\\Server\\Movies\\movies\\movie\\movie.tbn"},
    // Embedded linux path tests
    {"bluray://udf%3a%2f%2f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "/somepath/movie.tbn"},
    {"bluray://udf%3a%2f%2f%252fsomepath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "/somepath/disc 1/movie.tbn"},
    {"bluray://%2fsomepath%2fmovie%2f/BDMV/PLAYLIST/00800.mpls", "/somepath/movie/BDMV/index.tbn"},
    {"bluray://%2fsomepath%2fmovie%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls",
     "/somepath/movie/disc 1/BDMV/index.tbn"},
    {"zip://%2fsomepath%2fmovie.zip/film.mkv", "/somepath/movie.tbn"},
    {"zip://%2fsomepath%2fmovie.zip/BDMV/index.bdmv", "/somepath/movie.tbn"},
    {"zip://%2fsomepath%2fmovie%2fdisc%201%2fmovie.zip/BDMV/index.bdmv",
     "/somepath/movie/disc 1/movie.tbn"},
    {"rar://%2fsomepath%2fmovie.rar/film.mkv", "/somepath/movie.tbn"},
    {"rar://%2fsomepath%2fmovie.rar/BDMV/index.bdmv", "/somepath/movie.tbn"},
    {"rar://%2fsomepath%2fmovie%2fdisc%201%2fmovie.rar/BDMV/index.bdmv",
     "/somepath/movie/disc 1/movie.tbn"},
    {"archive://%2fsomepath%2fmovie.tar.gz/film.mkv", "/somepath/movie.tbn"},
    {"archive://%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", "/somepath/movie.tbn"},
    {"archive://%2fsomepath%2fmovie%2fdisc%201%2fmovie.tar.gz/BDMV/index.bdmv",
     "/somepath/movie/disc 1/movie.tbn"},
    // Embedded linux path tests
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "smb://somepath/movie.tbn"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "smb://somepath/disc 1/movie.tbn"},
    {"bluray://smb%3a%2f%2fsomepath%2fmovie%2f/BDMV/PLAYLIST/00800.mpls",
     "smb://somepath/movie/BDMV/index.tbn"},
    {"bluray://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls",
     "smb://somepath/movie/disc 1/BDMV/index.tbn"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie.zip/film.mkv", "smb://somepath/movie.tbn"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie.zip/BDMV/index.bdmv", "smb://somepath/movie.tbn"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie.zip/BDMV/index.bdmv",
     "smb://somepath/movie/disc 1/movie.tbn"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie.rar/film.mkv", "smb://somepath/movie.tbn"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie.rar/BDMV/index.bdmv", "smb://somepath/movie.tbn"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie.rar/BDMV/index.bdmv",
     "smb://somepath/movie/disc 1/movie.tbn"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie.tar.gz/film.mkv", "smb://somepath/movie.tbn"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", "smb://somepath/movie.tbn"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie.tar.gz/BDMV/index.bdmv",
     "smb://somepath/movie/disc 1/movie.tbn"},
    // Embedded DOS path tests
    {"bluray://udf%3a%2f%2fD%253a%255csomepath%255cmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "D:\\somepath\\movie.tbn"},
    {"bluray://udf%3a%2f%2fD%253a%255csomepath%255cdisc%25201%255cmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "D:\\somepath\\disc 1\\movie.tbn"},
    {"bluray://D%3a%5csomepath%5cmovie%5c/BDMV/PLAYLIST/00800.mpls",
     "D:\\somepath\\movie\\BDMV\\index.tbn"},
    {"bluray://D%3a%5csomepath%5cmovie%5cdisc%201%5c/BDMV/PLAYLIST/00800.mpls",
     "D:\\somepath\\movie\\disc 1\\BDMV\\index.tbn"},
    {"zip://D%3a%5csomepath%5cmovie.zip/film.mkv", "D:\\somepath\\movie.tbn"},
    {"zip://D%3a%5csomepath%5cmovie.zip/BDMV/index.bdmv", "D:\\somepath\\movie.tbn"},
    {"zip://D%3a%5csomepath%5cmovie%5cdisc%201%5cmovie.zip/BDMV/index.bdmv",
     "D:\\somepath\\movie\\disc 1\\movie.tbn"},
    {"rar://D%3a%5csomepath%5cmovie.rar/film.mkv", "D:\\somepath\\movie.tbn"},
    {"rar://D%3a%5csomepath%5cmovie.rar/BDMV/index.bdmv", "D:\\somepath\\movie.tbn"},
    {"rar://D%3a%5csomepath%5cmovie%5cdisc%201%5cmovie.rar/BDMV/index.bdmv",
     "D:\\somepath\\movie\\disc 1\\movie.tbn"},
    {"archive://D%3a%5csomepath%5cmovie.tar.gz/film.mkv", "D:\\somepath\\movie.tbn"},
    {"archive://D%3a%5csomepath%5cmovie.tar.gz/BDMV/index.bdmv", "D:\\somepath\\movie.tbn"},
    {"archive://D%3a%5csomepath%5cmovie%5cdisc%201%5cmovie.tar.gz/BDMV/index.bdmv",
     "D:\\somepath\\movie\\disc 1\\movie.tbn"},
    // Embedded Windows server path tests
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "\\\\Server\\Movies\\movie.tbn"},
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cdisc%25201%255cmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "\\\\Server\\Movies\\disc 1\\movie.tbn"},
    {"bluray://%5c%5cServer%5cMovies%5cmovie%5c/BDMV/PLAYLIST/00800.mpls",
     "\\\\Server\\Movies\\movie\\BDMV\\index.tbn"},
    {"bluray://%5c%5cServer%5cMovies%5cmovie%5cdisc%201%5c/BDMV/PLAYLIST/00800.mpls",
     "\\\\Server\\Movies\\movie\\disc 1\\BDMV\\index.tbn"},
    {"zip://%5c%5cServer%5cMovies%5cmovie.zip/film.mkv", "\\\\Server\\Movies\\movie.tbn"},
    {"zip://%5c%5cServer%5cMovies%5cmovie.zip/BDMV/index.bdmv", "\\\\Server\\Movies\\movie.tbn"},
    {"zip://%5c%5cServer%5cMovies%5cmovie%5cdisc%201%5cmovie.zip/BDMV/index.bdmv",
     "\\\\Server\\Movies\\movie\\disc 1\\movie.tbn"},
    {"rar://%5c%5cServer%5cMovies%5cmovie.rar/film.mkv", "\\\\Server\\Movies\\movie.tbn"},
    {"rar://%5c%5cServer%5cMovies%5cmovie.rar/BDMV/index.bdmv", "\\\\Server\\Movies\\movie.tbn"},
    {"rar://%5c%5cServer%5cMovies%5cmovie%5cdisc%201%5cmovie.rar/BDMV/index.bdmv",
     "\\\\Server\\Movies\\movie\\disc 1\\movie.tbn"},
    {"archive://%5c%5cServer%5cMovies%5cmovie.tar.gz/film.mkv", "\\\\Server\\Movies\\movie.tbn"},
    {"archive://%5c%5cServer%5cMovies%5cmovie.tar.gz/BDMV/index.bdmv",
     "\\\\Server\\Movies\\movie.tbn"},
    {"archive://%5c%5cServer%5cMovies%5cmovie%5cdisc%201%5cmovie.tar.gz/BDMV/index.bdmv",
     "\\\\Server\\Movies\\movie\\disc 1\\movie.tbn"},
    // Special tests
    // Episodes
    {"/home/user/foo.avi", "/home/user/foo-S03E04.tbn", false, 3, 4},
    {"/home/user/BDMV/index.bdmv", "/home/user/BDMV/index-S03E04.tbn", false, 3, 4},
    {"smb://home/user/foo.avi", "smb://home/user/foo-S03E04.tbn", false, 3, 4},
    {"D:\\home\\user\\foo.avi", "D:\\home\\user\\foo-S03E04.tbn", false, 3, 4},
    {"\\\\Server\\user\\foo.avi", "\\\\Server\\user\\foo-S03E04.tbn", false, 3, 4},
    {"bluray://smb%3a%2f%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls",
     "smb://somepath/BDMV/index-S03E04.tbn", false, 3, 4},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252ftvshow.iso%2f/BDMV/"
     "PLAYLIST/00800.mpls",
     "smb://somepath/tvshow-S03E04.tbn", false, 3, 4}};

TEST_P(GetTbnTest, TbnTest)
{
  EXPECT_EQ(ART::GetTBNFile(CFileItem(GetParam().path, GetParam().isFolder), GetParam().season,
                            GetParam().episode),
            GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(TestArtUtils, GetTbnTest, testing::ValuesIn(tbn_tests));

// ART::GetTBNFile() stack tests

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
