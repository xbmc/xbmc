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
#include "Util.h"
#include "XBDateTime.h"
#include "cores/VideoSettings.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "imagefiles/ImageFileURL.h"
#include "interfaces/AnnouncementManager.h"
#include "language/LangInfo.h"
#include "playlists/SmartPlayList.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/Artwork.h"
#include "utils/DatabaseUtils.h"
#include "utils/StreamDetails.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "video/Bookmark.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbUrl.h"
#include "video/VideoInfoScanner.h"
#include "video/VideoInfoScannerArt.h"
#include "video/VideoInfoScannerExtras.h"
#include "video/VideoInfoTag.h"
#include "video/VideoManagerTypes.h"
#include "video/VideoUtils.h"
#include "video/jobs/VideoLibraryMarkWatchedJob.h"
#include "video/jobs/VideoLibraryResetResumePointJob.h"

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <utility>
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

  // A movie with the given art, and a second movie with the given art converted into a version
  // of it
  struct MovieWithVersion
  {
    int movieId{-1};
    int movieFileId{-1};
    int versionFileId{-1};
  };

  MovieWithVersion AddMovieWithVersion(const KODI::ART::Artwork& movieArt,
                                       const KODI::ART::Artwork& versionArt)
  {
    CVideoInfoTag movieTag{Tag("/movies/Movie (2010)/Movie (2010) Extended Edition.mkv")};
    CVideoInfoTag versionTag{Tag("/movies/Movie (2010)/Movie (2010) Standard Edition.mkv")};
    const int movieId{m_db.SetDetailsForMovie(movieTag, movieArt)};
    const int sourceId{m_db.SetDetailsForMovie(versionTag, versionArt)};
    if (movieId <= 0 || sourceId <= 0)
      return {};

    const MovieWithVersion result{movieId, m_db.GetFileIdByMovie(movieId),
                                  m_db.GetFileIdByMovie(sourceId)};
    if (!m_db.ConvertVideoToVersion(VideoDbContentType::MOVIES, sourceId, movieId, -1,
                                    VideoAssetType::VERSION))
      return {};
    return result;
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

TEST_F(TestVideoDatabase, ATvShowDirectorFoundBySearchListsTheirShows)
{
  CVideoInfoTag directed;
  directed.m_strTitle = "Directed";
  directed.m_strPath = "/tv/directed/";
  directed.SetDirector({"Jane Director"});
  const int idShow{m_db.SetDetailsForTvShow({directed.m_strPath}, directed, KODI::ART::Artwork{},
                                            KODI::ART::SeasonsArtwork{})};
  ASSERT_GT(idShow, 0);
  ASSERT_GT(AddTvShow("/tv/undirected/"), 0);

  CFileItemList directors;
  m_db.GetTvShowsDirectorsByName("Jane", directors);
  ASSERT_EQ(1, directors.Size());

  CFileItemList shows;
  ASSERT_TRUE(m_db.GetTvShowsByWhere(directors[0]->GetPath(), CDatabase::Filter(), shows));
  ASSERT_EQ(1, shows.Size());
  EXPECT_EQ(idShow, shows[0]->GetVideoInfoTag()->m_iDbId);
}

TEST_F(TestVideoDatabase, ASmartPlaylistGenreRuleFindsAMovieOfThatGenre)
{
  CVideoInfoTag drama{Tag("/videos/drama.mkv")};
  drama.SetGenre({"Drama"});
  const int idMovie{m_db.SetDetailsForMovie(drama, KODI::ART::Artwork{})};
  ASSERT_GT(idMovie, 0);
  ASSERT_GT(AddMovie("/videos/other.mkv"), 0);

  KODI::PLAYLIST::CSmartPlaylistRule rule;
  rule.m_field = static_cast<int>(Field::GENRE);
  rule.m_operator = CDatabaseQueryRule::SearchOperator::OPERATOR_EQUALS;
  rule.m_parameter = {"Drama"};
  const std::string where{rule.GetWhereClause(m_db, "movies")};
  EXPECT_NE(std::string::npos, where.find("genre_link.media_type = 'movie'")) << where;

  CFileItemList items;
  ASSERT_TRUE(m_db.GetMoviesByWhere("videodb://movies/titles/", CDatabase::Filter{where}, items));
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

// The id column and the table NameOf() gives must name the same table
TEST(TestVideoDatabaseContent, ContentNamesItsTableAndIdColumn)
{
  const auto check = [](VideoDbContentType content, KODI::MEDIA::TYPE type, std::string_view idColumn)
  {
    EXPECT_EQ(DatabaseUtils::MediaTypeFromVideoContentType(content), type);
    EXPECT_EQ(CVideoDatabase::IdColumnOf(type), idColumn);
  };
  check(VideoDbContentType::MOVIES, KODI::MEDIA::TYPE::MOVIE, "idMovie");
  check(VideoDbContentType::TVSHOWS, KODI::MEDIA::TYPE::TV_SHOW, "idShow");
  check(VideoDbContentType::EPISODES, KODI::MEDIA::TYPE::EPISODE, "idEpisode");
  check(VideoDbContentType::MUSICVIDEOS, KODI::MEDIA::TYPE::MUSIC_VIDEO, "idMVideo");
  check(VideoDbContentType::MOVIE_SETS, KODI::MEDIA::TYPE::VIDEO_COLLECTION, "");
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

// The entries of a disc share its nfo even where another of the movie's files sorts between them,
// as a file extra's folder does between the disc's raw default and its playlists
TEST_F(TestVideoDatabase, ExportToSeparateFilesKeepsADiscsEntriesTogether)
{
  const std::string root{
      CSpecialProtocol::TranslatePath("special://temp/ExportDisc/Movie (2020)/")};
  XFILE::CDirectory::RemoveRecursive(root);
  const std::string bdmv{URIUtils::AddFileToFolder(root, "Disc 1", "BDMV")};
  const std::string extras{URIUtils::AddFileToFolder(root, "Extras")};
  for (const std::string& folder : {root, URIUtils::AddFileToFolder(root, "Disc 1"), bdmv,
                                    URIUtils::AddFileToFolder(bdmv, "PLAYLIST"), extras})
    ASSERT_TRUE(XFILE::CDirectory::Create(folder));
  const std::string disc{URIUtils::AddFileToFolder(bdmv, "index.bdmv")};
  const std::string trailer{URIUtils::AddFileToFolder(extras, "trailer.mkv")};
  for (const std::string& file :
       {disc, URIUtils::AddFileToFolder(bdmv, "PLAYLIST", "00801.mpls"), trailer})
  {
    XFILE::CFile out;
    ASSERT_TRUE(out.OpenForWrite(file, true));
    out.Close();
  }

  const int idMovie{AddMovie(disc)};
  ASSERT_GT(idMovie, 0);
  CFileItem version{URIUtils::GetBlurayPlaylistPath(disc, 801), false};
  ASSERT_TRUE(m_db.AddVideoAsset(
      VideoDbContentType::MOVIES, idMovie,
      m_db.AddVideoVersionType("Extended", VideoAssetTypeOwner::USER, VideoAssetType::VERSION),
      VideoAssetType::VERSION, version));
  CFileItem extra{trailer, false};
  ASSERT_TRUE(m_db.AddVideoAsset(
      VideoDbContentType::MOVIES, idMovie,
      m_db.AddVideoVersionType("Trailer", VideoAssetTypeOwner::AUTO, VideoAssetType::EXTRA),
      VideoAssetType::EXTRA, extra));

  m_db.ExportToXML(root, false);

  CXBMCTinyXML doc;
  const bool loaded{doc.LoadFile(URIUtils::AddFileToFolder(bdmv, "index.nfo"))};
  std::vector<int> playlists;
  for (const TiXmlElement* movie{loaded ? doc.RootElement()->FirstChildElement("movie") : nullptr};
       movie; movie = movie->NextSiblingElement("movie"))
  {
    int playlist{-1};
    XMLUtils::GetInt(movie, "playlist", playlist);
    playlists.emplace_back(playlist);
  }
  XFILE::CDirectory::RemoveRecursive(CSpecialProtocol::TranslatePath("special://temp/ExportDisc/"));
  ASSERT_TRUE(loaded);
  EXPECT_EQ(2u, playlists.size());
  EXPECT_NE(playlists.end(), std::ranges::find(playlists, 801));
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
  ASSERT_TRUE(m_db.SetArtForItem(idMovie, KODI::MEDIA::TYPE::MOVIE, movieArt));

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
  ASSERT_TRUE(m_db.GetArtForItem(idMovie, KODI::MEDIA::TYPE::MOVIE, art));
  EXPECT_EQ((KODI::ART::Artwork{{"fanart", "movie-fanart"},
                                {"poster", "version-poster"},
                                {"thumb", chapterFrame.ToString()}}),
            art);

  KODI::ART::Artwork originalArt;
  ASSERT_TRUE(m_db.GetArtForItem(idOriginal, KODI::MEDIA::TYPE::VIDEO_VERSION, originalArt));
  EXPECT_EQ(movieArt, originalArt);
}

// No frame can be taken from a disc, so one of the old default's file isn't moved onto it
TEST_F(TestVideoDatabase, SetDefaultVideoVersionDropsAFrameNoneCanBeTakenFor)
{
  const std::string original{"/movies/Movie (2010)/Movie (2010).mkv"};
  const int idMovie{AddMovie(original)};
  ASSERT_GT(idMovie, 0);
  ASSERT_TRUE(m_db.SetArtForItem(
      idMovie, KODI::MEDIA::TYPE::MOVIE,
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
  ASSERT_TRUE(m_db.GetArtForItem(idMovie, KODI::MEDIA::TYPE::MOVIE, art));
  EXPECT_EQ((KODI::ART::Artwork{{"fanart", "movie-fanart"}}), art);
}

// A frame of the old default's file is not kept for a new default that has no thumb of its own,
// one of the new default's file is taken instead
TEST_F(TestVideoDatabase, SetDefaultVideoVersionTakesAFrameOfTheNewDefault)
{
  const std::string original{"/movies/Movie (2010)/Movie (2010).mkv"};
  const int idMovie{AddMovie(original)};
  ASSERT_GT(idMovie, 0);
  ASSERT_TRUE(m_db.SetArtForItem(
      idMovie, KODI::MEDIA::TYPE::MOVIE,
      {{"fanart", "movie-fanart"}, {"thumb", IMAGE_FILES::URLFromFile(original, "video")}}));

  const std::string extended{"/movies/Movie (2010)/Movie (2010) Extended Edition.mkv"};
  CFileItem item{extended, false};
  const int idType{
      m_db.AddVideoVersionType("Extended", VideoAssetTypeOwner::USER, VideoAssetType::VERSION)};
  ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, idType,
                                 VideoAssetType::VERSION, item));

  ASSERT_TRUE(
      m_db.SetDefaultVideoVersion(VideoDbContentType::MOVIES, idMovie, m_db.AddFile(extended)));

  KODI::ART::Artwork art;
  ASSERT_TRUE(m_db.GetArtForItem(idMovie, KODI::MEDIA::TYPE::MOVIE, art));
  EXPECT_EQ((KODI::ART::Artwork{{"fanart", "movie-fanart"},
                                {"thumb", IMAGE_FILES::URLFromFile(extended, "video")}}),
            art);
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
    std::erase_if(CMediaSourceSettings::GetInstance().GetSources(KODI::MEDIA::MediaSection::VIDEO),
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
    CMediaSourceSettings::GetInstance()
        .GetSources(KODI::MEDIA::MediaSection::VIDEO)
        .push_back(source);

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
  ASSERT_TRUE(m_db.SetArtForItem(idMovie, KODI::MEDIA::TYPE::MOVIE,
                                 {{"clearlogo", IMAGE_FILES::URLFromFile(gone, "video_clearlogo")},
                                  {"fanart", "movie-fanart"},
                                  {"poster", "movie-poster"},
                                  {"thumb", IMAGE_FILES::URLFromFile(gone, "video")}}));
  const std::string version{File(film, "Film (2010) Extended.mkv")};
  ASSERT_GT(AddAsset(idMovie, version, VideoAssetType::VERSION, {{"poster", "version-poster"}}), 0);

  ASSERT_TRUE(XFILE::CFile::Delete(gone));
  Clean();

  KODI::ART::Artwork art;
  ASSERT_TRUE(m_db.GetArtForItem(idMovie, KODI::MEDIA::TYPE::MOVIE, art));
  EXPECT_EQ((KODI::ART::Artwork{{"fanart", "movie-fanart"},
                                {"poster", "version-poster"},
                                {"thumb", IMAGE_FILES::URLFromFile(version, "video")}}),
            art);
}

// The versions of one movie share its id, and each has gone
TEST_F(TestVideoDatabaseClean, RemovesEveryVersionOfAMovieThatHasGone)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string film{Folder(movies, "Film (2010)")};
  const int idMovie{AddMovie(File(film, "Film (2010).mkv"))};
  ASSERT_GT(idMovie, 0);
  const std::string extended{File(film, "Film (2010) Extended.mkv")};
  const std::string finalCut{File(film, "Film (2010) Final Cut.mkv")};
  ASSERT_GT(AddAsset(idMovie, extended), 0);
  ASSERT_GT(AddAsset(idMovie, finalCut), 0);

  ASSERT_TRUE(XFILE::CFile::Delete(extended));
  ASSERT_TRUE(XFILE::CFile::Delete(finalCut));
  Clean();

  EXPECT_EQ(-1, m_db.GetMovieId(extended));
  EXPECT_EQ(-1, m_db.GetMovieId(finalCut));
}

// The episodes of a multi-episode file share its file, and each has gone
TEST_F(TestVideoDatabaseClean, RemovesEveryEpisodeOfAFileThatHasGone)
{
  const std::string shows{Folder(m_root, "shows")};
  AddSource(shows, "tvshows");
  const std::string show{Folder(shows, "Show")};
  const int idShow{AddTvShow(show)};
  ASSERT_GT(idShow, 0);
  const std::string gone{File(show, "Show S01E01E02.mkv")};
  // as a scan adds each episode of one file
  for (const int episode : {1, 2})
  {
    CVideoInfoTag tag{Tag(gone)};
    tag.m_iSeason = 1;
    tag.m_iEpisode = episode;
    const int idEpisode{m_db.AddNewEpisode(idShow, tag)};
    ASSERT_GT(m_db.SetDetailsForEpisode(tag, KODI::ART::Artwork{}, idShow, idEpisode), 0);
  }
  const int idKept{AddEpisode(idShow, File(show, "Show S01E03.mkv"), 3)};
  ASSERT_GT(idKept, 0);
  ASSERT_EQ(3, m_db.GetSingleValueInt(
                   m_db.PrepareSQL("SELECT COUNT(*) FROM episode WHERE idShow=%i", idShow)));

  ASSERT_TRUE(XFILE::CFile::Delete(gone));
  Clean();

  EXPECT_EQ(1, m_db.GetSingleValueInt(
                   m_db.PrepareSQL("SELECT COUNT(*) FROM episode WHERE idShow=%i", idShow)));
  EXPECT_EQ(idKept, m_db.GetSingleValueInt(
                        m_db.PrepareSQL("SELECT idEpisode FROM episode WHERE idShow=%i", idShow)));
}

namespace
{
// Never started, so it keeps every announcement made in its queue
class CQueuedAnnouncements : public ANNOUNCEMENT::CAnnouncementManager
{
public:
  long CountRemoved(const std::string& type, int id) const
  {
    return std::ranges::count_if(m_announcementQueue,
                                 [&type, id](const CAnnounceData& announcement)
                                 {
                                   return announcement.message == "OnRemove" &&
                                          announcement.data["type"].asString() == type &&
                                          announcement.data["id"].asInteger() == id;
                                 });
  }

  std::vector<CAnnounceData> Files() const
  {
    std::vector<CAnnounceData> files;
    std::ranges::copy_if(m_announcementQueue, std::back_inserter(files),
                         [](const CAnnounceData& announcement)
                         { return announcement.flag == ANNOUNCEMENT::Files; });
    return files;
  }
};
} // namespace

// The clean finds an episode's source through each folder of its show
TEST_F(TestVideoDatabaseClean, AnnouncesAnEpisodeOfAShowWithTwoFoldersRemovedOnce)
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
  const int idEpisode{AddEpisode(idShow, gone, 1)};
  ASSERT_GT(idEpisode, 0);
  ASSERT_GT(AddEpisode(idShow, File(second, "Show S01E02.mkv"), 2), 0);

  const auto announcements{std::make_shared<CQueuedAnnouncements>()};
  CServiceBroker::UnregisterAnnouncementManager();
  CServiceBroker::RegisterAnnouncementManager(announcements);

  ASSERT_TRUE(XFILE::CFile::Delete(gone));
  Clean();

  EXPECT_EQ(0, m_db.GetSingleValueInt(
                   m_db.PrepareSQL("SELECT COUNT(*) FROM episode WHERE idEpisode=%i", idEpisode)));
  EXPECT_EQ(1, announcements->CountRemoved("episode", idEpisode));
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

TEST_F(TestVideoDatabaseClean, RemovesAnExtraOutsideTheSourcesWhoseDiscHasGone)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const int idMovie{AddMovie(File(Folder(movies, "Film (2010)"), "Film (2010).mkv"))};
  ASSERT_GT(idMovie, 0);
  const std::string gone{URIUtils::GetBlurayPlaylistPath(
      URIUtils::AddFileToFolder(Folder(m_root, "outside"), "BDMV", "index.bdmv"), 801)};
  const int idExtra{AddAsset(idMovie, gone, VideoAssetType::EXTRA)};
  ASSERT_GT(idExtra, 0);
  const std::string unavailable{URIUtils::GetBlurayPlaylistPath(
      URIUtils::AddFileToFolder(Folder(m_root, "unplugged", false), "BDMV", "index.bdmv"), 801)};
  ASSERT_GT(AddAsset(idMovie, unavailable, VideoAssetType::EXTRA), 0);

  Clean();

  EXPECT_EQ(0, m_db.GetSingleValueInt(
                   m_db.PrepareSQL("SELECT COUNT(*) FROM files WHERE idFile=%i", idExtra)));
  EXPECT_EQ(idMovie, m_db.GetVideoVersionInfo(unavailable).m_idMedia);
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

// A new default version lacking an art type must not take that art away from the movie
TEST_F(TestVideoDatabase, SetDefaultVideoVersionKeepsArtTheVersionLacks)
{
  const auto [movieId, movieFileId, versionFileId] = AddMovieWithVersion(
      {{"poster", "old-poster.jpg"}, {"fanart", "old-fanart.jpg"}}, {{"poster", "new-poster.jpg"}});
  ASSERT_GT(versionFileId, 0);

  ASSERT_TRUE(m_db.SetDefaultVideoVersion(VideoDbContentType::MOVIES, movieId, versionFileId));

  KODI::ART::Artwork movieArt;
  ASSERT_TRUE(m_db.GetArtForItem(movieId, KODI::MEDIA::TYPE::MOVIE, movieArt));
  EXPECT_EQ((KODI::ART::Artwork{{"poster", "new-poster.jpg"}, {"fanart", "old-fanart.jpg"}}),
            movieArt);

  // The previous default version keeps all of its own art
  KODI::ART::Artwork previousVersionArt;
  ASSERT_TRUE(
      m_db.GetArtForItem(movieFileId, KODI::MEDIA::TYPE::VIDEO_VERSION, previousVersionArt));
  EXPECT_EQ((KODI::ART::Artwork{{"poster", "old-poster.jpg"}, {"fanart", "old-fanart.jpg"}}),
            previousVersionArt);
}

// An empty version art url (art type set to none) falls back to the movie art, so it must not
// replace that art when the version becomes the default
TEST_F(TestVideoDatabase, SetDefaultVideoVersionKeepsArtTheVersionHasNoneOf)
{
  const auto [movieId, movieFileId, versionFileId] = AddMovieWithVersion(
      {{"poster", "old-poster.jpg"}, {"fanart", "old-fanart.jpg"}}, {{"poster", "new-poster.jpg"}});
  ASSERT_GT(versionFileId, 0);
  ASSERT_TRUE(m_db.SetArtForItem(versionFileId, KODI::MEDIA::TYPE::VIDEO_VERSION, "fanart", ""));

  ASSERT_TRUE(m_db.SetDefaultVideoVersion(VideoDbContentType::MOVIES, movieId, versionFileId));

  EXPECT_EQ("old-fanart.jpg", m_db.GetArtForItem(movieId, KODI::MEDIA::TYPE::MOVIE, "fanart"));
}

// Art of the previous default version itself is not movie art and must not be kept as such
TEST_F(TestVideoDatabase, SetDefaultVideoVersionKeepsOnlyTheMovieArt)
{
  const auto [movieId, movieFileId, versionFileId] = AddMovieWithVersion(
      {{"poster", "old-poster.jpg"}, {"fanart", "old-fanart.jpg"}}, {{"poster", "new-poster.jpg"}});
  ASSERT_GT(versionFileId, 0);
  ASSERT_TRUE(m_db.SetArtForItem(movieFileId, KODI::MEDIA::TYPE::VIDEO_VERSION, "fanart",
                                 "version-fanart.jpg"));

  ASSERT_TRUE(m_db.SetDefaultVideoVersion(VideoDbContentType::MOVIES, movieId, versionFileId));

  EXPECT_EQ(1, m_db.GetSingleValueInt(
                   "SELECT COUNT(*) FROM art WHERE media_id = " + std::to_string(movieId) +
                   " AND media_type = 'movie' AND type = 'fanart'"));
  EXPECT_EQ("old-fanart.jpg", m_db.GetArtForItem(movieId, KODI::MEDIA::TYPE::MOVIE, "fanart"));
}

namespace
{
// The jobs run on the test database instead of the library's
class MarkWatchedJob : public CVideoLibraryMarkWatchedJob
{
public:
  using CVideoLibraryMarkWatchedJob::CVideoLibraryMarkWatchedJob;
  using CVideoLibraryMarkWatchedJob::Work;
};

class ResetResumePointJob : public CVideoLibraryResetResumePointJob
{
public:
  using CVideoLibraryResetResumePointJob::CVideoLibraryResetResumePointJob;
  using CVideoLibraryResetResumePointJob::Work;
};

class TestPlayStateAnnouncements : public TestVideoDatabase
{
protected:
  void SetUp() override
  {
    TestVideoDatabase::SetUp();
    CServiceBroker::UnregisterAnnouncementManager();
    CServiceBroker::RegisterAnnouncementManager(m_announcements);
  }

  const std::shared_ptr<CQueuedAnnouncements> m_announcements{
      std::make_shared<CQueuedAnnouncements>()};
};

constexpr const char* PLUGIN_ITEM = "plugin://plugin.video.example/play/?id=1";

// A plugin item showing the play count and resume point its add-on gave it, unknown to the database
std::shared_ptr<CFileItem> PluginItem(int playCount, double resumeSeconds = 0.0)
{
  auto item{std::make_shared<CFileItem>(PLUGIN_ITEM, false)};
  item->GetVideoInfoTag()->SetPlayCount(playCount);
  CBookmark resumePoint;
  resumePoint.timeInSeconds = resumeSeconds;
  resumePoint.totalTimeInSeconds = 3600.0;
  item->GetVideoInfoTag()->SetResumePoint(resumePoint);
  return item;
}
} // namespace

TEST_F(TestPlayStateAnnouncements, MarkingUnwatchedAnnouncesAPlayCountTheDatabaseNeverHad)
{
  ASSERT_TRUE(MarkWatchedJob(PluginItem(1), false).Work(m_db));

  const auto files{m_announcements->Files()};
  ASSERT_EQ(1u, files.size());
  EXPECT_EQ("OnPlayCountChanged", files[0].message);
  EXPECT_EQ(PLUGIN_ITEM, files[0].data["file"].asString());
  EXPECT_EQ(0, files[0].data["playcount"].asInteger());
}

TEST_F(TestPlayStateAnnouncements, MarkingWatchedResetsTheResumePointShownFirst)
{
  ASSERT_TRUE(MarkWatchedJob(PluginItem(0, 600.0), true).Work(m_db));

  const auto files{m_announcements->Files()};
  ASSERT_EQ(2u, files.size());
  EXPECT_EQ("OnResumePointChanged", files[0].message);
  EXPECT_EQ(0.0, files[0].data["resume"]["position"].asDouble());
  EXPECT_EQ("OnPlayCountChanged", files[1].message);
  EXPECT_EQ(1, files[1].data["playcount"].asInteger());
}

TEST_F(TestPlayStateAnnouncements, MarkingAnItemShownInThatStateAnnouncesNothing)
{
  ASSERT_TRUE(MarkWatchedJob(PluginItem(1), true).Work(m_db));

  EXPECT_TRUE(m_announcements->Files().empty());
}

TEST_F(TestPlayStateAnnouncements, MarkingWatchedResetsAResumePointOnlyTheDatabaseHad)
{
  CBookmark resumePoint;
  resumePoint.timeInSeconds = 600.0;
  resumePoint.totalTimeInSeconds = 3600.0;
  ASSERT_TRUE(m_db.AddBookMarkToFile("/videos/film.mkv", resumePoint, CBookmark::RESUME));

  // As the items of a folder come, without a video info tag
  ASSERT_TRUE(
      MarkWatchedJob(std::make_shared<CFileItem>("/videos/film.mkv", false), true).Work(m_db));

  const auto files{m_announcements->Files()};
  ASSERT_EQ(2u, files.size());
  EXPECT_EQ("OnResumePointChanged", files[0].message);
  EXPECT_EQ("/videos/film.mkv", files[0].data["file"].asString());
  EXPECT_EQ("OnPlayCountChanged", files[1].message);
}

TEST_F(TestPlayStateAnnouncements, ResettingAResumePointTheDatabaseNeverHadAnnouncesIt)
{
  ASSERT_TRUE(ResetResumePointJob(PluginItem(0, 600.0)).Work(m_db));

  const auto files{m_announcements->Files()};
  ASSERT_EQ(1u, files.size());
  EXPECT_EQ("OnResumePointChanged", files[0].message);
  EXPECT_EQ(PLUGIN_ITEM, files[0].data["file"].asString());
  EXPECT_EQ(0.0, files[0].data["resume"]["position"].asDouble());
}

TEST_F(TestPlayStateAnnouncements, ResettingALibraryItemAnnouncesItsFile)
{
  auto item{std::make_shared<CFileItem>("videodb://movies/titles/1", false)};
  item->GetVideoInfoTag()->m_strFileNameAndPath = "/movies/film.mkv";

  ASSERT_TRUE(ResetResumePointJob(item).Work(m_db));

  const auto files{m_announcements->Files()};
  ASSERT_EQ(1u, files.size());
  EXPECT_EQ("/movies/film.mkv", files[0].data["file"].asString());
}

TEST_F(TestPlayStateAnnouncements, AnnouncesAFileWithoutCredentials)
{
  KODI::VIDEO::UTILS::AnnouncePlayCountChanged("smb://user:secret@server/videos/film.mkv", 1);

  const auto files{m_announcements->Files()};
  ASSERT_EQ(1u, files.size());
  EXPECT_EQ("smb://server/videos/film.mkv", files[0].data["file"].asString());
}

// Writes made later on the connection would otherwise never be committed
TEST_F(TestVideoDatabaseClean, EndsItsTransactionWhenThePathsHoldNoMedia)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");

  m_db.CleanDatabase(nullptr, {m_db.GetPathId(movies)}, false);

  EXPECT_FALSE(m_db.InTransaction());
}

// A bonus disc's folder sits beside the folders holding the movie's own discs
TEST_F(TestVideoDatabase, GetMovieIdInFolderFindsTheMovieOnTheDiscsBesideAnExtrasFolder)
{
  const std::string folder{"/movies/Aliens (1986)/"};
  const std::string extras{folder + "Bonus Disc/"};

  const int idMovie{AddMovie(URIUtils::GetBlurayPlaylistPath(folder + "Disc 1/ALIENS.iso", 800))};
  ASSERT_GT(idMovie, 0);
  CFileItem version{URIUtils::GetBlurayPlaylistPath(folder + "Disc 2/ALIENS.iso", 800), false};
  ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, VIDEO_VERSION_ID_DEFAULT,
                                 VideoAssetType::VERSION, version));

  // Scraped as a movie of its own before extras were recognised there
  ASSERT_GT(AddMovie(URIUtils::GetBlurayPlaylistPath(extras + "ALIENS_BONUS.iso", 800)), 0);

  EXPECT_EQ(idMovie, m_db.GetMovieIdInFolder(folder, extras));

  // Nor is there a single movie where a folder holds two
  ASSERT_GT(AddMovie(folder + "Aliens (1986) Special Edition.mkv"), 0);
  EXPECT_EQ(-1, m_db.GetMovieIdInFolder(folder, extras));
}

TEST_F(TestVideoDatabase, GetMovieIdFindsTheMovieOnTheDiscPlaylistsOfADiscImageWithoutOne)
{
  const std::string disc{"/movies/M3GAN 2.0 (2025)/M3GAN20_UPK1.iso"};

  // Left by a refresh that found the movie on two of the disc's playlists
  ASSERT_GT(m_db.AddFile(disc), 0);
  EXPECT_EQ(-1, m_db.GetMovieId(disc));

  const int idMovie{AddMovie(URIUtils::GetBlurayPlaylistPath(disc, 801))};
  ASSERT_GT(idMovie, 0);
  CFileItem version{URIUtils::GetBlurayPlaylistPath(disc, 800), false};
  ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, VIDEO_VERSION_ID_DEFAULT,
                                 VideoAssetType::VERSION, version));

  EXPECT_EQ(idMovie, m_db.GetMovieId(disc));
}

// The movie GetMovieId() finds for such a disc image is read through its default version, as the
// image's own file is none of its versions
TEST_F(TestVideoDatabase, GetMovieInfoReadsTheMovieOfADiscImageWithoutOne)
{
  const std::string disc{"/movies/M3GAN 2.0 (2025)/M3GAN20_UPK1.iso"};
  ASSERT_GT(m_db.AddFile(disc), 0);
  const std::string main{URIUtils::GetBlurayPlaylistPath(disc, 801)};
  const int idMovie{AddMovie(main)};
  ASSERT_GT(idMovie, 0);

  CVideoInfoTag tag;
  ASSERT_TRUE(m_db.GetMovieInfo(disc, tag));
  EXPECT_EQ(idMovie, tag.m_iDbId);
  EXPECT_EQ(main, tag.m_strFileNameAndPath);
}

TEST_F(TestVideoDatabase, KindsOfExtraAreBuiltInExtraTypes)
{
  // The types were named when the database was made, before any strings were loaded
  auto& strings{CServiceBroker::GetResourcesComponent().GetLocalizeStrings()};
  ASSERT_TRUE(strings.Load(g_langInfo.GetLanguagePath(), "resource.language.en_gb"));
  m_db.UpdateVideoVersionTypeTable();

  const auto names{[this](VideoAssetType assetType)
                   {
                     CFileItemList types;
                     m_db.GetVideoVersionTypes(VideoDbContentType::MOVIES, assetType, types);
                     std::set<std::string> labels;
                     for (const auto& type : types)
                       labels.emplace(type->GetLabel());
                     return labels;
                   }};
  const std::set<std::string> extras{names(VideoAssetType::EXTRA)};
  const std::set<std::string> versions{names(VideoAssetType::VERSION)};

  EXPECT_TRUE(extras.contains("Deleted scenes"));
  EXPECT_TRUE(extras.contains("Play all"));
  EXPECT_FALSE(versions.contains("Deleted scenes"));
  EXPECT_TRUE(versions.contains("Standard Edition"));
  EXPECT_FALSE(extras.contains("Deleted scene: {0:s}"));
  EXPECT_FALSE(versions.contains("Deleted scene: {0:s}"));

  // A disc's extra known only by its kind takes the built-in type
  EXPECT_EQ(m_db.AddVideoVersionType("Behind the scenes", VideoAssetTypeOwner::AUTO,
                                     VideoAssetType::EXTRA),
            40506);

  strings.Clear();
}

// A file can be only one asset, so each extra on a disc is a playlist of its own
TEST_F(TestVideoDatabase, ExtrasOnADiscAreItsPlaylists)
{
  const std::string disc{"/movies/Nope (2022)/NOPE.iso"};
  const int idMovie{AddMovie(URIUtils::GetBlurayPlaylistPath(disc, 800))};
  ASSERT_GT(idMovie, 0);

  for (const auto& [playlist, name] : {std::pair{1244, "Gag Reel"}, std::pair{1245, "Making Of"}})
  {
    const int idType{
        m_db.AddVideoVersionType(name, VideoAssetTypeOwner::AUTO, VideoAssetType::EXTRA)};
    CFileItem extra{URIUtils::GetBlurayPlaylistPath(disc, playlist), false};
    ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, idType,
                                   VideoAssetType::EXTRA, extra));
  }

  std::set<int> extras;
  for (const auto& playlist : m_db.GetPlaylistsByPath(URIUtils::GetBlurayPlaylistPath(disc)))
  {
    EXPECT_EQ(idMovie, playlist.idMedia);
    if (playlist.itemType == VideoAssetType::EXTRA)
      extras.emplace(playlist.playlist);
  }
  EXPECT_EQ(extras, (std::set<int>{1244, 1245}));
}

// A name used for both a version and an extra is a type of each kind, so neither is stored with a
// type of the other kind
TEST_F(TestVideoDatabase, VideoVersionTypeIsLookedUpByItemType)
{
  const int version{m_db.AddOrValidateVideoVersionType("Prologue", VideoAssetType::VERSION)};
  const int extra{m_db.AddOrValidateVideoVersionType("Prologue", VideoAssetType::EXTRA)};
  ASSERT_GT(version, 0);
  ASSERT_GT(extra, 0);
  EXPECT_NE(version, extra);
  EXPECT_EQ(version, m_db.GetVideoVersionByTitle("Prologue", VideoAssetType::VERSION));
  EXPECT_EQ(extra, m_db.GetVideoVersionByTitle("Prologue", VideoAssetType::EXTRA));
  EXPECT_EQ(extra, m_db.AddOrValidateVideoVersionType("Prologue", VideoAssetType::EXTRA));
}

// The name of an extra other than a built-in kind is that of the one extra, so goes once no extra
// has it, whether the extra is removed or given another type
TEST_F(TestVideoDatabase, AnExtrasTypeGoesWithItsLastExtra)
{
  auto& strings{CServiceBroker::GetResourcesComponent().GetLocalizeStrings()};
  ASSERT_TRUE(strings.Load(g_langInfo.GetLanguagePath(), "resource.language.en_gb"));
  m_db.UpdateVideoVersionTypeTable();

  const int idMovie{AddMovie("/movies/Movie (2020)/movie.mkv")};
  ASSERT_GT(idMovie, 0);
  const int made{
      m_db.AddVideoVersionType("Gag Reel", VideoAssetTypeOwner::AUTO, VideoAssetType::EXTRA)};
  const int typed{
      m_db.AddVideoVersionType("My Scene", VideoAssetTypeOwner::USER, VideoAssetType::EXTRA)};
  const int version{
      m_db.AddVideoVersionType("My Cut", VideoAssetTypeOwner::USER, VideoAssetType::VERSION)};
  const std::string kind{m_db.GetVideoVersionById(VIDEO_EXTRA_ID_BEGIN + 1)};
  ASSERT_FALSE(kind.empty());

  const auto addExtra{[this, idMovie](const std::string& path, int idType)
                      {
                        CFileItem extra{path, false};
                        EXPECT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, idType,
                                                       VideoAssetType::EXTRA, extra));
                        return m_db.GetVideoVersionInfo(path).m_idFile;
                      }};
  const int first{addExtra("/movies/Movie (2020)/Extras/gag reel 1.mkv", made)};
  const int second{addExtra("/movies/Movie (2020)/Extras/gag reel 2.mkv", made)};
  const int third{addExtra("/movies/Movie (2020)/Extras/my scene.mkv", typed)};

  ASSERT_TRUE(m_db.DeleteVideoAsset(first));
  EXPECT_EQ("Gag Reel", m_db.GetVideoVersionById(made));
  ASSERT_TRUE(m_db.DeleteVideoAsset(second));
  EXPECT_EQ("", m_db.GetVideoVersionById(made));

  m_db.SetVideoVersion(third, VIDEO_EXTRA_ID_BEGIN + 1);
  EXPECT_EQ("", m_db.GetVideoVersionById(typed));
  ASSERT_TRUE(m_db.DeleteVideoAsset(third));
  EXPECT_EQ(kind, m_db.GetVideoVersionById(VIDEO_EXTRA_ID_BEGIN + 1));
  EXPECT_EQ("My Cut", m_db.GetVideoVersionById(version));

  strings.Clear();
}

// A name typed for an extra is not offered for others, as one typed for a version is
TEST_F(TestVideoDatabase, TypedExtraNamesAreNotOffered)
{
  ASSERT_GT(m_db.AddVideoVersionType("My Scene", VideoAssetTypeOwner::USER, VideoAssetType::EXTRA),
            0);
  ASSERT_GT(m_db.AddVideoVersionType("My Cut", VideoAssetTypeOwner::USER, VideoAssetType::VERSION),
            0);

  const auto offered{[this](VideoAssetType assetType, const std::string& name)
                     {
                       CFileItemList types;
                       m_db.GetVideoVersionTypes(VideoDbContentType::MOVIES, assetType, types);
                       return std::ranges::any_of(types, [&name](const auto& type)
                                                  { return type->GetLabel() == name; });
                     }};
  EXPECT_FALSE(offered(VideoAssetType::EXTRA, "My Scene"));
  EXPECT_TRUE(offered(VideoAssetType::VERSION, "My Cut"));
}

// What an import needs to restore a disc's extra as an extra, with its playlist
TEST_F(TestVideoDatabase, ExportToXMLWritesABlurayExtra)
{
  const std::string disc{"/movies/Nope (2022)/NOPE.iso"};
  const int idMovie{AddMovie(URIUtils::GetBlurayPlaylistPath(disc, 800))};
  ASSERT_GT(idMovie, 0);
  const int idType{
      m_db.AddVideoVersionType("Gag Reel", VideoAssetTypeOwner::AUTO, VideoAssetType::EXTRA)};
  CFileItem extra{URIUtils::GetBlurayPlaylistPath(disc, 12), false};
  ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, idType, VideoAssetType::EXTRA,
                                 extra));

  const std::string exportPath{CSpecialProtocol::TranslatePath("special://temp/")};
  const std::string exportRoot{URIUtils::AddFileToFolder(
      exportPath, "kodi_videodb_" + CDateTime::GetCurrentDateTime().GetAsDBDate())};
  m_db.ExportToXML(exportPath, true);

  CXBMCTinyXML doc;
  const bool loaded{doc.LoadFile(URIUtils::AddFileToFolder(exportRoot, "videodb.xml"))};
  XFILE::CDirectory::RemoveRecursive(exportRoot);
  ASSERT_TRUE(loaded);

  // The movie comes first, so an import can add the extra to it
  const TiXmlElement* exportedMovie{doc.RootElement()->FirstChildElement("movie")};
  ASSERT_NE(nullptr, exportedMovie);
  int playlist{-1};
  EXPECT_TRUE(XMLUtils::GetInt(exportedMovie, "playlist", playlist));
  EXPECT_EQ(800, playlist);

  const TiXmlElement* exportedExtra{exportedMovie->NextSiblingElement("movie")};
  ASSERT_NE(nullptr, exportedExtra);
  int assetType{-1};
  EXPECT_TRUE(XMLUtils::GetInt(exportedExtra, "videoassettype", assetType));
  EXPECT_EQ(static_cast<int>(VideoAssetType::EXTRA), assetType);
  std::string title;
  EXPECT_TRUE(XMLUtils::GetString(exportedExtra, "videoassettitle", title));
  EXPECT_EQ("Gag Reel", title);
  EXPECT_TRUE(XMLUtils::GetInt(exportedExtra, "playlist", playlist));
  EXPECT_EQ(12, playlist);
  std::string path;
  EXPECT_TRUE(XMLUtils::GetString(exportedExtra, "filenameandpath", path));
  EXPECT_EQ(URIUtils::GetBlurayPlaylistPath(disc, 12), path);
}

TEST_F(TestVideoDatabase, SetFileForMediaRepointsAnExtra)
{
  const int idMovie{AddMovie("/movies/Movie (2020)/movie.mkv")};
  ASSERT_GT(idMovie, 0);

  const std::string disc{"/movies/Movie (2020)/Extras/Disc 1/BDMV/index.bdmv"};
  const int idType{
      m_db.AddVideoVersionType("Disc 1", VideoAssetTypeOwner::AUTO, VideoAssetType::EXTRA)};
  CFileItem extra{disc, false};
  ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, idType, VideoAssetType::EXTRA,
                                 extra));
  const int oldFile{m_db.GetVideoVersionInfo(disc).m_idFile};
  ASSERT_GT(oldFile, 0);

  CStreamDetails streams;
  auto* video{new CStreamDetailVideo()};
  video->m_iDuration = 600;
  streams.AddStream(video);
  ASSERT_TRUE(m_db.SetStreamDetailsForFileId(streams, oldFile));
  ASSERT_TRUE(m_db.SetArtForItem(oldFile, KODI::MEDIA::TYPE::VIDEO_VERSION, "thumb", "thumb.jpg"));

  const std::string playlist{URIUtils::GetBlurayPlaylistPath(disc, 12)};
  m_db.BeginTransaction();
  const int newFile{m_db.SetFileForMedia(playlist, VideoDbContentType::MOVIES, idMovie,
                                         CVideoDatabase::FileRecord{.m_idFile = oldFile})};
  m_db.CommitTransaction();
  ASSERT_GT(newFile, 0);

  const VideoAssetInfo asset{m_db.GetVideoVersionInfo(playlist)};
  EXPECT_EQ(asset.m_idMedia, idMovie);
  EXPECT_EQ(asset.m_assetType, VideoAssetType::EXTRA);
  EXPECT_LT(m_db.GetVideoVersionInfo(disc).m_idFile, 0);

  CStreamDetails moved;
  EXPECT_TRUE(m_db.GetStreamDetails(playlist, moved));
  EXPECT_EQ(moved.GetVideoDuration(), 600);
  KODI::ART::Artwork art;
  EXPECT_TRUE(m_db.GetArtForItem(newFile, KODI::MEDIA::TYPE::VIDEO_VERSION, art));
  EXPECT_EQ(art["thumb"], "thumb.jpg");
}

// A movie restored from its nfos has only the extras they record, so a video without an nfo in its
// extras folder is left out during that scan, and added by a later one
TEST_F(TestVideoDatabase, ARestoredMovieHasOnlyTheExtrasOfItsNfos)
{
  const std::string root{CSpecialProtocol::TranslatePath("special://temp/RestoredExtras/")};
  const std::string folder{URIUtils::AddFileToFolder(root, "Extras/")};
  XFILE::CDirectory::RemoveRecursive(root);
  ASSERT_TRUE(CUtil::CreateDirectoryEx(folder));
  const std::string video{URIUtils::AddFileToFolder(folder, "Interview.mkv")};
  {
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(video));
    file.Close();
  }

  const int idMovie{AddMovie(URIUtils::AddFileToFolder(root, "Movie.mkv"))};
  ASSERT_GT(idMovie, 0);
  KODI::VIDEO::CVideoInfoScannerArt art;
  {
    KODI::VIDEO::CVideoInfoScannerExtras restoringScan{m_db, art};
    restoringScan.SetRestoredFromNfo(idMovie);
    restoringScan.AddVideoExtras(idMovie, folder);
    EXPECT_EQ(m_db.GetVideoVersionInfo(video).m_assetTypeId, -1);
  }

  KODI::VIDEO::CVideoInfoScannerExtras laterScan{m_db, art};
  laterScan.AddVideoExtras(idMovie, folder);
  EXPECT_EQ(m_db.GetVideoVersionInfo(video).m_assetType, VideoAssetType::EXTRA);

  XFILE::CDirectory::RemoveRecursive(root);
}

TEST_F(TestVideoDatabase, ExtrasBesideADiscNoticeANewVideo)
{
  const std::string root{CSpecialProtocol::TranslatePath("special://temp/ScannerExtras/")};
  const std::string folder{URIUtils::AddFileToFolder(root, "Extras/")};
  XFILE::CDirectory::RemoveRecursive(root);
  ASSERT_TRUE(CUtil::CreateDirectoryEx(folder));

  const int idMovie{AddMovie(URIUtils::AddFileToFolder(root, "Movie.mkv"))};
  ASSERT_GT(idMovie, 0);
  const auto addVideo{[this](const std::string& path)
                      {
                        XFILE::CFile file;
                        EXPECT_TRUE(file.OpenForWrite(path));
                        file.Close();
                        return AddMovie(path);
                      }};
  const std::string first{URIUtils::AddFileToFolder(folder, "First.mkv")};
  ASSERT_GT(addVideo(first), 0);

  KODI::VIDEO::CVideoInfoScannerArt art;
  KODI::VIDEO::CVideoInfoScannerExtras scanner{m_db, art};
  EXPECT_TRUE(scanner.AddVideoExtrasBesideDisc(root, true));
  EXPECT_EQ(m_db.GetVideoVersionInfo(first).m_assetType, VideoAssetType::EXTRA);
  std::string previousHash;
  EXPECT_TRUE(m_db.GetPathHash(folder, previousHash));
  EXPECT_FALSE(previousHash.empty());

  const int renamed{
      m_db.AddVideoVersionType("Renamed", VideoAssetTypeOwner::USER, VideoAssetType::EXTRA)};
  m_db.SetVideoVersion(m_db.GetVideoVersionInfo(first).m_idFile, renamed);

  const std::string second{URIUtils::AddFileToFolder(folder, "Second.mkv")};
  EXPECT_GT(addVideo(second), 0);
  EXPECT_TRUE(scanner.AddVideoExtrasBesideDisc(root, false));
  EXPECT_EQ(m_db.GetVideoVersionInfo(second).m_assetType, VideoAssetType::EXTRA);
  EXPECT_EQ(m_db.GetVideoVersionInfo(first).m_assetTypeId, renamed);
  std::string hash;
  EXPECT_TRUE(m_db.GetPathHash(folder, hash));
  EXPECT_NE(hash, previousHash);

  EXPECT_FALSE(scanner.AddVideoExtrasBesideDisc(root, false));
  CFileItemList extras;
  m_db.GetVideoVersions(VideoDbContentType::MOVIES, idMovie, extras, VideoAssetType::EXTRA);
  EXPECT_EQ(extras.Size(), 2);

  // A deleted video is reported too, so that the scanner's clean removes it
  EXPECT_TRUE(XFILE::CFile::Delete(second));
  EXPECT_TRUE(scanner.AddVideoExtrasBesideDisc(root, false));

  EXPECT_TRUE(XFILE::CDirectory::RemoveRecursive(root));
}

// A video in an extras folder made a version of the movie stays one when the folder is scanned
// again, and the movie keeps its own file
TEST_F(TestVideoDatabase, AVersionInAnExtrasFolderStaysAVersion)
{
  const std::string root{CSpecialProtocol::TranslatePath("special://temp/VersionInExtras/")};
  const std::string folder{URIUtils::AddFileToFolder(root, "Extras/")};
  XFILE::CDirectory::RemoveRecursive(root);
  ASSERT_TRUE(CUtil::CreateDirectoryEx(folder));
  const std::string video{URIUtils::AddFileToFolder(folder, "Director's Cut.mkv")};
  {
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(video));
    file.Close();
  }

  const std::string movie{URIUtils::AddFileToFolder(root, "Movie.mkv")};
  const int idMovie{AddMovie(movie)};
  ASSERT_GT(idMovie, 0);
  const int idType{m_db.AddVideoVersionType("Director's Cut", VideoAssetTypeOwner::USER,
                                            VideoAssetType::VERSION)};
  CFileItem version{video, false};
  ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, idType,
                                 VideoAssetType::VERSION, version));

  KODI::VIDEO::CVideoInfoScannerArt art;
  KODI::VIDEO::CVideoInfoScannerExtras scanner{m_db, art};
  scanner.AddVideoExtras(idMovie, folder);

  EXPECT_EQ(m_db.GetVideoVersionInfo(video).m_assetTypeId, idType);
  const VideoAssetInfo own{m_db.GetVideoVersionInfo(movie)};
  EXPECT_EQ(own.m_assetType, VideoAssetType::VERSION);
  EXPECT_EQ(own.m_idMedia, idMovie);

  XFILE::CDirectory::RemoveRecursive(root);
}

// A refresh adds the extras of a movie's extras folders, which a scan passes by where the folder
// hasn't changed since it was last added, as in a library scanned before they were recognised.
// Those of a movie on several discs are beside the discs' folders.
TEST_F(TestVideoDatabase, ARefreshAddsTheExtrasOfAnUnchangedFolder)
{
  const std::string root{CSpecialProtocol::TranslatePath("special://temp/RefreshedExtras/")};
  const std::string folder{URIUtils::AddFileToFolder(root, "Extras/")};
  XFILE::CDirectory::RemoveRecursive(root);
  ASSERT_TRUE(CUtil::CreateDirectoryEx(folder));
  const std::string video{URIUtils::AddFileToFolder(folder, "Interview.mkv")};
  {
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(video));
    file.Close();
  }

  const int idMovie{AddMovie(URIUtils::AddFileToFolder(root, "Disc 1", "Movie.mkv"))};
  ASSERT_GT(idMovie, 0);
  ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
      "UPDATE path SET strContent='movies', strScraper='metadata.local' WHERE idPath=%i",
      m_db.AddPath(root))));

  // A version outside the sources, which a scan never looks at
  const std::string elsewhere{
      CSpecialProtocol::TranslatePath("special://temp/RefreshedElsewhere/")};
  const std::string elsewhereExtra{URIUtils::AddFileToFolder(elsewhere, "Extras", "Other.mkv")};
  XFILE::CDirectory::RemoveRecursive(elsewhere);
  ASSERT_TRUE(CUtil::CreateDirectoryEx(URIUtils::GetDirectory(elsewhereExtra)));
  {
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(elsewhereExtra));
    file.Close();
  }
  CFileItem version{URIUtils::AddFileToFolder(elsewhere, "Movie Extended.mkv"), false};
  ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, VIDEO_VERSION_ID_DEFAULT,
                                 VideoAssetType::VERSION, version));

  // The folder's hash is that of its extras, as the scanner stores it
  CFileItemList extras;
  KODI::VIDEO::UTILS::EnumerateVideoExtras(folder, [&extras](const std::shared_ptr<CFileItem>& item)
                                           { extras.Add(item); });
  extras.Sort(SortBy::FILE, SortOrder::ASCENDING, SortAttributeNone);
  std::string hash;
  KODI::VIDEO::UTILS::GetPathHash(extras, hash);
  m_db.SetPathHash(folder, hash);

  KODI::VIDEO::CVideoInfoScannerArt art;
  KODI::VIDEO::CVideoInfoScannerExtras scanner{m_db, art};
  scanner.AddVideoExtrasBesideDisc(root, true);
  EXPECT_EQ(m_db.GetVideoVersionInfo(video).m_assetTypeId, -1);

  // Without folder names a movie has no extras folders
  scanner.AddMovieExtras(idMovie, false);
  EXPECT_EQ(m_db.GetVideoVersionInfo(video).m_assetTypeId, -1);

  scanner.AddMovieExtras(idMovie, true);
  const VideoAssetInfo extra{m_db.GetVideoVersionInfo(video)};
  EXPECT_EQ(extra.m_assetType, VideoAssetType::EXTRA);
  EXPECT_EQ(extra.m_idMedia, idMovie);
  EXPECT_EQ(m_db.GetVideoVersionInfo(elsewhereExtra).m_assetTypeId, -1);

  XFILE::CDirectory::RemoveRecursive(root);
  XFILE::CDirectory::RemoveRecursive(elsewhere);
}

// The search's path for an extra ends in its file id, so the extra is played from its file
TEST_F(TestVideoDatabase, AnExtraFoundBySearchPlaysItsFile)
{
  const std::string disc{"/movies/Movie (2000)/BDMV/index.bdmv"};
  const int idMovie{AddMovie(URIUtils::GetBlurayPlaylistPath(disc, 800))};
  ASSERT_GT(idMovie, 0);
  const std::string playlist{URIUtils::GetBlurayPlaylistPath(disc, 802)};
  CFileItem extra{playlist, false};
  ASSERT_TRUE(m_db.AddVideoAsset(
      VideoDbContentType::MOVIES, idMovie,
      m_db.AddVideoVersionType("Gag Reel", VideoAssetTypeOwner::AUTO, VideoAssetType::EXTRA),
      VideoAssetType::EXTRA, extra));

  CFileItemList items;
  m_db.GetMovieExtrasByName("Gag", items);
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(items[0]->GetDynPath(), playlist);
}
