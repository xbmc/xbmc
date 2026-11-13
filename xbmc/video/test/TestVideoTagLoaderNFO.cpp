/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "InfoScanner.h"
#include "NfoFile.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/AddonManager.h"
#include "addons/Scraper.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "platform/Filesystem.h"
#include "utils/ItemProperties.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"
#include "video/tags/VideoTagLoaderNFO.h"

#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <gtest/gtest.h>

namespace
{
struct ArchiveNfoTest
{
  int season;
  int episode;
  std::vector<std::string> nfoFiles; // created next to archive.rar
  std::string expected;
};

const ArchiveNfoTest archive_nfo_tests[] = {
    {3, 4, {"archive-S03E04.nfo"}, "archive-S03E04.nfo"},
    {3, 4, {"archive.nfo"}, "archive.nfo"},
    {3, 4, {"archive-S03E04.nfo", "archive.nfo"}, "archive-S03E04.nfo"},
    {3, 4, {"foo.nfo", "archive-S03E04.nfo"}, "foo.nfo"},
    {3, 4, {"archive-S03E05.nfo"}, ""},
    {-1, -1, {"archive.nfo"}, "archive.nfo"},
};
} // namespace

class ArchiveNfoTestFixture : public testing::WithParamInterface<ArchiveNfoTest>,
                              public testing::Test
{
};

TEST_P(ArchiveNfoTestFixture, FindNFO)
{
  std::error_code ec;
  const std::string tmpdir{KODI::PLATFORM::FILESYSTEM::temp_directory_path(ec)};
  ASSERT_FALSE(ec);
  std::string dir{URIUtils::AddFileToFolder(
      tmpdir, fmt::format("NfoArchiveTest{:08x}", std::random_device{}()))};
  URIUtils::AddSlashAtEnd(dir);
  ASSERT_TRUE(XFILE::CDirectory::Create(dir));
  for (const std::string& nfoFile : GetParam().nfoFiles)
    std::ofstream(URIUtils::AddFileToFolder(dir, nfoFile), std::ios::out);

  CFileItem item(
      fmt::format("rar://{}/foo.mkv", CURL::Encode(URIUtils::AddFileToFolder(dir, "archive.rar"))),
      false);
  item.GetVideoInfoTag()->m_iSeason = GetParam().season;
  item.GetVideoInfoTag()->m_iEpisode = GetParam().episode;
  const CVideoTagLoaderNFO loader(item, nullptr, false);

  EXPECT_EQ(URIUtils::GetFileName(loader.GetNFOPath()), GetParam().expected);

  XFILE::CDirectory::RemoveRecursive(dir);
}

INSTANTIATE_TEST_SUITE_P(TestVideoTagLoaderNFO,
                         ArchiveNfoTestFixture,
                         testing::ValuesIn(archive_nfo_tests));

namespace
{
struct StackNfoTest
{
  std::vector<std::string> parts; // relative to the folder Film
  std::vector<std::string> nfoFiles; // created in Film
  std::string expected;
};

const StackNfoTest stack_nfo_tests[] = {
    // File stack spanning folders, where Film's movie.nfo is not its only nfo
    {{"A/Feature part 1.mkv", "B/Feature part 2.mkv"}, {"movie.nfo", "extras.nfo"}, "movie.nfo"},
    // Folder stack with part folders named by their part alone
    {{"part 1/movie.mkv", "part 2/movie.mkv"}, {"Film.nfo", "extras.nfo"}, "Film.nfo"},
    {{"part 1/movie.mkv", "part 2/movie.mkv"}, {"movie.nfo", "extras.nfo"}, "movie.nfo"},
    {{"part 1/movie.mkv", "part 2/movie.mkv"}, {"movie.nfo", "Film.nfo"}, "movie.nfo"},
    // Folder stack with part folders named after another title than the folder holding them
    {{"Feature part 1/movie.mkv", "Feature part 2/movie.mkv"},
     {"movie.nfo", "Feature.nfo"},
     "Feature.nfo"},
};
} // namespace

class StackNfoTestFixture : public testing::WithParamInterface<StackNfoTest>, public testing::Test
{
};

TEST_P(StackNfoTestFixture, FindNFO)
{
  std::error_code ec;
  const std::string tmpdir{KODI::PLATFORM::FILESYSTEM::temp_directory_path(ec)};
  ASSERT_FALSE(ec);
  std::string dir{
      URIUtils::AddFileToFolder(tmpdir, fmt::format("NfoStackTest{:08x}", std::random_device{}()))};
  URIUtils::AddSlashAtEnd(dir);
  std::string movieDir{URIUtils::AddFileToFolder(dir, "Film")};
  URIUtils::AddSlashAtEnd(movieDir);
  ASSERT_TRUE(XFILE::CDirectory::Create(movieDir));
  for (const std::string& nfoFile : GetParam().nfoFiles)
    std::ofstream(URIUtils::AddFileToFolder(movieDir, nfoFile), std::ios::out);

  std::vector<std::string> partPaths;
  for (const std::string& part : GetParam().parts)
  {
    partPaths.emplace_back(URIUtils::AddFileToFolder(movieDir, part));
    ASSERT_TRUE(XFILE::CDirectory::Create(URIUtils::GetDirectory(partPaths.back())));
  }

  const CFileItem item("stack://" + StringUtils::Join(partPaths, " , "), false);
  const CVideoTagLoaderNFO loader(item, nullptr, true);

  EXPECT_EQ(loader.GetNFOPath(), URIUtils::AddFileToFolder(movieDir, GetParam().expected));

  XFILE::CDirectory::RemoveRecursive(dir);
}

INSTANTIATE_TEST_SUITE_P(TestVideoTagLoaderNFO,
                         StackNfoTestFixture,
                         testing::ValuesIn(stack_nfo_tests));

namespace
{
struct DiscNfoTest
{
  bool image; // a disc image FILM.iso in Film, or a folder rip of Film
  bool dynPath; // the playlist is the item's dynamic path too, as in a library item
  bool hybridFolder; // a library item of a movie with versions or extras, listed as a folder
  bool movieFolder;
  std::vector<std::string> nfoFiles; // relative to the folder Film
  std::string expected;
};

const DiscNfoTest disc_nfo_tests[] = {
    // A folder rip's movie.nfo, or the index.nfo an export writes in its BDMV folder
    {false, false, false, true, {"movie.nfo"}, "movie.nfo"},
    {false, true, false, true, {"movie.nfo"}, "movie.nfo"},
    {false, true, false, false, {"movie.nfo"}, "movie.nfo"},
    {false, true, true, true, {"movie.nfo"}, "movie.nfo"},
    {false, true, true, false, {"movie.nfo"}, "movie.nfo"},
    {false, true, false, true, {"BDMV/index.nfo"}, "BDMV/index.nfo"},
    {false, true, true, true, {"BDMV/index.nfo"}, "BDMV/index.nfo"},
    // Beside BDMV the folder can take any nfo, if it is the only one there
    {false, true, false, true, {"Film.nfo"}, "Film.nfo"},
    {false, true, false, true, {"anything.nfo"}, "anything.nfo"},
    {false, true, false, true, {"movie.nfo", "Film.nfo"}, ""},
    // A disc image's movie.nfo, or the nfo named after it
    {true, true, false, true, {"movie.nfo"}, "movie.nfo"},
    {true, true, false, false, {"FILM.nfo"}, "FILM.nfo"},
    {true, true, true, true, {"movie.nfo"}, "movie.nfo"},
    {true, true, true, false, {"FILM.nfo"}, "FILM.nfo"},
};
} // namespace

class DiscNfoTestFixture : public testing::WithParamInterface<DiscNfoTest>, public testing::Test
{
};

// A movie on a disc is refreshed from one of its playlists, whose nfo is the disc's
TEST_P(DiscNfoTestFixture, FindNFO)
{
  std::error_code ec;
  const std::string tmpdir{KODI::PLATFORM::FILESYSTEM::temp_directory_path(ec)};
  ASSERT_FALSE(ec);
  std::string dir{
      URIUtils::AddFileToFolder(tmpdir, fmt::format("NfoDiscTest{:08x}", std::random_device{}()))};
  URIUtils::AddSlashAtEnd(dir);
  std::string movieDir{URIUtils::AddFileToFolder(dir, "Film")};
  URIUtils::AddSlashAtEnd(movieDir);
  ASSERT_TRUE(XFILE::CDirectory::Create(URIUtils::AddFileToFolder(movieDir, "BDMV")));
  const std::string disc{GetParam().image
                             ? URIUtils::AddFileToFolder(movieDir, "FILM.iso")
                             : URIUtils::AddFileToFolder(movieDir, "BDMV", "index.bdmv")};
  std::ofstream(disc, std::ios::out);
  for (const std::string& nfoFile : GetParam().nfoFiles)
    std::ofstream(URIUtils::AddFileToFolder(movieDir, nfoFile), std::ios::out);

  const std::string playlist{URIUtils::GetBlurayPlaylistPath(disc, 800)};
  CFileItem item(playlist, false);
  if (GetParam().dynPath)
    item.SetDynPath(playlist);
  if (GetParam().hybridFolder)
  {
    item.SetFolder(true);
    item.SetProperty(KODI::ITEM::PROPERTY::IS_HYBRID_FOLDER, true);
  }
  const CVideoTagLoaderNFO loader(item, nullptr, GetParam().movieFolder);

  EXPECT_EQ(loader.GetNFOPath(), GetParam().expected.empty()
                                     ? ""
                                     : URIUtils::AddFileToFolder(movieDir, GetParam().expected));

  XFILE::CDirectory::RemoveRecursive(dir);
}

INSTANTIATE_TEST_SUITE_P(TestVideoTagLoaderNFO,
                         DiscNfoTestFixture,
                         testing::ValuesIn(disc_nfo_tests));

// Each movie of an nfo holding several, as an export writes a disc's versions and extras, is read
// in turn, whole and alone
TEST(TestVideoTagLoaderNFO, EachMovieOfAnNfoHoldingSeveralIsReadWhole)
{
  std::error_code ec;
  const std::string tmpdir{KODI::PLATFORM::FILESYSTEM::temp_directory_path(ec)};
  ASSERT_FALSE(ec);
  std::string dir{URIUtils::AddFileToFolder(
      tmpdir, fmt::format("NfoMoviesTest{:08x}", std::random_device{}()))};
  URIUtils::AddSlashAtEnd(dir);
  ASSERT_TRUE(XFILE::CDirectory::Create(dir));
  const std::string video{URIUtils::AddFileToFolder(dir, "Film.mkv")};
  std::ofstream(video, std::ios::out);

  constexpr int MOVIES{5};
  std::string nfo{"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>\n<movies>\n"};
  for (int movie = 1; movie <= MOVIES; ++movie)
    nfo += fmt::format("    <movie version=\"0\">\n"
                       "        <title>Film {0}</title>\n"
                       "        <playlist>{0}</playlist>\n"
                       "        <dateadded>2024-02-0{0} 05:38:04</dateadded>\n"
                       "    </movie>\n",
                       movie);
  nfo += "</movies>\n";
  std::ofstream(URIUtils::AddFileToFolder(dir, "Film.nfo"), std::ios::out | std::ios::binary)
      << nfo;

  ADDON::AddonPtr addon;
  ASSERT_TRUE(CServiceBroker::GetAddonMgr().GetAddon(
      "metadata.local", addon, ADDON::AddonType::SCRAPER_MOVIES, ADDON::OnlyEnabled::CHOICE_NO));
  CVideoTagLoaderNFO loader(CFileItem(video, false),
                            std::dynamic_pointer_cast<ADDON::CScraper>(addon), false);

  for (int movie = 1; movie <= MOVIES; ++movie)
  {
    CVideoInfoTag tag;
    ASSERT_EQ(movie == 1 ? loader.Load(tag, false) : loader.LoadVersion(movie, tag),
              CInfoScanner::InfoType::FULL);
    EXPECT_EQ(tag.m_strTitle, fmt::format("Film {}", movie));
    EXPECT_EQ(loader.GetBlurayPlaylist(), movie);
    // The last of its elements
    EXPECT_EQ(tag.m_dateAdded.GetAsDBDateTime(), fmt::format("2024-02-0{} 05:38:04", movie));
  }
  CVideoInfoTag tag;
  EXPECT_EQ(loader.LoadVersion(MOVIES + 1, tag), CInfoScanner::InfoType::NONE);

  XFILE::CDirectory::RemoveRecursive(dir);
}

// A movie's end tag inside its text or a comment ends nothing: each movie is still read whole
TEST(TestVideoTagLoaderNFO, AMoviesEndTagInItsTextOrACommentEndsNothing)
{
  std::error_code ec;
  const std::string tmpdir{KODI::PLATFORM::FILESYSTEM::temp_directory_path(ec)};
  ASSERT_FALSE(ec);
  std::string dir{URIUtils::AddFileToFolder(
      tmpdir, fmt::format("NfoEndTagTest{:08x}", std::random_device{}()))};
  URIUtils::AddSlashAtEnd(dir);
  ASSERT_TRUE(XFILE::CDirectory::Create(dir));
  const std::string video{URIUtils::AddFileToFolder(dir, "Film.mkv")};
  std::ofstream(video, std::ios::out);
  std::ofstream(URIUtils::AddFileToFolder(dir, "Film.nfo"), std::ios::out | std::ios::binary)
      << "<movies>\n"
         "  <movie>\n"
         "    <title>Film</title>\n"
         "    <plot><![CDATA[Literal </movie> marker]]></plot>\n"
         "    <dateadded>2024-02-01 05:38:04</dateadded>\n"
         "  </movie>\n"
         "  <movie>\n"
         "    <title>Version</title>\n"
         "    <!-- </movie> -->\n"
         "    <dateadded>2024-02-02 05:38:04</dateadded>\n"
         "  </movie>\n"
         "  <movie><title>Extra</title></movie>\n"
         "</movies>\n";

  ADDON::AddonPtr addon;
  ASSERT_TRUE(CServiceBroker::GetAddonMgr().GetAddon(
      "metadata.local", addon, ADDON::AddonType::SCRAPER_MOVIES, ADDON::OnlyEnabled::CHOICE_NO));
  CVideoTagLoaderNFO loader(CFileItem(video, false),
                            std::dynamic_pointer_cast<ADDON::CScraper>(addon), false);

  CVideoInfoTag film;
  ASSERT_EQ(loader.Load(film, false), CInfoScanner::InfoType::FULL);
  EXPECT_EQ("Literal </movie> marker", film.m_strPlot);
  EXPECT_EQ("2024-02-01 05:38:04", film.m_dateAdded.GetAsDBDateTime());
  CVideoInfoTag version;
  ASSERT_EQ(loader.LoadVersion(2, version), CInfoScanner::InfoType::FULL);
  EXPECT_EQ("Version", version.m_strTitle);
  EXPECT_EQ("2024-02-02 05:38:04", version.m_dateAdded.GetAsDBDateTime());

  XFILE::CDirectory::RemoveRecursive(dir);
}

// An nfo isn't read without a scraper, as when the art held in a video file is looked for, whose
// tags can name a scraper's url
TEST(TestVideoTagLoaderNFO, AnNfoIsNotReadWithoutAScraper)
{
  const std::string folder{
      CSpecialProtocol::TranslatePath("special://temp/TestVideoTagLoaderNFO/")};
  ASSERT_TRUE(XFILE::CDirectory::Create(folder));
  const std::string nfo{URIUtils::AddFileToFolder(folder, "Film.nfo")};
  std::ofstream(nfo, std::ios::out | std::ios::binary) << "<movie><title>Film</title></movie>";

  CNfoFile reader;
  EXPECT_EQ(CInfoScanner::InfoType::NONE, reader.Create(nfo, nullptr));

  XFILE::CDirectory::RemoveRecursive(folder);
}
