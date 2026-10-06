/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "MediaSource.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "XBDateTime.h"
#include "cores/VideoSettings.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "imagefiles/ImageFileURL.h"
#include "interfaces/AnnouncementManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "utils/Artwork.h"
#include "utils/StreamDetails.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "video/Bookmark.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbUrl.h"
#include "video/VideoInfoScanner.h"
#include "video/VideoInfoTag.h"
#include "video/VideoManagerTypes.h"

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
constexpr const char* DB_NAME = "TestVideoDatabase";

class TestVideoDatabase : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_settings.type = "sqlite3";
    m_settings.name = DB_NAME;
    m_settings.host = CSpecialProtocol::TranslatePath("special://temp/");
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, m_db.Connect(DB_NAME, m_settings, true));

    // The test environment has no announcement manager, and e.g. ExportToXML() announces.
    // An unstarted one only queues the announcements.
    m_previousAnnouncementManager = CServiceBroker::GetAnnouncementManager();
    CServiceBroker::RegisterAnnouncementManager(
        std::make_shared<ANNOUNCEMENT::CAnnouncementManager>());
  }

  void TearDown() override
  {
    CServiceBroker::UnregisterAnnouncementManager();
    if (m_previousAnnouncementManager)
      CServiceBroker::RegisterAnnouncementManager(m_previousAnnouncementManager);

    m_db.Close();
    XFILE::CFile::Delete(m_settings.host + DB_NAME + ".db");
  }

  // Write a play count and resume point the way playback does (AddFile via the item)
  void MarkPlayed(const std::string& path, int playCount, double resumeSeconds = 0.0)
  {
    ASSERT_TRUE(m_db.SetPlayCount(CFileItem(path, false), playCount).IsValid());
    if (resumeSeconds > 0.0)
    {
      CBookmark bookmark;
      bookmark.timeInSeconds = resumeSeconds;
      bookmark.totalTimeInSeconds = 3600.0;
      ASSERT_TRUE(m_db.AddBookMarkToFile(path, bookmark, CBookmark::RESUME));
    }
  }

  static std::string ArchivePath(const std::string& type,
                                 const std::string& archive,
                                 const std::string& file = "")
  {
    return URIUtils::CreateArchivePath(type, CURL(archive), file).Get();
  }

  static std::shared_ptr<CFileItem> AddItem(CFileItemList& items, const std::string& path)
  {
    auto item = std::make_shared<CFileItem>(path, false);
    items.Add(item);
    return item;
  }

  // Library entries, created the way the scanner does (AddFile via the tag)
  CVideoInfoTag Tag(const std::string& fileAndPath)
  {
    CVideoInfoTag tag;
    tag.m_strTitle = URIUtils::GetFileName(fileAndPath);
    tag.m_strFileNameAndPath = fileAndPath;
    tag.m_strPath = URIUtils::GetDirectory(fileAndPath);
    tag.m_basePath = CFileItem(fileAndPath, false).GetBaseMoviePath(false);
    tag.m_parentPathID = m_db.AddPath(URIUtils::GetParentPath(tag.m_basePath));
    return tag;
  }

  int AddMovie(const std::string& fileAndPath)
  {
    CVideoInfoTag tag{Tag(fileAndPath)};
    return m_db.SetDetailsForMovie(tag, KODI::ART::Artwork{});
  }

  int AddTvShow(const std::string& showPath)
  {
    CVideoInfoTag tag;
    tag.m_strTitle = "Show";
    tag.m_strPath = showPath;
    return m_db.SetDetailsForTvShow({showPath}, tag, KODI::ART::Artwork{},
                                    KODI::ART::SeasonsArtwork{});
  }

  int AddEpisode(int idShow, const std::string& fileAndPath, int episode)
  {
    CVideoInfoTag tag{Tag(fileAndPath)};
    tag.m_iSeason = 1;
    tag.m_iEpisode = episode;
    return m_db.SetDetailsForEpisode(tag, KODI::ART::Artwork{}, idShow);
  }

  DatabaseSettings m_settings;
  CVideoDatabase m_db;
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previousAnnouncementManager;
};
} // namespace

TEST_F(TestVideoDatabase, GetPlayCountsForFilesInFolder)
{
  MarkPlayed("/videos/watched.mkv", 2);
  MarkPlayed("/videos/resumable.mkv", 0, 600.0);

  CFileItemList items;
  items.SetPath("/videos/");
  const auto watched = AddItem(items, "/videos/watched.mkv");
  const auto resumable = AddItem(items, "/videos/resumable.mkv");
  const auto unknown = AddItem(items, "/videos/unknown.mkv");

  EXPECT_TRUE(m_db.GetPlayCounts(items.GetPath(), items));

  EXPECT_EQ(2, watched->GetVideoInfoTag()->GetPlayCount());
  EXPECT_FALSE(watched->GetVideoInfoTag()->GetResumePoint().IsSet());
  EXPECT_EQ(0, resumable->GetVideoInfoTag()->GetPlayCount());
  EXPECT_EQ(600.0, resumable->GetVideoInfoTag()->GetResumePoint().timeInSeconds);
  EXPECT_FALSE(unknown->HasVideoInfoTag());
}

TEST_F(TestVideoDatabase, GetPlayCountsForFolderNotInDatabase)
{
  CFileItemList items;
  items.SetPath("/videos/");
  const auto item = AddItem(items, "/videos/unknown.mkv");

  EXPECT_FALSE(m_db.GetPlayCounts(items.GetPath(), items));
  EXPECT_FALSE(item->HasVideoInfoTag());
}

// A single-file archive is collapsed to the file inside it when its folder is listed, so the
// item's records are under the archive's own path row and the folder may have no row at all.
TEST_F(TestVideoDatabase, GetPlayCountsForCollapsedArchiveItems)
{
  const std::string watched{ArchivePath("rar", "/videos/watched.rar", "watched.mkv")};
  const std::string resumable{ArchivePath("rar", "/videos/resumable.rar", "resumable.mkv")};
  MarkPlayed(watched, 1);
  MarkPlayed(resumable, 0, 900.0);
  ASSERT_LT(m_db.GetPathId("/videos/"), 0);

  CFileItemList items;
  items.SetPath("/videos/");
  const auto watchedItem = AddItem(items, watched);
  const auto resumableItem = AddItem(items, resumable);
  const auto unknownItem = AddItem(items, ArchivePath("rar", "/videos/unknown.rar", "u.mkv"));

  EXPECT_TRUE(m_db.GetPlayCounts(items.GetPath(), items));

  EXPECT_EQ(1, watchedItem->GetVideoInfoTag()->GetPlayCount());
  EXPECT_EQ(0, resumableItem->GetVideoInfoTag()->GetPlayCount());
  EXPECT_EQ(900.0, resumableItem->GetVideoInfoTag()->GetResumePoint().timeInSeconds);
  EXPECT_FALSE(unknownItem->HasVideoInfoTag());
}

TEST_F(TestVideoDatabase, GetPlayCountsOverwritesStaleArchiveItemPlayCount)
{
  const std::string path{ArchivePath("rar", "/videos/show.rar", "episode.mkv")};
  MarkPlayed(path, 0);

  CFileItemList items;
  items.SetPath("/videos/");
  const auto item = AddItem(items, path);
  item->GetVideoInfoTag()->SetPlayCount(5);

  EXPECT_TRUE(m_db.GetPlayCounts(items.GetPath(), items));
  EXPECT_EQ(0, item->GetVideoInfoTag()->GetPlayCount());
}

TEST_F(TestVideoDatabase, GetPlayCountsWhenListingInsideArchive)
{
  const std::string archive{ArchivePath("rar", "/videos/season.rar")};
  MarkPlayed(ArchivePath("rar", "/videos/season.rar", "e01.mkv"), 1);
  MarkPlayed(ArchivePath("rar", "/videos/season.rar", "e02.mkv"), 0, 300.0);

  CFileItemList items;
  items.SetPath(archive);
  const auto e01 = AddItem(items, ArchivePath("rar", "/videos/season.rar", "e01.mkv"));
  const auto e02 = AddItem(items, ArchivePath("rar", "/videos/season.rar", "e02.mkv"));
  const auto e03 = AddItem(items, ArchivePath("rar", "/videos/season.rar", "e03.mkv"));

  EXPECT_TRUE(m_db.GetPlayCounts(items.GetPath(), items));

  EXPECT_EQ(1, e01->GetVideoInfoTag()->GetPlayCount());
  EXPECT_EQ(300.0, e02->GetVideoInfoTag()->GetResumePoint().timeInSeconds);
  EXPECT_FALSE(e03->HasVideoInfoTag());
}

// zip:// (native) and archive:// (vfs addon) paths share one path row
TEST_F(TestVideoDatabase, GetPlayCountsAcrossZipAndArchiveProtocols)
{
  MarkPlayed(ArchivePath("zip", "/videos/native.zip", "native.mkv"), 1);
  MarkPlayed(ArchivePath("archive", "/videos/addon.zip", "addon.mkv"), 1);

  CFileItemList items;
  items.SetPath("/videos/");
  const auto native = AddItem(items, ArchivePath("archive", "/videos/native.zip", "native.mkv"));
  const auto addon = AddItem(items, ArchivePath("zip", "/videos/addon.zip", "addon.mkv"));

  EXPECT_TRUE(m_db.GetPlayCounts(items.GetPath(), items));

  EXPECT_EQ(1, native->GetVideoInfoTag()->GetPlayCount());
  EXPECT_EQ(1, addon->GetVideoInfoTag()->GetPlayCount());
}

TEST_F(TestVideoDatabase, GetPlayCountsForMultiPath)
{
  const std::string archived{ArchivePath("rar", "/more/archived.rar", "archived.mkv")};
  MarkPlayed("/videos/plain.mkv", 1);
  MarkPlayed(archived, 1);

  CFileItemList items;
  items.SetPath(XFILE::CMultiPathDirectory::ConstructMultiPath(
      std::vector<std::string>{"/videos/", "/more/"}));
  const auto plain = AddItem(items, "/videos/plain.mkv");
  const auto archivedItem = AddItem(items, archived);
  const auto unknown = AddItem(items, "/more/unknown.mkv");

  EXPECT_TRUE(m_db.GetPlayCounts(items.GetPath(), items));

  EXPECT_EQ(1, plain->GetVideoInfoTag()->GetPlayCount());
  EXPECT_EQ(1, archivedItem->GetVideoInfoTag()->GetPlayCount());
  EXPECT_FALSE(unknown->HasVideoInfoTag());
}

TEST_F(TestVideoDatabase, GetPlayCountsForNestedMultiPath)
{
  const std::string archived{ArchivePath("rar", "/more/archived.rar", "archived.mkv")};
  MarkPlayed("/videos/plain.mkv", 1);
  MarkPlayed("/other/other.mkv", 0, 400.0);
  MarkPlayed(archived, 1);

  const std::string inner{XFILE::CMultiPathDirectory::ConstructMultiPath(
      std::vector<std::string>{"/other/", "/more/"})};
  CFileItemList items;
  items.SetPath(
      XFILE::CMultiPathDirectory::ConstructMultiPath(std::vector<std::string>{"/videos/", inner}));
  const auto plain = AddItem(items, "/videos/plain.mkv");
  const auto other = AddItem(items, "/other/other.mkv");
  const auto archivedItem = AddItem(items, archived);

  EXPECT_TRUE(m_db.GetPlayCounts(items.GetPath(), items));

  EXPECT_EQ(1, plain->GetVideoInfoTag()->GetPlayCount());
  EXPECT_EQ(400.0, other->GetVideoInfoTag()->GetResumePoint().timeInSeconds);
  EXPECT_EQ(1, archivedItem->GetVideoInfoTag()->GetPlayCount());
}

// Items listed by a plugin are looked up by their own (playback) URL rather than the listing
// path, and a play count supplied by the plugin is kept
TEST_F(TestVideoDatabase, GetPlayCountsForPluginItems)
{
  const std::string pluginArchive{ArchivePath("zip", "/elsewhere/film.zip", "film.mkv")};
  MarkPlayed("plugin://plugin.video.test/play/?id=1", 1);
  MarkPlayed("plugin://plugin.video.test/play/?id=2", 1, 250.0);
  MarkPlayed(pluginArchive, 1, 100.0);

  for (const std::string& listing :
       {std::string{"plugin://plugin.video.test/folder/"},
        XFILE::CMultiPathDirectory::ConstructMultiPath(
            std::vector<std::string>{"/videos/", "plugin://plugin.video.test/folder/"})})
  {
    CFileItemList items;
    items.SetPath(listing);
    const auto first = AddItem(items, "plugin://plugin.video.test/play/?id=1");
    const auto second = AddItem(items, "plugin://plugin.video.test/play/?id=2");
    const auto notPlayable = AddItem(items, "plugin://plugin.video.test/play/?id=1");
    const auto notPlayableArchive = AddItem(items, pluginArchive);
    first->SetProperty("IsPlayable", true);
    second->SetProperty("IsPlayable", true);
    second->GetVideoInfoTag()->SetPlayCount(3);
    notPlayableArchive->GetVideoInfoTag()->SetPlayCount(3);

    EXPECT_TRUE(m_db.GetPlayCounts(items.GetPath(), items)) << listing;

    EXPECT_EQ(1, first->GetVideoInfoTag()->GetPlayCount()) << listing;
    EXPECT_EQ(3, second->GetVideoInfoTag()->GetPlayCount()) << listing;
    EXPECT_EQ(250.0, second->GetVideoInfoTag()->GetResumePoint().timeInSeconds) << listing;
    EXPECT_FALSE(notPlayable->HasVideoInfoTag()) << listing;
    // Only playable items are eligible, even if the plugin lists archive URLs
    EXPECT_EQ(3, notPlayableArchive->GetVideoInfoTag()->GetPlayCount()) << listing;
    EXPECT_FALSE(notPlayableArchive->GetVideoInfoTag()->GetResumePoint().IsSet()) << listing;
  }
}

// A collapsed archive from a filesystem folder in the multipath is still recognised as such
TEST_F(TestVideoDatabase, GetPlayCountsForLocalArchiveInMultiPathWithPlugin)
{
  const std::string localArchive{ArchivePath("zip", "/videos/local.zip", "local.mkv")};
  MarkPlayed(localArchive, 1, 100.0);

  CFileItemList items;
  items.SetPath(XFILE::CMultiPathDirectory::ConstructMultiPath(
      std::vector<std::string>{"/videos/", "plugin://plugin.video.test/folder/"}));
  const auto item = AddItem(items, localArchive);

  EXPECT_TRUE(m_db.GetPlayCounts(items.GetPath(), items));
  EXPECT_EQ(1, item->GetVideoInfoTag()->GetPlayCount());
  EXPECT_EQ(100.0, item->GetVideoInfoTag()->GetResumePoint().timeInSeconds);
}

TEST_F(TestVideoDatabase, InsertStoresOrientationAndCenterMixLevel)
{
  const int idFile = m_db.AddFile("/videos/rotated.mkv");
  ASSERT_GT(idFile, 0);

  CVideoSettings rotated;
  rotated.m_Orientation = 90;
  rotated.m_CenterMixLevel = 3;
  m_db.SetVideoSettings(idFile, rotated);

  CVideoSettings stored;
  ASSERT_TRUE(m_db.GetVideoSettings(idFile, stored));
  EXPECT_EQ(90, stored.m_Orientation);
  EXPECT_EQ(3, stored.m_CenterMixLevel);
}

TEST_F(TestVideoDatabase, UpdateStoresOrientationAndCenterMixLevel)
{
  const int idFile = m_db.AddFile("/videos/rotated.mkv");
  ASSERT_GT(idFile, 0);

  m_db.SetVideoSettings(idFile, CVideoSettings());

  CVideoSettings rotated;
  rotated.m_Orientation = 90;
  rotated.m_CenterMixLevel = 3;
  m_db.SetVideoSettings(idFile, rotated);

  CVideoSettings stored;
  ASSERT_TRUE(m_db.GetVideoSettings(idFile, stored));
  EXPECT_EQ(90, stored.m_Orientation);
  EXPECT_EQ(3, stored.m_CenterMixLevel);
}

// zip:// (native) and archive:// (vfs addon) special case

TEST_F(TestVideoDatabase, AddPathReusesRowAcrossZipAndArchiveProtocols)
{
  const std::string zip{ArchivePath("zip", "/movies/film.zip")};
  const std::string archive{ArchivePath("archive", "/movies/film.zip")};
  const int idZip{m_db.AddPath(zip)};
  ASSERT_GT(idZip, 0);
  EXPECT_EQ(idZip, m_db.AddPath(archive));
  EXPECT_EQ(idZip, m_db.GetArchiveOrAliasPathId(archive));
  EXPECT_EQ(idZip, m_db.GetArchiveOrAliasPathId(zip));
  EXPECT_LT(m_db.GetPathId(archive), 0);
}

TEST_F(TestVideoDatabase, AMovieIsFoundByItsDirectorsName)
{
  CVideoInfoTag directed{Tag("/videos/directed.mkv")};
  directed.SetDirector({"Jane Director"});
  const int idMovie{m_db.SetDetailsForMovie(directed, KODI::ART::Artwork{})};
  ASSERT_GT(idMovie, 0);
  ASSERT_GT(AddMovie("/videos/undirected.mkv"), 0);

  CVideoDbUrl url;
  ASSERT_TRUE(url.FromString("videodb://movies/titles/"));
  url.AddOption("director", "Jane Director");

  CFileItemList items;
  ASSERT_TRUE(m_db.GetMoviesByWhere(url.ToString(), CDatabase::Filter(), items));
  ASSERT_EQ(1, items.Size());
  EXPECT_EQ(idMovie, items[0]->GetVideoInfoTag()->m_iDbId);
}

TEST_F(TestVideoDatabase, GetPlayCountsListingInsideArchiveAcrossZipAndArchiveProtocols)
{
  MarkPlayed(ArchivePath("zip", "/tv/season.zip", "e01.mkv"), 1);
  MarkPlayed(ArchivePath("archive", "/tv/season.zip", "e02.mkv"), 0, 300.0);

  for (const std::string protocol : {"zip", "archive"})
  {
    CFileItemList items;
    items.SetPath(ArchivePath(protocol, "/tv/season.zip"));
    const auto e01 = AddItem(items, ArchivePath(protocol, "/tv/season.zip", "e01.mkv"));
    const auto e02 = AddItem(items, ArchivePath(protocol, "/tv/season.zip", "e02.mkv"));

    EXPECT_TRUE(m_db.GetPlayCounts(items.GetPath(), items)) << protocol;
    EXPECT_EQ(1, e01->GetVideoInfoTag()->GetPlayCount()) << protocol;
    EXPECT_EQ(300.0, e02->GetVideoInfoTag()->GetResumePoint().timeInSeconds) << protocol;
  }
}

// In Videos -> Files, CGUIWindowVideoBase::LoadVideoInfo() only falls back to GetPlayCounts()
// for items without a library match. Inside a source the flags come from GetItemsForPath()
// instead, so archived items must be found from their folder there too.

TEST_F(TestVideoDatabase, GetItemsForPathReturnsArchivedEpisodesWithCollapsedPaths)
{
  const int idShow{AddTvShow("/tv/Show/")};
  ASSERT_GT(idShow, 0);
  const std::string e01{"/tv/Show/Season 1/e01.mkv"};
  const std::string e02{ArchivePath("rar", "/tv/Show/Season 1/e02.rar", "e02.mkv")};
  const std::string e03{ArchivePath("rar", "/tv/Show/Season 1/e03.rar", "e03.mkv")};
  ASSERT_GT(AddEpisode(idShow, e01, 1), 0);
  ASSERT_GT(AddEpisode(idShow, e02, 2), 0);
  ASSERT_GT(AddEpisode(idShow, e03, 3), 0);
  MarkPlayed(e01, 1);
  MarkPlayed(e02, 1);
  MarkPlayed(e03, 0, 700.0);

  CFileItemList items;
  ASSERT_TRUE(m_db.GetItemsForPath("episodes", "/tv/Show/Season 1/", items));
  ASSERT_EQ(3, items.Size());
  items.SetFastLookup(true);

  // Paths are those of the items a Files listing of the folder contains
  const auto e01Item = items.Get(e01);
  const auto e02Item = items.Get(e02);
  const auto e03Item = items.Get(e03);
  ASSERT_TRUE(e01Item && e02Item && e03Item);
  EXPECT_EQ(1, e01Item->GetVideoInfoTag()->GetPlayCount());
  EXPECT_EQ(1, e02Item->GetVideoInfoTag()->GetPlayCount());
  EXPECT_EQ(0, e03Item->GetVideoInfoTag()->GetPlayCount());
  EXPECT_EQ(700.0, e03Item->GetVideoInfoTag()->GetResumePoint().timeInSeconds);
}

TEST_F(TestVideoDatabase, GetItemsForPathReturnsArchivedMoviesWithCollapsedPaths)
{
  const std::string plain{"/movies/Plain (2020)/plain.mkv"};
  const std::string archived{
      ArchivePath("rar", "/movies/Archived (2021)/archived.rar", "archived.mkv")};
  ASSERT_GT(AddMovie(plain), 0);
  ASSERT_GT(AddMovie(archived), 0);
  MarkPlayed(plain, 1);
  MarkPlayed(archived, 0, 1200.0);

  CFileItemList plainItems;
  ASSERT_TRUE(m_db.GetItemsForPath("movies", "/movies/Plain (2020)/", plainItems));
  ASSERT_EQ(1, plainItems.Size());
  EXPECT_EQ(plain, plainItems[0]->GetPath());
  EXPECT_EQ(1, plainItems[0]->GetVideoInfoTag()->GetPlayCount());

  CFileItemList archivedItems;
  ASSERT_TRUE(m_db.GetItemsForPath("movies", "/movies/Archived (2021)/", archivedItems));
  ASSERT_EQ(1, archivedItems.Size());
  EXPECT_EQ(archived, archivedItems[0]->GetPath());
  EXPECT_EQ(0, archivedItems[0]->GetVideoInfoTag()->GetPlayCount());
  EXPECT_EQ(1200.0, archivedItems[0]->GetVideoInfoTag()->GetResumePoint().timeInSeconds);
}

TEST_F(TestVideoDatabase, ToStoredPathAddsTheTrailingSeparator)
{
  EXPECT_EQ("smb://server/movies/", CVideoDatabase::ToStoredPath("smb://server/movies"));
  EXPECT_EQ("smb://server/movies/", CVideoDatabase::ToStoredPath("smb://server/movies/"));
}

TEST_F(TestVideoDatabase, GetPathsForCleaningMatchesADirectoryGivenWithoutItsSeparator)
{
  const int idSource{m_db.AddPath("smb://server/movies/")};
  const int idFilm{m_db.AddPath("smb://server/movies/film/", "smb://server/movies/")};
  ASSERT_GT(idSource, 0);
  ASSERT_GT(idFilm, 0);

  // no content named, so the path's own content, here none, does not matter
  std::set<int> paths;
  ASSERT_TRUE(m_db.GetPathsForCleaning("smb://server/movies", "", paths));
  EXPECT_EQ((std::set<int>{idSource, idFilm}), paths);
}

TEST_F(TestVideoDatabase, GetPathsForCleaningIncludesDiscPaths)
{
  ASSERT_GT(m_db.AddPath("/movies/"), 0);

  const std::string disc{"/movies/Film/BDMV/index.bdmv"};
  const std::string playlist{URIUtils::GetBlurayPlaylistPath(disc, 800)};
  ASSERT_GT(m_db.AddFile(disc), 0);
  ASSERT_GT(m_db.AddFile(playlist), 0);
  const int idDisc{m_db.GetPathId(URIUtils::GetDirectory(disc))};
  const int idPlaylist{m_db.GetPathId(URIUtils::GetDirectory(playlist))};
  ASSERT_GT(idDisc, 0);
  ASSERT_GT(idPlaylist, 0);

  std::set<int> paths;
  ASSERT_TRUE(m_db.GetPathsForCleaning("/movies", "", paths));
  EXPECT_TRUE(paths.contains(idDisc));
  EXPECT_TRUE(paths.contains(idPlaylist));
}

TEST_F(TestVideoDatabase, GetPathsForCleaningResolvesNothingForAnUnknownDirectory)
{
  ASSERT_GT(m_db.AddPath("smb://server/movies/"), 0);

  std::set<int> paths;
  ASSERT_TRUE(m_db.GetPathsForCleaning("smb://server/shows", "", paths));
  EXPECT_TRUE(paths.empty());
}

TEST_F(TestVideoDatabase, GetPathsForCleaningMatchesTheWholeLibraryByContentExactly)
{
  ASSERT_GT(m_db.AddPath("smb://server/movies/"), 0);

  // a path with no scraper has no content, so a clean for movies leaves it alone
  std::set<int> paths;
  ASSERT_TRUE(m_db.GetPathsForCleaning("", "movies", paths));
  EXPECT_TRUE(paths.empty());
}

// The export must write the stored (scraped) runtime, not the stream duration that loading the
// streamdetails puts in its place
TEST_F(TestVideoDatabase, ExportToXMLWritesStoredRuntime)
{
  // 1289s rounds to 21 minutes, the stored 1320s is 22
  const auto withRuntime = [](CVideoInfoTag tag)
  {
    tag.SetDuration(1320);
    auto* video = new CStreamDetailVideo();
    video->m_iDuration = 1289;
    video->SetSource(CStreamDetail::MEDIA);
    tag.m_streamDetails.AddStream(video);
    tag.m_streamDetails.DetermineBestStreams();
    return tag;
  };

  CVideoInfoTag movie{withRuntime(Tag("/movies/Movie (2020)/movie.mkv"))};
  ASSERT_GT(m_db.SetDetailsForMovie(movie, KODI::ART::Artwork{}), 0);

  const int idShow{AddTvShow("/tvshows/Show/")};
  ASSERT_GT(idShow, 0);
  CVideoInfoTag episode{withRuntime(Tag("/tvshows/Show/s01e01.mkv"))};
  episode.m_iSeason = 1;
  episode.m_iEpisode = 1;
  ASSERT_GT(m_db.SetDetailsForEpisode(episode, KODI::ART::Artwork{}, idShow), 0);

  const std::string exportPath{CSpecialProtocol::TranslatePath("special://temp/")};
  const std::string exportRoot{URIUtils::AddFileToFolder(
      exportPath, "kodi_videodb_" + CDateTime::GetCurrentDateTime().GetAsDBDate())};
  m_db.ExportToXML(exportPath, true);

  CXBMCTinyXML doc;
  const bool loaded{doc.LoadFile(URIUtils::AddFileToFolder(exportRoot, "videodb.xml"))};
  XFILE::CDirectory::RemoveRecursive(exportRoot);
  ASSERT_TRUE(loaded);

  const TiXmlElement* exportedMovie{doc.RootElement()->FirstChildElement("movie")};
  ASSERT_NE(nullptr, exportedMovie);
  int runtime{0};
  EXPECT_TRUE(XMLUtils::GetInt(exportedMovie, "runtime", runtime));
  EXPECT_EQ(22, runtime);

  const TiXmlElement* exportedShow{doc.RootElement()->FirstChildElement("tvshow")};
  ASSERT_NE(nullptr, exportedShow);
  const TiXmlElement* exportedEpisode{exportedShow->FirstChildElement("episodedetails")};
  ASSERT_NE(nullptr, exportedEpisode);
  runtime = 0;
  EXPECT_TRUE(XMLUtils::GetInt(exportedEpisode, "runtime", runtime));
  EXPECT_EQ(22, runtime);
}

// The converted movie's file is kept as the version, so its streamdetails must be kept too
TEST_F(TestVideoDatabase, ConvertVideoToVersionKeepsStreamDetails)
{
  const auto addMovie = [this](const std::string& fileAndPath, int duration)
  {
    CVideoInfoTag tag{Tag(fileAndPath)};
    auto* video = new CStreamDetailVideo();
    video->m_iDuration = duration;
    video->SetSource(CStreamDetail::MEDIA);
    tag.m_streamDetails.AddStream(video);
    tag.m_streamDetails.DetermineBestStreams();
    return m_db.SetDetailsForMovie(tag, KODI::ART::Artwork{});
  };

  const std::string source{"/movies/Movie (2010)/Movie (2010) Standard Edition.mkv"};
  const int targetId{addMovie("/movies/Movie (2010)/Movie (2010) Extended Edition.mkv", 6500)};
  const int sourceId{addMovie(source, 6019)};
  ASSERT_GT(targetId, 0);
  ASSERT_GT(sourceId, 0);

  ASSERT_TRUE(m_db.ConvertVideoToVersion(VideoDbContentType::MOVIES, sourceId, targetId, -1,
                                         VideoAssetType::VERSION));

  CStreamDetails details;
  EXPECT_TRUE(m_db.GetStreamDetails(source, details));
  EXPECT_EQ(6019, details.GetVideoDuration());
}

// A failed refresh relies on this to remove the assets of the movie it has already deleted
TEST_F(TestVideoDatabase, DeleteMovieRemovesTheAssetsOfADeletedMovie)
{
  const int idMovie{AddMovie("/movies/Movie (2010)/Movie (2010).mkv")};
  ASSERT_GT(idMovie, 0);

  const std::string version{"/movies/Movie (2010)/Movie (2010) Extended Edition.mkv"};
  CFileItem item{version, false};
  const int idType{
      m_db.AddVideoVersionType("Extended", VideoAssetTypeOwner::USER, VideoAssetType::VERSION)};
  ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, idType,
                                 VideoAssetType::VERSION, item));

  ASSERT_TRUE(m_db.DeleteMovie(idMovie, DeleteMovieCascadeAction::DEFAULT_VERSION));
  ASSERT_EQ(idMovie, m_db.GetMovieId(version));

  ASSERT_TRUE(m_db.DeleteMovie(idMovie, DeleteMovieCascadeAction::ALL_ASSETS));
  EXPECT_EQ(-1, m_db.GetMovieId(version));
}

// The clean finds the source of a movie through its parent path, which must follow the default
// version into another source
TEST_F(TestVideoDatabase, SetDefaultVideoVersionMovesTheParentPath)
{
  const int idMovie{AddMovie("/movies/Movie (2010)/Movie (2010).mkv")};
  ASSERT_GT(idMovie, 0);

  CFileItem item{"/more movies/Movie (2010)/Movie (2010) Extended Edition.mkv", false};
  const int idType{
      m_db.AddVideoVersionType("Extended", VideoAssetTypeOwner::USER, VideoAssetType::VERSION)};
  ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, idType,
                                 VideoAssetType::VERSION, item));
  const int idFile{m_db.AddFile(item.GetPath())};
  ASSERT_GT(idFile, 0);

  ASSERT_TRUE(m_db.SetDefaultVideoVersion(VideoDbContentType::MOVIES, idMovie, idFile));

  CFileItemList newParent;
  EXPECT_TRUE(m_db.GetItemsForPath("movies", "/more movies/", newParent));
  CFileItemList oldParent;
  EXPECT_FALSE(m_db.GetItemsForPath("movies", "/movies/Movie (2010)/", oldParent));
}

TEST_F(TestVideoDatabase, SetDefaultVideoVersionKeepsTheArtTheVersionLacks)
{
  const std::string original{"/movies/Movie (2010)/Movie (2010).mkv"};
  const int idMovie{AddMovie(original)};
  ASSERT_GT(idMovie, 0);
  const KODI::ART::Artwork movieArt{{"fanart", "movie-fanart"},
                                    {"poster", "movie-poster"},
                                    {"thumb", IMAGE_FILES::URLFromFile(original, "video")}};
  ASSERT_TRUE(m_db.SetArtForItem(idMovie, MediaTypeMovie, movieArt));

  const std::string extended{"/movies/Movie (2010)/Movie (2010) Extended Edition.mkv"};
  // A frame the version chose for itself, here of a chapter
  IMAGE_FILES::CImageFileURL chapterFrame{IMAGE_FILES::CImageFileURL::FromFile(extended, "video")};
  chapterFrame.AddOption("chapter", "5");
  CFileItem item{extended, false};
  item.SetArt({{"poster", "version-poster"}, {"thumb", chapterFrame.ToString()}});
  const int idType{
      m_db.AddVideoVersionType("Extended", VideoAssetTypeOwner::USER, VideoAssetType::VERSION)};
  ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, idType,
                                 VideoAssetType::VERSION, item));
  const int idOriginal{m_db.GetFileIdByMovie(idMovie)};
  const int idExtended{m_db.AddFile(extended)};

  ASSERT_TRUE(m_db.SetDefaultVideoVersion(VideoDbContentType::MOVIES, idMovie, idExtended));

  KODI::ART::Artwork art;
  ASSERT_TRUE(m_db.GetArtForItem(idMovie, MediaTypeMovie, art));
  EXPECT_EQ((KODI::ART::Artwork{{"fanart", "movie-fanart"},
                                {"poster", "version-poster"},
                                {"thumb", chapterFrame.ToString()}}),
            art);

  KODI::ART::Artwork originalArt;
  ASSERT_TRUE(m_db.GetArtForItem(idOriginal, MediaTypeVideoVersion, originalArt));
  EXPECT_EQ(movieArt, originalArt);
}

// No frame can be taken from a disc, so one of the old default's file isn't moved onto it
TEST_F(TestVideoDatabase, SetDefaultVideoVersionDropsAFrameNoneCanBeTakenFor)
{
  const std::string original{"/movies/Movie (2010)/Movie (2010).mkv"};
  const int idMovie{AddMovie(original)};
  ASSERT_GT(idMovie, 0);
  ASSERT_TRUE(m_db.SetArtForItem(
      idMovie, MediaTypeMovie,
      {{"fanart", "movie-fanart"}, {"thumb", IMAGE_FILES::URLFromFile(original, "video")}}));

  const std::string playlist{
      URIUtils::GetBlurayPlaylistPath("/movies/Movie (2010)/BDMV/index.bdmv", 800)};
  CFileItem item{playlist, false};
  const int idType{
      m_db.AddVideoVersionType("Extended", VideoAssetTypeOwner::USER, VideoAssetType::VERSION)};
  ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, idType,
                                 VideoAssetType::VERSION, item));

  ASSERT_TRUE(
      m_db.SetDefaultVideoVersion(VideoDbContentType::MOVIES, idMovie, m_db.AddFile(playlist)));

  KODI::ART::Artwork art;
  ASSERT_TRUE(m_db.GetArtForItem(idMovie, MediaTypeMovie, art));
  EXPECT_EQ((KODI::ART::Artwork{{"fanart", "movie-fanart"}}), art);
}

TEST_F(TestVideoDatabase, GetSubPathsLeavesOutDiscStructuresWhateverTheirCase)
{
  ASSERT_GT(m_db.AddPath("/movies/"), 0);
  const int idFilm{m_db.AddPath("/movies/Film/")};
  ASSERT_GT(idFilm, 0);

  std::set<int> discs;
  for (const char* file : {"/movies/A/VIDEO_TS/VIDEO_TS.IFO", "/movies/B/VIDEO_TS/video_ts.ifo",
                           "/movies/C/BDMV/index.bdmv", "/movies/D/BDMV/INDEX.BDMV"})
  {
    ASSERT_GT(m_db.AddFile(file), 0);
    discs.insert(m_db.GetPathId(URIUtils::GetDirectory(file)));
  }

  std::vector<std::pair<int, std::string>> subPaths;
  ASSERT_TRUE(m_db.GetSubPaths("/movies/", subPaths));
  std::set<int> ids;
  for (const auto& [idPath, path] : subPaths)
    ids.insert(idPath);
  EXPECT_TRUE(ids.contains(idFilm));
  for (const int idDisc : discs)
    EXPECT_FALSE(ids.contains(idDisc));
}

TEST_F(TestVideoDatabase, GetSubPathsIncludesArchivesAndDiscImagesOnlyForCleaning)
{
  ASSERT_GT(m_db.AddPath("/movies/"), 0);

  const std::string archived{ArchivePath("zip", "/movies/Film.zip", "Film.mkv")};
  const std::string playlist{URIUtils::GetBlurayPlaylistPath("/movies/Other.iso", 800)};
  std::set<int> encoded;
  for (const std::string& file : {archived, playlist})
  {
    ASSERT_GT(m_db.AddFile(file), 0);
    encoded.insert(m_db.GetPathId(URIUtils::GetDirectory(file)));
  }

  const auto subPathIds = [this](bool excludeDiscPaths)
  {
    std::vector<std::pair<int, std::string>> subPaths;
    EXPECT_TRUE(m_db.GetSubPaths("/movies/", subPaths, excludeDiscPaths));
    std::set<int> ids;
    for (const auto& [idPath, path] : subPaths)
      ids.insert(idPath);
    return ids;
  };

  const std::set<int> forCleaning{subPathIds(false)};
  const std::set<int> forScanning{subPathIds(true)};
  for (const int idPath : encoded)
  {
    EXPECT_TRUE(forCleaning.contains(idPath));
    EXPECT_FALSE(forScanning.contains(idPath));
  }
}

TEST_F(TestVideoDatabase, GetSourcePathReadsTheSettingsOfTheSourceOfASubFolder)
{
  const int idSource{m_db.AddPath("/movies/")};
  ASSERT_GT(idSource, 0);
  ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
      "UPDATE path SET strContent='movies', strScraper='metadata.local', useFolderNames=1, "
      "scanRecursive=0 WHERE idPath=%i",
      idSource)));
  ASSERT_GT(m_db.AddPath("/movies/Film/"), 0);

  std::string sourcePath;
  KODI::VIDEO::SScanSettings settings;
  ASSERT_TRUE(m_db.GetSourcePath("/movies/Film/", sourcePath, settings));
  EXPECT_EQ("/movies/", sourcePath);
  EXPECT_TRUE(settings.parent_name);
  EXPECT_EQ(0, settings.recurse);
}

TEST_F(TestVideoDatabase, RemoveContentForPathRemovesDiscRips)
{
  ASSERT_GT(m_db.AddPath("/movies/"), 0);

  const std::string folderRip{"/movies/Film/BDMV/index.bdmv"};
  const std::string playlist{URIUtils::GetBlurayPlaylistPath("/movies/Other/BDMV/index.bdmv", 800)};
  ASSERT_GT(AddMovie(folderRip), 0);
  ASSERT_GT(AddMovie(playlist), 0);

  m_db.RemoveContentForPath("/movies/");

  EXPECT_EQ(-1, m_db.GetMovieId(folderRip));
  EXPECT_EQ(-1, m_db.GetMovieId(playlist));
}

namespace
{
// A silent library clean of real files, below a temporary folder
class TestVideoDatabaseClean : public TestVideoDatabase
{
protected:
  void SetUp() override
  {
    TestVideoDatabase::SetUp();
    m_root = CSpecialProtocol::TranslatePath("special://temp/TestVideoDatabaseClean/");
    XFILE::CDirectory::RemoveRecursive(m_root);
    ASSERT_TRUE(XFILE::CDirectory::Create(m_root));
  }

  void TearDown() override
  {
    std::erase_if(*CMediaSourceSettings::GetInstance().GetSources("video"),
                  [this](const CMediaSource& source)
                  { return source.strPath.starts_with(m_root); });
    XFILE::CDirectory::RemoveRecursive(m_root);
    TestVideoDatabase::TearDown();
  }

  static std::string Folder(const std::string& parent, const std::string& name, bool onDisk = true)
  {
    std::string folder{URIUtils::AddFileToFolder(parent, name)};
    URIUtils::AddSlashAtEnd(folder);
    if (onDisk)
    {
      EXPECT_TRUE(XFILE::CDirectory::Create(folder));
    }
    return folder;
  }

  static std::string File(const std::string& folder, const std::string& name)
  {
    const std::string path{URIUtils::AddFileToFolder(folder, name)};
    XFILE::CFile file;
    EXPECT_TRUE(file.OpenForWrite(path, true));
    file.Close();
    return path;
  }

  // The clean finds the video source of a path among the sources, and its library source by the
  // content set on it
  void AddSource(const std::string& path, const std::string& content)
  {
    CMediaSource source;
    source.FromNameAndPaths(path, {path});
    CMediaSourceSettings::GetInstance().GetSources("video")->push_back(source);

    ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
        "UPDATE path SET strContent='%s', strScraper='metadata.local' WHERE idPath=%i",
        content.c_str(), m_db.AddPath(path))));
  }

  int AddAsset(int idMovie,
               const std::string& path,
               VideoAssetType type = VideoAssetType::VERSION,
               const KODI::ART::Artwork& art = {})
  {
    CFileItem item{path, false};
    item.SetArt(art);
    const int idType{m_db.AddVideoVersionType(
        type == VideoAssetType::VERSION ? "Extended" : "Trailer", VideoAssetTypeOwner::USER, type)};
    if (!m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, idType, type, item))
      return -1;
    return m_db.AddFile(path);
  }

  std::string PathHash(const std::string& path)
  {
    std::string hash;
    m_db.GetPathHash(path, hash);
    return hash;
  }

  void Clean() { m_db.CleanDatabase(nullptr, {}, false); }

  std::string m_root;
};
} // namespace

TEST_F(TestVideoDatabaseClean, PromotesAVersionWhenTheDefaultHasGone)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string film{Folder(movies, "Film (2010)")};
  const std::string gone{File(film, "Film (2010).mkv")};
  const int idMovie{AddMovie(gone)};
  ASSERT_GT(idMovie, 0);
  const int idVersion{AddAsset(idMovie, File(film, "Film (2010) Extended.mkv"))};
  ASSERT_GT(idVersion, 0);

  ASSERT_TRUE(XFILE::CFile::Delete(gone));
  Clean();

  EXPECT_EQ(idVersion, m_db.GetFileIdByMovie(idMovie));
}

TEST_F(TestVideoDatabaseClean, RemovesTheExtrasOfAMovieThatHasGone)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string film{Folder(movies, "Film (2010)")};
  const std::string gone{File(film, "Film (2010).mkv")};
  const int idMovie{AddMovie(gone)};
  ASSERT_GT(idMovie, 0);
  const int idExtra{AddAsset(idMovie, File(film, "Trailer.mkv"), VideoAssetType::EXTRA)};
  ASSERT_GT(idExtra, 0);

  ASSERT_TRUE(XFILE::CFile::Delete(gone));
  Clean();

  EXPECT_EQ(-1, m_db.GetFileIdByMovie(idMovie));
  EXPECT_EQ(0, m_db.GetSingleValueInt(
                   m_db.PrepareSQL("SELECT COUNT(*) FROM files WHERE idFile=%i", idExtra)));
}

TEST_F(TestVideoDatabaseClean, ClearsTheHashOfEveryFolderFilesWentFrom)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");

  std::vector<std::string> folders;
  for (const char* name : {"A", "B"})
  {
    const std::string folder{Folder(movies, name)};
    ASSERT_GT(AddMovie(File(folder, "Kept.mkv")), 0);
    const std::string gone{File(folder, "Gone.mkv")};
    ASSERT_GT(AddMovie(gone), 0);
    ASSERT_TRUE(XFILE::CFile::Delete(gone));
    ASSERT_TRUE(m_db.SetPathHash(folder, "hash"));
    folders.push_back(folder);
  }

  Clean();

  for (const auto& folder : folders)
    EXPECT_EQ("", PathHash(folder));
}

TEST_F(TestVideoDatabaseClean, KeepsThePathsOfAnUnavailableSource)
{
  const std::string offline{Folder(m_root, "offline", false)};
  AddSource(offline, "movies");
  const std::string film{
      URIUtils::AddFileToFolder(Folder(offline, "Film (2010)", false), "Film (2010).mkv")};
  ASSERT_GT(AddMovie(film), 0);
  // A path further down than the source's own folders, which carry the decision for its media
  const std::string other{Folder(Folder(offline, "Other", false), "Deeper", false)};
  ASSERT_TRUE(m_db.SetPathHash(other, "hash"));

  Clean();

  EXPECT_GT(m_db.GetMovieId(film), 0);
  EXPECT_EQ("hash", PathHash(other));
}

TEST_F(TestVideoDatabaseClean, ClearsTheHashesOfEveryFolderOfAShowThatLosesEpisodes)
{
  const std::string shows{Folder(m_root, "shows")};
  AddSource(shows, "tvshows");
  const std::string first{Folder(shows, "Show")};
  const std::string second{Folder(shows, "Show (more)")};

  CVideoInfoTag tag;
  tag.m_strTitle = "Show";
  tag.m_strPath = first;
  const int idShow{m_db.SetDetailsForTvShow({first, second}, tag, KODI::ART::Artwork{},
                                            KODI::ART::SeasonsArtwork{})};
  ASSERT_GT(idShow, 0);
  const std::string gone{File(first, "Show S01E01.mkv")};
  ASSERT_GT(AddEpisode(idShow, gone, 1), 0);
  ASSERT_GT(AddEpisode(idShow, File(second, "Show S01E02.mkv"), 2), 0);
  ASSERT_TRUE(m_db.SetPathHash(first, "first"));
  ASSERT_TRUE(m_db.SetPathHash(second, "second"));

  ASSERT_TRUE(XFILE::CFile::Delete(gone));
  Clean();

  EXPECT_EQ("", PathHash(first));
  EXPECT_EQ("", PathHash(second));
}

TEST_F(TestVideoDatabaseClean, RemovesTheLinkToAShowFolderThatHasGone)
{
  const std::string shows{Folder(m_root, "shows")};
  AddSource(shows, "tvshows");
  const std::string current{Folder(shows, "Show")};
  const std::string old{Folder(shows, "Show (old)", false)};

  CVideoInfoTag tag;
  tag.m_strTitle = "Show";
  tag.m_strPath = current;
  const int idShow{m_db.SetDetailsForTvShow({current, old}, tag, KODI::ART::Artwork{},
                                            KODI::ART::SeasonsArtwork{})};
  ASSERT_GT(idShow, 0);
  ASSERT_GT(AddEpisode(idShow, File(current, "Show S01E01.mkv"), 1), 0);

  Clean();

  std::vector<std::string> links;
  ASSERT_TRUE(m_db.GetPathsLinkedToTvShow(idShow, links));
  EXPECT_EQ(std::vector<std::string>{current}, links);
}

TEST_F(TestVideoDatabaseClean, PromotingAVersionKeepsTheArtOfTheMovie)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string film{Folder(movies, "Film (2010)")};
  const std::string gone{File(film, "Film (2010).mkv")};
  const int idMovie{AddMovie(gone)};
  ASSERT_GT(idMovie, 0);
  ASSERT_TRUE(m_db.SetArtForItem(idMovie, MediaTypeMovie,
                                 {{"clearlogo", IMAGE_FILES::URLFromFile(gone, "video_clearlogo")},
                                  {"fanart", "movie-fanart"},
                                  {"poster", "movie-poster"},
                                  {"thumb", IMAGE_FILES::URLFromFile(gone, "video")}}));
  const std::string version{File(film, "Film (2010) Extended.mkv")};
  ASSERT_GT(AddAsset(idMovie, version, VideoAssetType::VERSION, {{"poster", "version-poster"}}), 0);

  ASSERT_TRUE(XFILE::CFile::Delete(gone));
  Clean();

  KODI::ART::Artwork art;
  ASSERT_TRUE(m_db.GetArtForItem(idMovie, MediaTypeMovie, art));
  EXPECT_EQ((KODI::ART::Artwork{{"fanart", "movie-fanart"},
                                {"poster", "version-poster"},
                                {"thumb", IMAGE_FILES::URLFromFile(version, "video")}}),
            art);
}

TEST_F(TestVideoDatabaseClean, KeepsAVersionOutsideTheSourcesWhileItExists)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const int idMovie{AddMovie(File(Folder(movies, "Film (2010)"), "Film (2010).mkv"))};
  ASSERT_GT(idMovie, 0);

  const std::string outside{Folder(m_root, "outside")};
  const std::string kept{File(outside, "Film (2010) Extended.mkv")};
  const std::string gone{File(outside, "Film (2010) Final Cut.mkv")};
  ASSERT_GT(AddAsset(idMovie, kept), 0);
  ASSERT_GT(AddAsset(idMovie, gone), 0);
  // Browsing for them caches the folder, which a file deleted outside Kodi leaves as it was
  CFileItemList items;
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(outside, items, "", XFILE::DIR_FLAG_DEFAULTS));

  ASSERT_TRUE(std::filesystem::remove(std::filesystem::path{
      std::u8string{reinterpret_cast<const char8_t*>(gone.data()), gone.size()}}));
  Clean();

  EXPECT_EQ(idMovie, m_db.GetMovieId(kept));
  EXPECT_EQ(-1, m_db.GetMovieId(gone));
}

// A folder that can't be listed may be on a drive that is only disconnected
TEST_F(TestVideoDatabaseClean, KeepsAVersionOutsideTheSourcesWhoseFolderIsUnavailable)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const int idMovie{AddMovie(File(Folder(movies, "Film (2010)"), "Film (2010).mkv"))};
  ASSERT_GT(idMovie, 0);
  const std::string unavailable{
      URIUtils::AddFileToFolder(Folder(m_root, "unplugged", false), "Film (2010) Extended.mkv")};
  ASSERT_GT(AddAsset(idMovie, unavailable), 0);

  Clean();

  EXPECT_EQ(idMovie, m_db.GetMovieId(unavailable));
}

TEST_F(TestVideoDatabaseClean, KeepsAPromotedVersionOutsideTheSourcesOnTheNextClean)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string gone{File(Folder(movies, "Film (2010)"), "Film (2010).mkv")};
  const int idMovie{AddMovie(gone)};
  ASSERT_GT(idMovie, 0);

  const std::string kept{File(Folder(m_root, "outside"), "Film (2010) Extended.mkv")};
  const int idVersion{AddAsset(idMovie, kept)};
  ASSERT_GT(idVersion, 0);

  ASSERT_TRUE(XFILE::CFile::Delete(gone));
  Clean();
  ASSERT_EQ(idVersion, m_db.GetFileIdByMovie(idMovie));

  Clean();

  EXPECT_EQ(idVersion, m_db.GetFileIdByMovie(idMovie));
  EXPECT_EQ(idMovie, m_db.GetMovieId(kept));
  EXPECT_EQ(1, m_db.GetSingleValueInt(
                   m_db.PrepareSQL("SELECT COUNT(*) FROM files WHERE idFile=%i", idVersion)));
  EXPECT_EQ(1, m_db.GetSingleValueInt(m_db.PrepareSQL(
                   "SELECT COUNT(*) FROM videoversion WHERE idFile=%i", idVersion)));
}

// The extra is checked first, as it has the lower file id
TEST_F(TestVideoDatabaseClean, KeepsADefaultVersionOutsideTheSourcesBesideAnExtraThatHasGone)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string original{File(Folder(movies, "Film (2010)"), "Film (2010).mkv")};
  const int idMovie{AddMovie(original)};
  ASSERT_GT(idMovie, 0);

  const std::string outside{Folder(m_root, "outside")};
  const std::string extra{File(outside, "Trailer.mkv")};
  const int idExtra{AddAsset(idMovie, extra, VideoAssetType::EXTRA)};
  const int idVersion{AddAsset(idMovie, File(outside, "Film (2010) Extended.mkv"))};
  ASSERT_GT(idExtra, 0);
  ASSERT_GT(idVersion, idExtra);
  ASSERT_TRUE(m_db.SetDefaultVideoVersion(VideoDbContentType::MOVIES, idMovie, idVersion));

  ASSERT_TRUE(XFILE::CFile::Delete(extra));
  Clean();

  EXPECT_EQ(idVersion, m_db.GetFileIdByMovie(idMovie));
  EXPECT_EQ(1, m_db.GetSingleValueInt(
                   m_db.PrepareSQL("SELECT COUNT(*) FROM files WHERE idFile=%i", idVersion)));
  EXPECT_EQ(0, m_db.GetSingleValueInt(
                   m_db.PrepareSQL("SELECT COUNT(*) FROM files WHERE idFile=%i", idExtra)));
}

TEST_F(TestVideoDatabaseClean, SwitchingDefaultsInASourceRootKeepsTheParentPath)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string kept{File(movies, "Film (2010).mkv")};
  const int idMovie{AddMovie(kept)};
  ASSERT_GT(idMovie, 0);
  const int idOriginal{m_db.GetFileIdByMovie(idMovie)};
  ASSERT_GT(idOriginal, 0);

  const std::string gone{File(movies, "Film (2010) Extended.mkv")};
  const int idVersion{AddAsset(idMovie, gone)};
  ASSERT_GT(idVersion, 0);
  ASSERT_TRUE(m_db.SetDefaultVideoVersion(VideoDbContentType::MOVIES, idMovie, idVersion));

  CFileItemList items;
  EXPECT_TRUE(m_db.GetItemsForPath("movies", movies, items));

  ASSERT_TRUE(XFILE::CFile::Delete(gone));
  Clean();

  EXPECT_EQ(idOriginal, m_db.GetFileIdByMovie(idMovie));
  EXPECT_EQ(idMovie, m_db.GetMovieId(kept));
  EXPECT_EQ(0, m_db.GetSingleValueInt(
                   m_db.PrepareSQL("SELECT COUNT(*) FROM files WHERE idFile=%i", idVersion)));
}

TEST_F(TestVideoDatabaseClean, KeepsAMovieWhoseDefaultVersionIsInAnUnavailableSource)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const int idMovie{AddMovie(File(Folder(movies, "Film (2010)"), "Film (2010).mkv"))};
  ASSERT_GT(idMovie, 0);

  const std::string offline{Folder(m_root, "offline", false)};
  AddSource(offline, "movies");
  const int idVersion{
      AddAsset(idMovie, URIUtils::AddFileToFolder(Folder(offline, "Film (2010)", false),
                                                  "Film (2010) Extended.mkv"))};
  ASSERT_GT(idVersion, 0);
  ASSERT_TRUE(m_db.SetDefaultVideoVersion(VideoDbContentType::MOVIES, idMovie, idVersion));

  Clean();

  EXPECT_EQ(idVersion, m_db.GetFileIdByMovie(idMovie));
}

TEST_F(TestVideoDatabaseClean, RemovesThePathOfAFolderThatHasGone)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string kept{File(Folder(movies, "Kept (2010)"), "Kept (2010).mkv")};
  ASSERT_GT(AddMovie(kept), 0);
  const std::string film{Folder(movies, "Film (2010)")};
  ASSERT_GT(AddMovie(File(film, "Film (2010).mkv")), 0);
  ASSERT_TRUE(m_db.SetPathHash(film, "hash"));

  ASSERT_TRUE(XFILE::CDirectory::RemoveRecursive(film));
  Clean();

  EXPECT_EQ(-1, m_db.GetPathId(film));
}

// The media's path carries a decision to remove its media when folder names are used, which
// must not remove the folder's own entry while it is still on disk
TEST_F(TestVideoDatabaseClean, KeepsThePathOfAFolderStillOnDiskThatLostItsMedia)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  ASSERT_TRUE(m_db.ExecuteQuery(
      m_db.PrepareSQL("UPDATE path SET useFolderNames=1 WHERE idPath=%i", m_db.GetPathId(movies))));
  const std::string film{Folder(movies, "Film (2010)")};
  const std::string gone{File(film, "Film (2010).mkv")};
  ASSERT_GT(AddMovie(gone), 0);
  ASSERT_TRUE(m_db.SetPathHash(film, "hash"));
  // keeps the folder's entry from going as one that holds nothing
  ASSERT_TRUE(m_db.SetPathHash(Folder(film, "Extras"), "hash"));

  ASSERT_TRUE(XFILE::CFile::Delete(gone));
  Clean();

  EXPECT_GT(m_db.GetPathId(film), 0);
}

// Media outside the paths being cleaned is not checked, so the entry of its path must stay too
TEST_F(TestVideoDatabaseClean, KeepsThePathOfMediaNotCleaned)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string kept{Folder(movies, "Kept (2010)")};
  ASSERT_GT(AddMovie(File(kept, "Kept (2010).mkv")), 0);
  const std::string film{Folder(movies, "Film (2010)")};
  const std::string gone{File(film, "Film (2010).mkv")};
  ASSERT_GT(AddMovie(gone), 0);
  ASSERT_TRUE(m_db.SetPathHash(film, "hash"));

  ASSERT_TRUE(XFILE::CDirectory::RemoveRecursive(film));
  m_db.CleanDatabase(nullptr, {m_db.GetPathId(kept)}, false);

  EXPECT_GT(m_db.GetPathId(film), 0);
  EXPECT_GT(m_db.GetMovieId(gone), 0);
}

// An archive's entry hangs off the folder holding it, which is still there
TEST_F(TestVideoDatabaseClean, RemovesThePathOfAnArchiveThatHasGone)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string film{Folder(movies, "Film (2010)")};
  ASSERT_TRUE(m_db.SetPathHash(film, "hash"));
  const std::string archive{File(film, "Film (2010).zip")};
  const std::string video{ArchivePath("zip", archive, "Film (2010).mkv")};
  ASSERT_GT(AddMovie(video), 0);
  const std::string archived{URIUtils::GetDirectory(video)};
  ASSERT_GT(m_db.GetPathId(archived), 0);

  ASSERT_TRUE(XFILE::CFile::Delete(archive));
  Clean();

  EXPECT_EQ(-1, m_db.GetMovieId(video));
  EXPECT_EQ(-1, m_db.GetPathId(archived));
}

TEST_F(TestVideoDatabaseClean, RemovesAPlayedFileThatHasGone)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string gone{File(Folder(movies, "Played"), "Played.mkv")};
  MarkPlayed(gone, 1, 600.0);
  const int idFile{m_db.AddFile(gone)};
  ASSERT_GT(idFile, 0);

  ASSERT_TRUE(XFILE::CFile::Delete(gone));
  Clean();

  EXPECT_EQ(0, m_db.GetSingleValueInt(
                   m_db.PrepareSQL("SELECT COUNT(*) FROM files WHERE idFile=%i", idFile)));
}

TEST_F(TestVideoDatabaseClean, KeepsPlayedFilesThatMayNotHaveGone)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string played{Folder(movies, "Played")};
  const std::string offline{Folder(m_root, "offline", false)};
  AddSource(offline, "movies");
  const std::string outside{Folder(m_root, "outside")};

  const std::vector<std::string> files{
      File(played, "Still there.mkv"),
      URIUtils::AddFileToFolder(Folder(offline, "Played", false), "Unavailable.mkv"),
      URIUtils::AddFileToFolder(outside, "Outside the sources.mkv")};
  std::vector<int> ids;
  for (const auto& file : files)
  {
    MarkPlayed(file, 1);
    ids.push_back(m_db.AddFile(file));
    ASSERT_GT(ids.back(), 0);
  }

  // a folder entry is still there while its folder can be listed
  ASSERT_TRUE(m_db.SetPlayCount(CFileItem(played, true), 1).IsValid());
  ids.push_back(m_db.AddFile(played));
  ASSERT_GT(ids.back(), 0);

  Clean();

  for (const int idFile : ids)
    EXPECT_EQ(1, m_db.GetSingleValueInt(
                     m_db.PrepareSQL("SELECT COUNT(*) FROM files WHERE idFile=%i", idFile)))
        << idFile;
}
