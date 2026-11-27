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
#include "addons/Scraper.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "cores/VideoSettings.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "imagefiles/ImageFileURL.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "test/TestAnnouncements.h"
#include "test/TestLibrary.h"
#include "test/TestUtils.h"
#include "utils/Artwork.h"
#include "utils/DatabaseUtils.h"
#include "utils/FileUtils.h"
#include "utils/ItemProperties.h"
#include "utils/StreamDetails.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "video/Bookmark.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbUrl.h"
#include "video/VideoInfoScanner.h"
#include "video/VideoInfoTag.h"
#include "video/VideoManagerTypes.h"
#include "video/jobs/VideoLibraryMarkWatchedJob.h"
#include "video/jobs/VideoLibraryResetResumePointJob.h"
#include "video/test/VideoDatabaseTestBase.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace
{
class TestVideoDatabase : public VideoDatabaseTestBase
{
protected:
  TestVideoDatabase() : VideoDatabaseTestBase("TestVideoDatabase", "/videos/") {}

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
    return KODI::TEST::VideoTag(m_db, fileAndPath);
  }

  int AddMovie(const std::string& fileAndPath) { return KODI::TEST::AddMovie(m_db, fileAndPath); }

  int AddTvShow(const std::string& showPath)
  {
    return KODI::TEST::AddTvShow(m_db, "Show", {showPath});
  }

  int AddTvShow(const std::string& title,
                const std::vector<std::string>& folders,
                const CVideoInfoTag& details = {})
  {
    return KODI::TEST::AddTvShow(m_db, title, folders, details);
  }

  int AddEpisode(int idShow, const std::string& fileAndPath, int episode)
  {
    return KODI::TEST::AddEpisode(m_db, idShow, fileAndPath, episode);
  }

  //! The number of rows of \p table whose \p column is \p id
  int Rows(const std::string& table, const std::string& column, int id)
  {
    return m_db.GetSingleValueInt(
        m_db.PrepareSQL("SELECT COUNT(*) FROM %s WHERE %s=%i", table.c_str(), column.c_str(), id));
  }

  //! Dates the file or folder two days back, so that a date read once it is gone, which is now,
  //! can't be taken for it
  static void MakeTwoDaysOld(std::string path)
  {
    URIUtils::RemoveSlashAtEnd(path);
    const std::filesystem::path native{
        std::u8string{reinterpret_cast<const char8_t*>(path.data()), path.size()}};
    std::filesystem::last_write_time(native, std::filesystem::last_write_time(native) -
                                                 std::chrono::hours{48});
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

  // The test environment has no announcement manager, and e.g. ExportToXML() announces.
  // An unstarted one only queues the announcements.
  const CScopedAnnouncementManager m_announcementManager;
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
  const auto unknown = AddItem(items, "/more/unknown.mkv");

  EXPECT_TRUE(m_db.GetPlayCounts(items.GetPath(), items));

  EXPECT_EQ(1, plain->GetVideoInfoTag()->GetPlayCount());
  EXPECT_EQ(400.0, other->GetVideoInfoTag()->GetResumePoint().timeInSeconds);
  EXPECT_EQ(1, archivedItem->GetVideoInfoTag()->GetPlayCount());
  EXPECT_FALSE(unknown->HasVideoInfoTag());
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

TEST_F(TestVideoDatabase, OrientationAndCenterMixLevelAreStoredAndRestored)
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

  const CVideoSettings defaults;
  m_db.SetVideoSettings(idFile, defaults);

  ASSERT_TRUE(m_db.GetVideoSettings(idFile, stored));
  EXPECT_EQ(defaults.m_Orientation, stored.m_Orientation);
  EXPECT_EQ(defaults.m_CenterMixLevel, stored.m_CenterMixLevel);
}

TEST_F(TestVideoDatabase, AStreamLanguageIsStoredAsItsWholeTag)
{
  const int idFile = m_db.AddFile("/videos/languages.mkv");
  ASSERT_GT(idFile, 0);

  auto* video = new CStreamDetailVideo();
  video->m_language = KODI::LANGUAGE::CLanguageTag::Parse("en-AU");
  auto* audio = new CStreamDetailAudio();
  audio->m_language = KODI::LANGUAGE::CLanguageTag::Parse("pt-BR");
  auto* subtitle = new CStreamDetailSubtitle();
  subtitle->m_language = KODI::LANGUAGE::CLanguageTag::Parse("zh-Hant");
  CStreamDetails details;
  details.AddStream(video);
  details.AddStream(audio);
  details.AddStream(subtitle);
  ASSERT_TRUE(m_db.SetStreamDetailsForFileId(details, idFile));

  CVideoInfoTag stored;
  stored.m_iFileId = idFile;
  ASSERT_TRUE(m_db.GetStreamDetails(stored));
  EXPECT_EQ("en-AU", stored.m_streamDetails.GetVideoLanguage(1).ToString());
  EXPECT_EQ("pt-BR", stored.m_streamDetails.GetAudioLanguage(1).ToString());
  EXPECT_EQ("zh-Hant", stored.m_streamDetails.GetSubtitleLanguage(1).ToString());
}

TEST_F(TestVideoDatabase, AStoredViewModeSinceRemovedReadsAsNormal)
{
  const int idFile = m_db.AddFile("/videos/zoomed.mkv");
  ASSERT_GT(idFile, 0);

  CVideoSettings zoomed;
  zoomed.m_ViewMode = 1; // Zoom, before it was removed
  m_db.SetVideoSettings(idFile, zoomed);

  CVideoSettings stored;
  ASSERT_TRUE(m_db.GetVideoSettings(idFile, stored));
  EXPECT_EQ(ViewModeNormal, stored.m_ViewMode);

  zoomed.m_ViewMode = ViewModeStretch16x9;
  m_db.SetVideoSettings(idFile, zoomed);
  ASSERT_TRUE(m_db.GetVideoSettings(idFile, stored));
  EXPECT_EQ(ViewModeStretch16x9, stored.m_ViewMode);

  zoomed.m_ViewMode = ViewModeStretchToFill;
  m_db.SetVideoSettings(idFile, zoomed);
  ASSERT_TRUE(m_db.GetVideoSettings(idFile, stored));
  EXPECT_EQ(ViewModeStretchToFill, stored.m_ViewMode);
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
  directed.SetDirector({"Jane Director"});
  const int idShow{AddTvShow("Directed", {"/tv/directed/"}, directed)};
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

TEST_F(TestVideoDatabase, ASmartPlaylistRuleFindsTheMoviesItMatches)
{
  CVideoInfoTag other{Tag("/videos/other.mkv")};
  other.SetRating(3.0f, 100, "imdb", true);
  ASSERT_GT(m_db.SetDetailsForMovie(other, KODI::ART::Artwork{}), 0);

  struct Match
  {
    void (*details)(CVideoInfoTag& movie);
    std::string rule;
  };
  const Match matches[]{
      {[](CVideoInfoTag& movie) { movie.SetGenre({"Drama"}); },
       R"({"field":"genre","operator":"is","value":"Drama"})"},
      {[](CVideoInfoTag& movie) { movie.SetRating(8.0f, 100, "imdb", true); },
       R"({"field":"rating","operator":"greaterthan","value":"5.5"})"},
      {[](CVideoInfoTag& movie) { movie.SetRating(8.0f, 100, "imdb", true); },
       R"({"field":"rating","operator":"between","value":["5","9"]})"},
  };
  int row{0};
  for (const auto& [details, rule] : matches)
  {
    CVideoInfoTag matching{Tag("/videos/matching " + std::to_string(++row) + ".mkv")};
    details(matching);
    const int idMatching{m_db.SetDetailsForMovie(matching, KODI::ART::Artwork{})};
    ASSERT_GT(idMatching, 0) << rule;

    CVideoDbUrl url;
    ASSERT_TRUE(url.FromString("videodb://movies/titles/"));
    url.AddOption("xsp", R"({"type":"movies","rules":{"and":[)" + rule + "]}}");

    CFileItemList items;
    ASSERT_TRUE(m_db.GetMoviesByWhere(url.ToString(), CDatabase::Filter(), items)) << rule;
    ASSERT_EQ(1, items.Size()) << rule;
    EXPECT_EQ(idMatching, items[0]->GetVideoInfoTag()->m_iDbId) << rule;
    ASSERT_TRUE(m_db.DeleteMovie(idMatching)) << rule;
  }
}

TEST_F(TestVideoDatabase, ANumberRuleWhoseValueIsNotANumberFailsTheListing)
{
  ASSERT_GT(AddMovie("/videos/movie.mkv"), 0);

  const std::string rule{R"({"field":"rating","operator":"greaterthan","value":"0 OR 1=1"})"};
  CVideoDbUrl url;
  ASSERT_TRUE(url.FromString("videodb://movies/titles/"));
  url.AddOption("xsp", R"({"type":"movies","rules":{"and":[)" + rule + "]}}");

  CFileItemList items;
  EXPECT_FALSE(m_db.GetMoviesByWhere(url.ToString(), CDatabase::Filter(), items));
  EXPECT_TRUE(items.IsEmpty());
}

// A movie listing takes a smart playlist's order and limit, as the other types' listings do
TEST_F(TestVideoDatabase, ASmartPlaylistsOrderAndLimitApplyToAMovieListing)
{
  for (const char* name : {"C.mkv", "A.mkv", "B.mkv"})
    ASSERT_GT(AddMovie(std::string{"/movies/"} + name), 0);

  CVideoDbUrl url;
  ASSERT_TRUE(url.FromString("videodb://movies/titles/"));
  url.AddOption(
      "xsp", R"({"type":"movies","order":{"method":"title","direction":"ascending"},"limit":2})");

  CFileItemList items;
  ASSERT_TRUE(m_db.GetMoviesByWhere(url.ToString(), CDatabase::Filter(), items));
  ASSERT_EQ(2, items.Size());
  EXPECT_EQ("A.mkv", items[0]->GetVideoInfoTag()->m_strTitle);
  EXPECT_EQ("B.mkv", items[1]->GetVideoInfoTag()->m_strTitle);
}

TEST_F(TestVideoDatabase, AListItemHasTheDetailsOfTheItemLoadedAlone)
{
  const auto addDetails = [](CVideoInfoTag& tag, const std::string& name)
  {
    for (const std::string& actor : {"Zed " + name, "Amy " + name})
    {
      SActorInfo info;
      info.strName = actor;
      info.strRole = "Role";
      info.order = 0;
      tag.m_cast.push_back(info);
    }
    tag.SetTags({"Tag " + name});
    tag.SetRating(7.5f, 100, "imdb", true);
    tag.SetUniqueID("id " + name, "imdb", true);
    auto* video = new CStreamDetailVideo();
    video->m_iDuration = 5400;
    video->SetSource(CStreamDetail::MEDIA);
    tag.m_streamDetails.AddStream(video);
    tag.m_streamDetails.DetermineBestStreams();
  };

  CVideoInfoTag show;
  addDetails(show, "show");
  const int idShow{AddTvShow("Show", {"/tv/Show/"}, show)};
  ASSERT_GT(idShow, 0);
  for (const int episode : {1, 2})
  {
    CVideoInfoTag tag{Tag("/tv/Show/S01E0" + std::to_string(episode) + ".mkv")};
    tag.m_iSeason = 1;
    tag.m_iEpisode = episode;
    addDetails(tag, "episode " + std::to_string(episode));
    ASSERT_GT(m_db.SetDetailsForEpisode(tag, KODI::ART::Artwork{}, idShow), 0);
  }
  for (const std::string name : {"one", "two"})
  {
    CVideoInfoTag tag{Tag("/movies/" + name + ".mkv")};
    addDetails(tag, name);
    const int idMovie{m_db.SetDetailsForMovie(tag, KODI::ART::Artwork{})};
    ASSERT_GT(idMovie, 0);
    ASSERT_TRUE(m_db.LinkMovieToTvshow(idMovie, idShow, false));
  }

  const auto serialized = [](const CVideoInfoTag& tag)
  {
    CVariant value;
    tag.Serialize(value);
    return value;
  };

  CFileItemList movies;
  ASSERT_TRUE(m_db.GetMoviesByWhere("videodb://movies/titles/", CDatabase::Filter(), movies, {},
                                    VideoDbDetailsAll));
  ASSERT_EQ(2, movies.Size());
  for (const auto& item : movies)
  {
    CVideoInfoTag alone;
    ASSERT_TRUE(m_db.GetMovieInfo("", alone, item->GetVideoInfoTag()->m_iDbId));
    EXPECT_TRUE(serialized(alone) == serialized(*item->GetVideoInfoTag())) << item->GetLabel();
  }

  CFileItemList shows;
  ASSERT_TRUE(m_db.GetTvShowsByWhere("videodb://tvshows/titles/", CDatabase::Filter(), shows, {},
                                     VideoDbDetailsAll));
  ASSERT_EQ(1, shows.Size());
  CVideoInfoTag showAlone;
  ASSERT_TRUE(m_db.GetTvShowInfo("", showAlone, idShow));
  EXPECT_TRUE(serialized(showAlone) == serialized(*shows[0]->GetVideoInfoTag()));

  CFileItemList episodes;
  ASSERT_TRUE(m_db.GetEpisodesByWhere("videodb://tvshows/titles/" + std::to_string(idShow) + "/1/",
                                      CDatabase::Filter(), episodes, {}, VideoDbDetailsAll));
  ASSERT_EQ(2, episodes.Size());
  for (const auto& item : episodes)
  {
    CVideoInfoTag alone;
    ASSERT_TRUE(m_db.GetEpisodeInfo("", alone, item->GetVideoInfoTag()->m_iDbId));
    EXPECT_TRUE(serialized(alone) == serialized(*item->GetVideoInfoTag())) << item->GetLabel();
  }

  // Their ids are the movies', which a list must not take for movies linked to the show
  for (const std::string name : {"one", "two"})
  {
    CVideoInfoTag tag{Tag("/musicvideos/" + name + ".mkv")};
    addDetails(tag, "music video " + name);
    ASSERT_GT(m_db.SetDetailsForMusicVideo(tag, KODI::ART::Artwork{}), 0);
  }
  CFileItemList musicVideos;
  ASSERT_TRUE(m_db.GetMusicVideosByWhere("videodb://musicvideos/titles/", CDatabase::Filter(),
                                         musicVideos, {}, VideoDbDetailsAll));
  ASSERT_EQ(2, musicVideos.Size());
  for (const auto& item : musicVideos)
  {
    CVideoInfoTag alone;
    ASSERT_TRUE(m_db.GetMusicVideoInfo("", alone, item->GetVideoInfoTag()->m_iDbId));
    EXPECT_TRUE(serialized(alone) == serialized(*item->GetVideoInfoTag())) << item->GetLabel();
    EXPECT_TRUE(item->GetVideoInfoTag()->m_showLink.empty()) << item->GetLabel();
  }
}

namespace
{
std::string Overlay(CGUIListItem::GUIIconOverlay overlay)
{
  CFileItem item;
  item.SetOverlayImage(overlay);
  return item.GetOverlayImage();
}
} // namespace

// What each type's listing gives a skin or a client for an item

TEST_F(TestVideoDatabase, AMovieListedIsItsFileAtItsId)
{
  const int idMovie{AddMovie("/movies/Film.mkv")};
  ASSERT_GT(idMovie, 0);
  MarkPlayed("/movies/Film.mkv", 1);

  CFileItemList items;
  ASSERT_TRUE(m_db.GetMoviesByWhere("videodb://movies/titles/", CDatabase::Filter(), items));
  ASSERT_EQ(1, items.Size());
  const CFileItem& movie{*items[0]};
  EXPECT_EQ("videodb://movies/titles/" + std::to_string(idMovie), movie.GetPath());
  EXPECT_EQ("/movies/Film.mkv", movie.GetDynPath());
  EXPECT_FALSE(movie.IsFolder());
  EXPECT_EQ("Film.mkv", movie.GetLabel());
  EXPECT_EQ(Overlay(CGUIListItem::ICON_OVERLAY_WATCHED), movie.GetOverlayImage());
}

TEST_F(TestVideoDatabase, AMovieWithVersionsListsAsAFolderOfThemOrAsEachVersion)
{
  const MovieWithVersion movie{AddMovieWithVersion(KODI::ART::Artwork{}, KODI::ART::Artwork{})};
  ASSERT_GT(movie.versionFileId, 0);
  const std::string moviePath{"videodb://movies/titles/" + std::to_string(movie.movieId)};

  const auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};
  const CScopedSetting flatten{CSettings::SETTING_VIDEOLIBRARY_FLATTENVERSIONS};
  CFileItemList folder;
  settings->SetBool(CSettings::SETTING_VIDEOLIBRARY_FLATTENVERSIONS, false);
  const bool listedFolder{
      m_db.GetMoviesByWhere("videodb://movies/titles/", CDatabase::Filter(), folder)};
  CFileItemList versions;
  settings->SetBool(CSettings::SETTING_VIDEOLIBRARY_FLATTENVERSIONS, true);
  const bool listedVersions{
      m_db.GetMoviesByWhere("videodb://movies/titles/", CDatabase::Filter(), versions)};

  ASSERT_TRUE(listedFolder);
  ASSERT_EQ(1, folder.Size());
  EXPECT_EQ(moviePath + "/-2/", folder[0]->GetPath());
  EXPECT_TRUE(folder[0]->IsFolder());
  EXPECT_TRUE(folder[0]->GetProperty(KODI::ITEM::PROPERTY::IS_HYBRID_FOLDER).asBoolean());

  ASSERT_TRUE(listedVersions);
  ASSERT_EQ(2, versions.Size());
  for (const auto& item : versions)
  {
    EXPECT_EQ(moviePath + "/1/" + std::to_string(item->GetVideoInfoTag()->m_iFileId) +
                  "?assetType=1",
              item->GetPath());
    EXPECT_FALSE(item->IsFolder());
  }
}

TEST_F(TestVideoDatabase, AMoviesAssetsListAsTheAssetsUnderTheMoviesTitle)
{
  CVideoInfoTag film{Tag("/movies/Film/Film.mkv")};
  film.m_strTitle = "Film";
  const int idMovie{m_db.SetDetailsForMovie(film, KODI::ART::Artwork{})};
  ASSERT_GT(idMovie, 0);
  const int idExtended{AddAsset(idMovie, "/movies/Film/Film Extended.mkv")};
  ASSERT_GT(idExtended, 0);

  CVideoDbUrl url;
  ASSERT_TRUE(url.FromString("videodb://movies/titles/"));
  url.AddOption("movieid", idMovie);
  url.AddOption("assetType", static_cast<int>(VideoAssetType::VERSION));
  CFileItemList items;
  ASSERT_TRUE(m_db.GetMoviesByWhere(url.ToString(), CDatabase::Filter(), items));

  ASSERT_EQ(2, items.Size());
  EXPECT_EQ("Film", items.GetProperty(KODI::ITEM::PROPERTY::CUSTOM_TITLE).asString());
  for (const auto& item : items)
  {
    const CVideoInfoTag& asset{*item->GetVideoInfoTag()};
    EXPECT_EQ(asset.GetAssetInfo().GetTitle(), item->GetLabel());
    EXPECT_EQ(asset.m_iFileId != idExtended, item->IsSelected());
    EXPECT_NE(std::string::npos,
              item->GetPath().find("videodb://movies/titles/" + std::to_string(asset.m_iFileId)))
        << item->GetPath();
    if (asset.m_iFileId == idExtended)
      EXPECT_EQ("Extended", item->GetLabel());
  }
}

TEST_F(TestVideoDatabase, AnEpisodeListedIsItsIdUnderItsSeasonAndItsShowSeasonAndIdElsewhere)
{
  const int idShow{AddTvShow("/shows/Show/")};
  ASSERT_GT(idShow, 0);
  CVideoInfoTag tag{Tag("/shows/Show/Show S01E02.mkv")};
  tag.m_strTitle = "Pilot";
  tag.m_iSeason = 1;
  tag.m_iEpisode = 2;
  tag.m_firstAired.SetFromDBDate("2026-01-05");
  const int idEpisode{m_db.SetDetailsForEpisode(tag, KODI::ART::Artwork{}, idShow)};
  ASSERT_GT(idEpisode, 0);
  const std::string show{"videodb://tvshows/titles/" + std::to_string(idShow) + "/"};
  const std::string episode{std::to_string(idEpisode)};

  CFileItemList inSeason;
  ASSERT_TRUE(m_db.GetEpisodesByWhere(show + "1/", CDatabase::Filter(), inSeason));
  ASSERT_EQ(1, inSeason.Size());
  EXPECT_EQ(show + "1/" + episode + "?season=1&tvshowid=" + std::to_string(idShow),
            inSeason[0]->GetPath());
  EXPECT_EQ("/shows/Show/Show S01E02.mkv", inSeason[0]->GetDynPath());
  EXPECT_EQ("1x02. Pilot", inSeason[0]->GetLabel());
  EXPECT_EQ("2026-01-05", inSeason[0]->GetDateTime().GetAsDBDate());

  CFileItemList everywhere;
  ASSERT_TRUE(
      m_db.GetEpisodesByWhere("videodb://tvshows/titles/", CDatabase::Filter(), everywhere));
  ASSERT_EQ(1, everywhere.Size());
  EXPECT_EQ(show + "1/" + episode, everywhere[0]->GetPath());
}

TEST_F(TestVideoDatabase, ATvShowListedIsAFolderWithItsEpisodeCounts)
{
  const int idShow{AddTvShow("/shows/Show/")};
  ASSERT_GT(idShow, 0);
  ASSERT_GT(AddEpisode(idShow, "/shows/Show/Show S01E01.mkv", 1), 0);
  ASSERT_GT(AddEpisode(idShow, "/shows/Show/Show S01E02.mkv", 2), 0);
  const int idEmpty{AddTvShow("Empty", {"/shows/Empty/"})};
  ASSERT_GT(idEmpty, 0);
  MarkPlayed("/shows/Show/Show S01E01.mkv", 1);
  MarkPlayed("/shows/Show/Show S01E02.mkv", 0, 300.0);

  const auto listed{[this](int id)
                    {
                      CFileItemList items;
                      EXPECT_TRUE(m_db.GetTvShowsByWhere("videodb://tvshows/titles/",
                                                         CDatabase::Filter(), items));
                      for (const auto& item : items)
                      {
                        if (item->GetVideoInfoTag()->m_iDbId == id)
                          return item;
                      }
                      return std::shared_ptr<CFileItem>{};
                    }};

  const auto show{listed(idShow)};
  ASSERT_TRUE(show);
  EXPECT_EQ("videodb://tvshows/titles/" + std::to_string(idShow) + "/", show->GetPath());
  EXPECT_TRUE(show->IsFolder());
  EXPECT_EQ(1, show->GetProperty(KODI::ITEM::PROPERTY::TOTAL_SEASONS).asInteger());
  EXPECT_EQ(2, show->GetProperty(KODI::ITEM::PROPERTY::NUM_EPISODES).asInteger());
  EXPECT_EQ(1, show->GetProperty(KODI::ITEM::PROPERTY::WATCHED_EPISODES).asInteger());
  EXPECT_EQ(1, show->GetProperty(KODI::ITEM::PROPERTY::IN_PROGRESS_EPISODES).asInteger());
  EXPECT_EQ(Overlay(CGUIListItem::ICON_OVERLAY_UNWATCHED), show->GetOverlayImage());

  // A show is watched once all of its episodes are, which a show without any never is
  MarkPlayed("/shows/Show/Show S01E02.mkv", 1);
  const auto watchedShow{listed(idShow)};
  ASSERT_TRUE(watchedShow);
  EXPECT_EQ(Overlay(CGUIListItem::ICON_OVERLAY_WATCHED), watchedShow->GetOverlayImage());
  const auto emptyShow{listed(idEmpty)};
  ASSERT_TRUE(emptyShow);
  EXPECT_EQ(Overlay(CGUIListItem::ICON_OVERLAY_UNWATCHED), emptyShow->GetOverlayImage());
}

TEST_F(TestVideoDatabase, AMusicVideosCastIsLoadedOnlyWhenAskedFor)
{
  CVideoInfoTag tag{Tag("/musicvideos/Song.mkv")};
  SActorInfo actor;
  actor.strName = "Singer";
  tag.m_cast.push_back(actor);
  tag.SetTags({"Live"});
  const int idVideo{m_db.SetDetailsForMusicVideo(tag, KODI::ART::Artwork{})};
  ASSERT_GT(idVideo, 0);

  CVideoInfoTag tags;
  ASSERT_TRUE(m_db.GetMusicVideoInfo("", tags, idVideo, VideoDbDetailsTag));
  EXPECT_EQ(std::vector<std::string>{"Live"}, tags.m_tags);
  EXPECT_TRUE(tags.m_cast.empty());

  CVideoInfoTag cast;
  ASSERT_TRUE(m_db.GetMusicVideoInfo("", cast, idVideo, VideoDbDetailsCast));
  ASSERT_EQ(1u, cast.m_cast.size());
  EXPECT_EQ("Singer", cast.m_cast[0].strName);
}

TEST_F(TestVideoDatabase, AMusicVideoListedIsItsIdAndAnArtistsListIsTitledWithTheirName)
{
  CVideoInfoTag tag{Tag("/musicvideos/Song.mkv")};
  tag.m_artist = {"Singer"};
  const int idVideo{m_db.SetDetailsForMusicVideo(tag, KODI::ART::Artwork{})};
  ASSERT_GT(idVideo, 0);

  CFileItemList items;
  ASSERT_TRUE(
      m_db.GetMusicVideosByWhere("videodb://musicvideos/titles/", CDatabase::Filter(), items));
  ASSERT_EQ(1, items.Size());
  EXPECT_EQ("videodb://musicvideos/titles/" + std::to_string(idVideo), items[0]->GetPath());
  EXPECT_EQ("/musicvideos/Song.mkv", items[0]->GetDynPath());
  EXPECT_FALSE(items.HasProperty(KODI::ITEM::PROPERTY::CUSTOM_TITLE));

  CVideoDbUrl url;
  ASSERT_TRUE(url.FromString("videodb://musicvideos/titles/"));
  url.AddOption("artistid",
                m_db.GetSingleValueInt("SELECT actor_id FROM actor WHERE name = 'Singer'"));
  CFileItemList byArtist;
  ASSERT_TRUE(m_db.GetMusicVideosByWhere(url.ToString(), CDatabase::Filter(), byArtist));
  ASSERT_EQ(1, byArtist.Size());
  EXPECT_EQ("Singer", byArtist.GetProperty(KODI::ITEM::PROPERTY::CUSTOM_TITLE).asString());
}

TEST_F(TestVideoDatabase, AListingsTotalCountsEveryMatchNotOnlyThoseItsLimitReturns)
{
  for (const char* name : {"C.mkv", "A.mkv", "B.mkv"})
    ASSERT_GT(AddMovie(std::string{"/movies/"} + name), 0);

  SortDescription limited;
  limited.limitEnd = 2;
  CFileItemList unsorted;
  ASSERT_TRUE(
      m_db.GetMoviesByWhere("videodb://movies/titles/", CDatabase::Filter(), unsorted, limited));
  EXPECT_EQ(2, unsorted.Size());
  EXPECT_EQ(3, unsorted.GetProperty("total").asInteger());

  limited.sortBy = SortBy::TITLE;
  CFileItemList sorted;
  ASSERT_TRUE(
      m_db.GetMoviesByWhere("videodb://movies/titles/", CDatabase::Filter(), sorted, limited));
  ASSERT_EQ(2, sorted.Size());
  EXPECT_EQ(3, sorted.GetProperty("total").asInteger());
  EXPECT_EQ("A.mkv", sorted[0]->GetLabel());
}

TEST_F(TestVideoDatabase, ListArtIsTheArtOfEachItemLoadedAlone)
{
  CVideoInfoTag one{Tag("/movies/one.mkv")};
  const int idOne{m_db.SetDetailsForMovie(
      one, KODI::ART::Artwork{{"poster", "one.jpg"}, {"fanart", "one-fanart.jpg"}})};
  CVideoInfoTag two{Tag("/movies/two.mkv")};
  const int idTwo{m_db.SetDetailsForMovie(two, KODI::ART::Artwork{})};
  ASSERT_GT(idOne, 0);
  ASSERT_GT(idTwo, 0);

  std::map<int, KODI::ART::Artwork> art;
  ASSERT_TRUE(m_db.GetArtForItems("movie", {idOne, idTwo}, art));
  for (const int id : {idOne, idTwo})
  {
    KODI::ART::Artwork alone;
    ASSERT_TRUE(m_db.GetArtForItem(id, "movie", alone));
    EXPECT_EQ(alone, art[id]) << id;
  }
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
    const auto e03 = AddItem(items, ArchivePath(protocol, "/tv/season.zip", "e03.mkv"));

    EXPECT_TRUE(m_db.GetPlayCounts(items.GetPath(), items)) << protocol;
    EXPECT_EQ(1, e01->GetVideoInfoTag()->GetPlayCount()) << protocol;
    EXPECT_EQ(300.0, e02->GetVideoInfoTag()->GetResumePoint().timeInSeconds) << protocol;
    EXPECT_FALSE(e03->HasVideoInfoTag()) << protocol;
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

TEST(TestVideoDatabasePaths, ToStoredPathAddsTheTrailingSeparator)
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

// Of the source itself, of a folder below it with a path of its own, and of one without
TEST_F(TestVideoDatabase, GetSourcePathReadsTheScanSettingsOfAParentSource)
{
  const ADDON::AddonInfoPtr info{
      ADDON::CAddonInfoBuilder::Generate("metadata.test", ADDON::AddonType::SCRAPER_MOVIES)};
  const auto scraper{std::make_shared<ADDON::CScraper>(info, ADDON::AddonType::SCRAPER_MOVIES)};

  for (const auto& [source, parentName, recurse] :
       {std::tuple{"smb://server/movies/", false, 2}, std::tuple{"/movies/", true, 0}})
  {
    KODI::VIDEO::SScanSettings stored;
    stored.parent_name = parentName;
    stored.recurse = recurse;
    m_db.SetScraperForPath(source, scraper, stored);
    const std::string withPath{std::string{source} + "Film/"};
    ASSERT_GT(m_db.AddPath(withPath), 0);

    for (const std::string& path :
         {std::string{source}, withPath, std::string{source} + "genre/film/"})
    {
      KODI::VIDEO::SScanSettings read;
      std::string sourcePath;
      ASSERT_TRUE(m_db.GetSourcePath(path, sourcePath, read)) << path;
      EXPECT_EQ(source, sourcePath) << path;
      EXPECT_EQ(parentName, read.parent_name) << path;
      EXPECT_EQ(recurse, read.recurse) << path;
    }
  }
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
  const std::string root{Folder(m_root, "Movie (2020)")};
  const std::string bdmv{Folder(Folder(root, "Disc 1"), "BDMV")};
  const std::string disc{File(bdmv, "index.bdmv")};
  File(Folder(bdmv, "PLAYLIST"), "00801.mpls");
  const std::string trailer{File(Folder(root, "Extras"), "trailer.mkv")};

  const int idMovie{AddMovie(disc)};
  ASSERT_GT(idMovie, 0);
  ASSERT_GT(AddAsset(idMovie, URIUtils::GetBlurayPlaylistPath(disc, 801)), 0);
  ASSERT_GT(AddAsset(idMovie, trailer, VideoAssetType::EXTRA, "Trailer", VideoAssetTypeOwner::AUTO),
            0);

  m_db.ExportToXML(root, false);

  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.LoadFile(URIUtils::AddFileToFolder(bdmv, "index.nfo")));
  std::vector<int> playlists;
  for (const TiXmlElement* movie{doc.RootElement()->FirstChildElement("movie")}; movie;
       movie = movie->NextSiblingElement("movie"))
  {
    int playlist{-1};
    XMLUtils::GetInt(movie, "playlist", playlist);
    playlists.emplace_back(playlist);
  }
  EXPECT_EQ(2u, playlists.size());
  EXPECT_NE(playlists.end(), std::ranges::find(playlists, 801));
}

// The converted movie's file is kept as the version, so its streamdetails and art must be kept too
TEST_F(TestVideoDatabase, ConvertVideoToVersionKeepsStreamDetailsAndArt)
{
  const auto addMovie =
      [this](const std::string& fileAndPath, int duration, const KODI::ART::Artwork& art)
  {
    CVideoInfoTag tag{Tag(fileAndPath)};
    auto* video = new CStreamDetailVideo();
    video->m_iDuration = duration;
    video->SetSource(CStreamDetail::MEDIA);
    tag.m_streamDetails.AddStream(video);
    tag.m_streamDetails.DetermineBestStreams();
    return m_db.SetDetailsForMovie(tag, art);
  };

  const std::string source{"/movies/Movie (2010)/Movie (2010) Standard Edition.mkv"};
  const KODI::ART::Artwork sourceArt{{"poster", "source-poster.jpg"}};
  const int targetId{addMovie("/movies/Movie (2010)/Movie (2010) Extended Edition.mkv", 6500,
                              {{"poster", "target-poster.jpg"}, {"fanart", "target-fanart.jpg"}})};
  const int sourceId{addMovie(source, 6019, sourceArt)};
  ASSERT_GT(targetId, 0);
  ASSERT_GT(sourceId, 0);
  const int sourceFileId{m_db.GetFileIdByMovie(sourceId)};

  ASSERT_TRUE(m_db.ConvertVideoToVersion(VideoDbContentType::MOVIES, sourceId, targetId, -1,
                                         VideoAssetType::VERSION));

  CStreamDetails details;
  EXPECT_TRUE(m_db.GetStreamDetails(source, details));
  EXPECT_EQ(6019, details.GetVideoDuration());
  KODI::ART::Artwork versionArt;
  ASSERT_TRUE(m_db.GetArtForItem(sourceFileId, KODI::MEDIA::TYPE::VIDEO_VERSION, versionArt));
  EXPECT_EQ(sourceArt, versionArt);
}

// A failed refresh relies on this to remove the assets of the movie it has already deleted
TEST_F(TestVideoDatabase, DeleteMovieRemovesTheAssetsOfADeletedMovie)
{
  const int idMovie{AddMovie("/movies/Movie (2010)/Movie (2010).mkv")};
  ASSERT_GT(idMovie, 0);

  const std::string version{"/movies/Movie (2010)/Movie (2010) Extended Edition.mkv"};
  ASSERT_GT(AddAsset(idMovie, version), 0);

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

  const int idFile{
      AddAsset(idMovie, "/more movies/Movie (2010)/Movie (2010) Extended Edition.mkv")};
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
  const int idExtended{
      AddAsset(idMovie, extended, VideoAssetType::VERSION, "Extended", VideoAssetTypeOwner::USER,
               {{"poster", "version-poster"}, {"thumb", chapterFrame.ToString()}})};
  ASSERT_GT(idExtended, 0);
  const int idOriginal{m_db.GetFileIdByMovie(idMovie)};

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

// A frame of the old default's file is not kept for a new default that has no thumb of its own:
// one of the new default's file is taken instead, where one can be. None can be of a disc.
TEST_F(TestVideoDatabase, SetDefaultVideoVersionTakesAFrameOfTheNewDefaultWhereItCan)
{
  for (const auto& [folder, newDefault, framed] :
       {std::tuple{std::string{"/movies/Disc (2010)/"},
                   URIUtils::GetBlurayPlaylistPath("/movies/Disc (2010)/BDMV/index.bdmv", 800),
                   false},
        std::tuple{std::string{"/movies/File (2010)/"},
                   std::string{"/movies/File (2010)/Movie (2010) Extended Edition.mkv"}, true}})
  {
    const std::string original{folder + "Movie (2010).mkv"};
    const int idMovie{AddMovie(original)};
    ASSERT_GT(idMovie, 0) << newDefault;
    ASSERT_TRUE(m_db.SetArtForItem(
        idMovie, KODI::MEDIA::TYPE::MOVIE,
        {{"fanart", "movie-fanart"}, {"thumb", IMAGE_FILES::URLFromFile(original, "video")}}));
    const int idDefault{AddAsset(idMovie, newDefault)};
    ASSERT_GT(idDefault, 0) << newDefault;

    ASSERT_TRUE(m_db.SetDefaultVideoVersion(VideoDbContentType::MOVIES, idMovie, idDefault));

    KODI::ART::Artwork expected{{"fanart", "movie-fanart"}};
    if (framed)
      expected["thumb"] = IMAGE_FILES::URLFromFile(newDefault, "video");
    KODI::ART::Artwork art;
    ASSERT_TRUE(m_db.GetArtForItem(idMovie, KODI::MEDIA::TYPE::MOVIE, art));
    EXPECT_EQ(expected, art) << newDefault;
  }
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

TEST_F(TestVideoDatabase, APathBelowAFolderWhoseScraperIsNotInstalledHasNoScraper)
{
  const auto setScraper{[this](const std::string& path, const std::string& scraper)
                        {
                          return m_db.ExecuteQuery(m_db.PrepareSQL(
                              "UPDATE path SET strContent='movies', strScraper='%s', "
                              "scanRecursive=2147483647 WHERE idPath=%i",
                              scraper.c_str(), m_db.AddPath(path)));
                        }};
  ASSERT_TRUE(setScraper("/movies/", "metadata.local"));
  ASSERT_TRUE(setScraper("/movies/Foreign/", "metadata.not.installed"));

  EXPECT_FALSE(m_db.GetScraperForPath("/movies/Foreign/"));
  EXPECT_FALSE(m_db.GetScraperForPath("/movies/Foreign/Film/"));
  // Where no folder between has content, the source's scraper is found
  EXPECT_TRUE(m_db.GetScraperForPath("/movies/Other/Film/"));
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

TEST_F(TestVideoDatabase, RemoveContentForPathRemovesEachPathOfANestedMultiPath)
{
  const std::string inner{XFILE::CMultiPathDirectory::ConstructMultiPath(
      std::vector<std::string>{"/other/", "/more/"})};
  const std::string source{
      XFILE::CMultiPathDirectory::ConstructMultiPath(std::vector<std::string>{"/movies/", inner})};
  for (const std::string& path :
       {source, inner, std::string{"/movies/"}, std::string{"/other/"}, std::string{"/more/"}})
  {
    const int idPath{m_db.AddPath(path)};
    ASSERT_GT(idPath, 0);
    ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
        "UPDATE path SET strContent='movies', strScraper='metadata.local' WHERE idPath=%i",
        idPath)));
  }
  const std::vector<std::string> files{"/movies/Film/Film.mkv", "/other/Other.mkv",
                                       "/more/More/More.mkv"};
  for (const auto& file : files)
    ASSERT_GT(AddMovie(file), 0);

  m_db.RemoveContentForPath(source);

  for (const auto& file : files)
    EXPECT_EQ(-1, m_db.GetMovieId(file)) << file;
  EXPECT_EQ(0, m_db.GetSingleValueInt("SELECT COUNT(*) FROM path WHERE strContent<>''"));
}

namespace
{
// A silent library clean of real files, below the test's folder
class TestVideoDatabaseClean : public TestVideoDatabase
{
protected:
  void SetUp() override
  {
    TestVideoDatabase::SetUp();
    ASSERT_TRUE(XFILE::CDirectory::Create(m_root));
  }

  void TearDown() override
  {
    std::erase_if(CMediaSourceSettings::GetInstance().GetSources(KODI::MEDIA::MediaSection::VIDEO),
                  [this](const CMediaSource& source)
                  { return source.strPath.starts_with(m_root); });
    TestVideoDatabase::TearDown();
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

  //! A show in two folders of a tv show source, with an episode in each
  struct TwoFolderShow
  {
    std::string first;
    std::string second;
    int idShow{-1};
    //! The episode in the first folder
    std::string episode;
    int idEpisode{-1};
  };

  TwoFolderShow AddTwoFolderShow()
  {
    const std::string shows{Folder(m_root, "shows")};
    AddSource(shows, "tvshows");
    TwoFolderShow show;
    show.first = Folder(shows, "Show");
    show.second = Folder(shows, "Show (more)");
    show.idShow = AddTvShow("Show", {show.first, show.second});
    show.episode = File(show.first, "Show S01E01.mkv");
    show.idEpisode = AddEpisode(show.idShow, show.episode, 1);
    EXPECT_GT(AddEpisode(show.idShow, File(show.second, "Show S01E02.mkv"), 2), 0);
    return show;
  }

  std::string PathHash(const std::string& path)
  {
    std::string hash;
    m_db.GetPathHash(path, hash);
    return hash;
  }

  void Clean() { m_db.CleanDatabase(nullptr, {}, false); }
};
} // namespace

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
  EXPECT_EQ(0, Rows("files", "idFile", idExtra));
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

// The path the movie was scanned from is no longer in the library
TEST_F(TestVideoDatabaseClean, KeepsAMovieWithoutAParentPathWhileItsSourceIsUnavailable)
{
  const std::string offline{Folder(m_root, "offline", false)};
  AddSource(offline, "movies");
  const std::string film{
      URIUtils::AddFileToFolder(Folder(offline, "Film (2010)", false), "Film (2010).mkv")};
  const int idMovie{AddMovie(film)};
  ASSERT_GT(idMovie, 0);
  ASSERT_TRUE(m_db.ExecuteQuery(
      m_db.PrepareSQL("UPDATE movie SET c23='999999' WHERE idMovie=%i", idMovie)));

  Clean();

  EXPECT_GT(m_db.GetMovieId(film), 0);
}

TEST_F(TestVideoDatabaseClean, ClearsTheHashesOfEveryFolderOfAShowThatLosesEpisodes)
{
  const TwoFolderShow show{AddTwoFolderShow()};
  ASSERT_GT(show.idShow, 0);
  ASSERT_GT(show.idEpisode, 0);
  ASSERT_TRUE(m_db.SetPathHash(show.first, "first"));
  ASSERT_TRUE(m_db.SetPathHash(show.second, "second"));

  ASSERT_TRUE(XFILE::CFile::Delete(show.episode));
  Clean();

  EXPECT_EQ("", PathHash(show.first));
  EXPECT_EQ("", PathHash(show.second));
}

TEST_F(TestVideoDatabaseClean, RemovesTheLinkToAShowFolderThatHasGone)
{
  const std::string shows{Folder(m_root, "shows")};
  AddSource(shows, "tvshows");
  const std::string current{Folder(shows, "Show")};
  const std::string old{Folder(shows, "Show (old)", false)};

  const int idShow{AddTvShow("Show", {current, old})};
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
  const int idVersion{AddAsset(idMovie, version, VideoAssetType::VERSION, "Extended",
                               VideoAssetTypeOwner::USER, {{"poster", "version-poster"}})};
  ASSERT_GT(idVersion, 0);

  ASSERT_TRUE(XFILE::CFile::Delete(gone));
  Clean();

  EXPECT_EQ(idVersion, m_db.GetFileIdByMovie(idMovie));
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
  ASSERT_EQ(3, Rows("episode", "idShow", idShow));

  ASSERT_TRUE(XFILE::CFile::Delete(gone));
  Clean();

  EXPECT_EQ(1, Rows("episode", "idShow", idShow));
  EXPECT_EQ(idKept, m_db.GetSingleValueInt(
                        m_db.PrepareSQL("SELECT idEpisode FROM episode WHERE idShow=%i", idShow)));
}

// The clean finds an episode's source through each folder of its show
TEST_F(TestVideoDatabaseClean, AnnouncesAnEpisodeOfAShowWithTwoFoldersRemovedOnce)
{
  const TwoFolderShow show{AddTwoFolderShow()};
  ASSERT_GT(show.idShow, 0);
  ASSERT_GT(show.idEpisode, 0);

  const auto announcements{std::make_shared<CQueuedAnnouncements>()};
  const CScopedAnnouncementManager registered{announcements};

  ASSERT_TRUE(XFILE::CFile::Delete(show.episode));
  Clean();

  EXPECT_EQ(0, Rows("episode", "idEpisode", show.idEpisode));
  EXPECT_EQ(1, announcements->CountRemoved(KODI::MEDIA::TYPE::EPISODE, show.idEpisode));
}

// No file is left for the clean to start from
TEST_F(TestVideoDatabaseClean, RemovesMediaWhoseFileHasGoneFromTheLibrary)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const int idMovie{AddMovie(File(Folder(movies, "Film (2010)"), "Film (2010).mkv"))};
  ASSERT_GT(idMovie, 0);
  const std::string shows{Folder(m_root, "shows")};
  AddSource(shows, "tvshows");
  const std::string show{Folder(shows, "Show")};
  const int idShow{AddTvShow(show)};
  ASSERT_GT(idShow, 0);
  const int idEpisode{AddEpisode(idShow, File(show, "Show S01E01.mkv"), 1)};
  ASSERT_GT(idEpisode, 0);
  const std::string musicVideos{Folder(m_root, "musicvideos")};
  AddSource(musicVideos, "musicvideos");
  CVideoInfoTag tag{Tag(File(musicVideos, "Artist - Song.mkv"))};
  const int idMVideo{m_db.SetDetailsForMusicVideo(tag, KODI::ART::Artwork{})};
  ASSERT_GT(idMVideo, 0);

  ASSERT_TRUE(m_db.ExecuteQuery("DELETE FROM files"));
  Clean();

  EXPECT_EQ(0, Rows("movie", "idMovie", idMovie));
  EXPECT_EQ(0, Rows("episode", "idEpisode", idEpisode));
  EXPECT_EQ(0, Rows("musicvideo", "idMVideo", idMVideo));
}

// Nothing ties media without a file to the source being cleaned
TEST_F(TestVideoDatabaseClean, KeepsAMovieWhoseFileHasGoneInACleanOfOneSource)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string film{Folder(movies, "Film (2010)")};
  const int idMovie{AddMovie(File(film, "Film (2010).mkv"))};
  ASSERT_GT(idMovie, 0);
  ASSERT_GT(AddMovie(File(film, "Film (2010) Kept.mkv")), 0);

  ASSERT_TRUE(m_db.ExecuteQuery(
      m_db.PrepareSQL("DELETE FROM files WHERE idFile=%i", m_db.GetFileIdByMovie(idMovie))));
  m_db.CleanDatabase(nullptr, {m_db.GetPathId(film)}, false);

  EXPECT_EQ(1, Rows("movie", "idMovie", idMovie));
}

// The clean removes a file whose path has gone without checking it
TEST_F(TestVideoDatabaseClean, RemovesAMovieWhoseFileHasLostItsPath)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string film{Folder(movies, "Film (2010)")};
  const int idMovie{AddMovie(File(film, "Film (2010).mkv"))};
  ASSERT_GT(idMovie, 0);
  const int idFile{m_db.GetFileIdByMovie(idMovie)};

  ASSERT_TRUE(
      m_db.ExecuteQuery(m_db.PrepareSQL("DELETE FROM path WHERE idPath=%i", m_db.GetPathId(film))));
  Clean();

  EXPECT_EQ(0, Rows("movie", "idMovie", idMovie));
  EXPECT_EQ(0, Rows("files", "idFile", idFile));
}

TEST_F(TestVideoDatabaseClean, AnnouncesAMovieWhoseDefaultFileHasGoneFromTheLibraryUpdated)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string film{Folder(movies, "Film (2010)")};
  const int idMovie{AddMovie(File(film, "Film (2010).mkv"))};
  ASSERT_GT(idMovie, 0);
  const int idVersion{AddAsset(idMovie, File(film, "Film (2010) Extended.mkv"))};
  ASSERT_GT(idVersion, 0);

  ASSERT_TRUE(m_db.ExecuteQuery(
      m_db.PrepareSQL("DELETE FROM files WHERE idFile=%i", m_db.GetFileIdByMovie(idMovie))));
  const auto announcements{std::make_shared<CQueuedAnnouncements>()};
  const CScopedAnnouncementManager registered{announcements};
  Clean();

  EXPECT_EQ(idVersion, m_db.GetFileIdByMovie(idMovie));
  EXPECT_EQ(1, announcements->CountUpdated(KODI::MEDIA::TYPE::MOVIE, idMovie));
  EXPECT_EQ(0, announcements->CountRemoved(KODI::MEDIA::TYPE::MOVIE, idMovie));
}

// A version is only reached through its own table once its file has gone
TEST_F(TestVideoDatabaseClean, RemovesAVersionWhoseFileHasGoneFromTheLibrary)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");
  const std::string film{Folder(movies, "Film (2010)")};
  const int idMovie{AddMovie(File(film, "Film (2010).mkv"))};
  ASSERT_GT(idMovie, 0);
  const int idVersion{AddAsset(idMovie, File(film, "Film (2010) Extended.mkv"))};
  ASSERT_GT(idVersion, 0);

  ASSERT_TRUE(m_db.ExecuteQuery(
      m_db.PrepareSQL("UPDATE videoversion SET idFile=999999 WHERE idFile=%i", idVersion)));
  const auto announcements{std::make_shared<CQueuedAnnouncements>()};
  const CScopedAnnouncementManager registered{announcements};
  Clean();

  EXPECT_EQ(0, Rows("videoversion", "idFile", 999999));
  EXPECT_GT(m_db.GetMovieId(File(film, "Film (2010).mkv")), 0);
  EXPECT_EQ(1, announcements->CountUpdated(KODI::MEDIA::TYPE::MOVIE, idMovie));
}

// Another client of a shared library can have a source this one hasn't
TEST_F(TestVideoDatabaseClean, KeepsAnEpisodeOutsideTheSourcesWhileItExists)
{
  const std::string shows{Folder(m_root, "shows")};
  AddSource(shows, "tvshows");
  const std::string outside{Folder(m_root, "outside")};
  const std::string inSource{Folder(shows, "Show")};
  const std::string notInSource{Folder(outside, "Show")};

  const int idShow{AddTvShow("Show", {inSource, notInSource})};
  ASSERT_GT(idShow, 0);
  const int idEpisode{AddEpisode(idShow, File(notInSource, "Show S01E01.mkv"), 1)};
  ASSERT_GT(idEpisode, 0);

  Clean();

  EXPECT_EQ(1, Rows("episode", "idEpisode", idEpisode));
}

// The source of the show's other folder is available, but doesn't hold the episode
TEST_F(TestVideoDatabaseClean, KeepsAnEpisodeInAnUnavailableFolderOfAShow)
{
  const std::string shows{Folder(m_root, "shows")};
  AddSource(shows, "tvshows");
  const std::string offline{Folder(m_root, "offline", false)};
  AddSource(offline, "tvshows");
  const std::string available{Folder(shows, "Show")};
  const std::string unavailable{Folder(offline, "Show", false)};

  const int idShow{AddTvShow("Show", {available, unavailable})};
  ASSERT_GT(idShow, 0);
  const int idEpisode{
      AddEpisode(idShow, URIUtils::AddFileToFolder(unavailable, "Show S01E01.mkv"), 1)};
  ASSERT_GT(idEpisode, 0);

  Clean();

  EXPECT_EQ(1, Rows("episode", "idEpisode", idEpisode));
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

  EXPECT_EQ(0, Rows("files", "idFile", idExtra));
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
  EXPECT_EQ(1, Rows("files", "idFile", idVersion));
  EXPECT_EQ(1, Rows("videoversion", "idFile", idVersion));
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
  EXPECT_EQ(1, Rows("files", "idFile", idVersion));
  EXPECT_EQ(0, Rows("files", "idFile", idExtra));
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
  EXPECT_EQ(0, Rows("files", "idFile", idVersion));
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

// No path of the source has content set, so the movie's parent path stands in for the source
TEST_F(TestVideoDatabaseClean, RemovesAMovieThatHasGoneFromASourceWithoutContent)
{
  const std::string videos{Folder(m_root, "videos")};
  CMediaSource source;
  source.FromNameAndPaths(videos, {videos});
  CMediaSourceSettings::GetInstance()
      .GetSources(KODI::MEDIA::MediaSection::VIDEO)
      .push_back(source);
  const std::string gone{File(Folder(videos, "Film (2010)"), "Film (2010).mkv")};
  ASSERT_GT(AddMovie(gone), 0);

  ASSERT_TRUE(XFILE::CFile::Delete(gone));
  Clean();

  EXPECT_LE(m_db.GetMovieId(gone), 0);
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

  EXPECT_EQ(0, Rows("files", "idFile", idFile));
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
    EXPECT_EQ(1, Rows("files", "idFile", idFile)) << idFile;
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
  const std::shared_ptr<CQueuedAnnouncements> m_announcements{
      std::make_shared<CQueuedAnnouncements>()};
  const CScopedAnnouncementManager m_queued{m_announcements};
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
  m_db.SetAnnounceFileChanges(false);
  ASSERT_TRUE(m_db.AddBookMarkToFile("/videos/film.mkv", resumePoint, CBookmark::RESUME));
  m_db.SetAnnounceFileChanges(true);

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
  item->GetVideoInfoTag()->SetResumePoint(600.0, 3600.0, "");

  ASSERT_TRUE(ResetResumePointJob(item).Work(m_db));

  const auto files{m_announcements->Files()};
  ASSERT_EQ(1u, files.size());
  EXPECT_EQ("/movies/film.mkv", files[0].data["file"].asString());
}

TEST_F(TestPlayStateAnnouncements, AnnouncesAFileWithoutCredentials)
{
  // Dated by its modification time, the file would be looked up on the share
  CAdvancedSettings& advanced{*CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()};
  const int dateAdded{std::exchange(advanced.m_iVideoLibraryDateAdded, 0)};
  const bool written{
      m_db.SetPlayCount(CFileItem("smb://user:secret@server/videos/film.mkv", false), 1).IsValid()};
  advanced.m_iVideoLibraryDateAdded = dateAdded;
  ASSERT_TRUE(written);

  const auto files{m_announcements->Files()};
  ASSERT_EQ(1u, files.size());
  EXPECT_EQ("smb://server/videos/film.mkv", files[0].data["file"].asString());
}

TEST_F(TestPlayStateAnnouncements, ClearingAResumePointAnnouncesItOnlyWhenOneWasHeld)
{
  CBookmark resumePoint;
  resumePoint.timeInSeconds = 600.0;
  resumePoint.totalTimeInSeconds = 3600.0;
  m_db.SetAnnounceFileChanges(false);
  ASSERT_TRUE(m_db.AddBookMarkToFile("/videos/film.mkv", resumePoint, CBookmark::RESUME));
  m_db.SetAnnounceFileChanges(true);

  ASSERT_TRUE(m_db.ClearBookMarksOfFile("/videos/film.mkv", CBookmark::RESUME));
  ASSERT_TRUE(m_db.ClearBookMarksOfFile("/videos/film.mkv", CBookmark::RESUME));

  const auto files{m_announcements->Files()};
  ASSERT_EQ(1u, files.size());
  EXPECT_EQ("OnResumePointChanged", files[0].message);
  EXPECT_EQ(0.0, files[0].data["resume"]["position"].asDouble());
}

TEST_F(TestPlayStateAnnouncements, UpdatingTheLastPlayedDateAnnouncesNoPlayCount)
{
  const CFileItem film{"/videos/film.mkv", false};
  m_db.SetAnnounceFileChanges(false);
  ASSERT_TRUE(m_db.SetPlayCount(film, 2).IsValid());
  m_db.SetAnnounceFileChanges(true);

  ASSERT_TRUE(m_db.UpdateLastPlayed(film).IsValid());

  EXPECT_TRUE(m_announcements->Files().empty());
}

// Writes made later on the connection would otherwise never be committed
TEST_F(TestVideoDatabaseClean, EndsItsTransactionWhenThePathsHoldNoMedia)
{
  const std::string movies{Folder(m_root, "movies")};
  AddSource(movies, "movies");

  m_db.CleanDatabase(nullptr, {m_db.GetPathId(movies)}, false);

  EXPECT_FALSE(m_db.InTransaction());
}

TEST_F(TestVideoDatabase, DeleteItemRemovesTheKindItIsGiven)
{
  const int idMovie{AddMovie("/movies/film.mkv")};
  const int idSet{m_db.AddSet("Saga")};
  ASSERT_GT(idMovie, 0);
  ASSERT_GT(idSet, 0);
  m_db.SetMovieSet(idMovie, idSet);

  EXPECT_TRUE(m_db.DeleteItem(VideoDbContentType::MOVIE_SETS, idSet));
  CVideoInfoTag details;
  EXPECT_FALSE(m_db.GetSetInfo(idSet, details));
  EXPECT_TRUE(m_db.GetMovieInfo("", details, idMovie));

  EXPECT_TRUE(m_db.DeleteItem(VideoDbContentType::MOVIES, idMovie));
  EXPECT_FALSE(m_db.GetMovieInfo("", details, idMovie));
  EXPECT_FALSE(m_db.DeleteItem(VideoDbContentType::MUSICALBUMS, 1));
}

TEST_F(TestVideoDatabase, AShowIsWatchedWhileAnyOfItsEpisodesIs)
{
  const int idShow{AddTvShow("/tv/Show/")};
  const int idEpisode{AddEpisode(idShow, "/tv/Show/Show S01E01.mkv", 1)};
  ASSERT_GT(AddEpisode(idShow, "/tv/Show/Show S01E02.mkv", 2), 0);
  EXPECT_FALSE(m_db.IsWatched(KODI::MEDIA::TYPE::TV_SHOW, idShow));

  ASSERT_TRUE(m_db.SetPlayCount(CFileItem("/tv/Show/Show S01E01.mkv", false), 1).IsValid());

  EXPECT_TRUE(m_db.IsWatched(KODI::MEDIA::TYPE::TV_SHOW, idShow));
  EXPECT_TRUE(m_db.IsWatched(KODI::MEDIA::TYPE::EPISODE, idEpisode));
}

TEST_F(TestVideoDatabase, OnlyAnEditLeavingAFileUnwatchedMayClearItsLastPlayedDate)
{
  ASSERT_TRUE(m_db.SetPlayCount(CFileItem("/videos/film.mkv", false), 2).IsValid());

  EXPECT_FALSE(m_db.CanClearLastPlayed("/videos/film.mkv", std::nullopt));
  EXPECT_FALSE(m_db.CanClearLastPlayed("/videos/film.mkv", 1));
  EXPECT_TRUE(m_db.CanClearLastPlayed("/videos/film.mkv", 0));
  EXPECT_TRUE(m_db.CanClearLastPlayed("/videos/unplayed.mkv", std::nullopt));
}

TEST_F(TestVideoDatabase, AResumePointWithoutALengthTakesTheOneStoredOrTheVideos)
{
  const std::string film{"/videos/film.mkv"};
  const int idFile{m_db.AddFile(film)};
  ASSERT_GT(idFile, 0);
  CStreamDetails details;
  auto* video = new CStreamDetailVideo();
  video->m_iDuration = 5400;
  video->SetSource(CStreamDetail::MEDIA);
  details.AddStream(video);
  ASSERT_TRUE(m_db.SetStreamDetailsForFileId(details, idFile));

  CBookmark resumePoint;
  ASSERT_TRUE(m_db.SetResumePoint(film, 600.0, 0.0));
  ASSERT_TRUE(m_db.GetResumeBookMark(film, resumePoint));
  EXPECT_EQ(5400.0, resumePoint.totalTimeInSeconds);

  ASSERT_TRUE(m_db.SetResumePoint(film, 900.0, 3600.0));
  ASSERT_TRUE(m_db.SetResumePoint(film, 1200.0, 0.0));
  ASSERT_TRUE(m_db.GetResumeBookMark(film, resumePoint));
  EXPECT_EQ(1200.0, resumePoint.timeInSeconds);
  EXPECT_EQ(3600.0, resumePoint.totalTimeInSeconds);

  ASSERT_TRUE(m_db.SetResumePoint(film, 0.0, 0.0));
  EXPECT_FALSE(m_db.GetResumeBookMark(film, resumePoint));
}

TEST_F(TestVideoDatabase, ABlurayInADriveResumesFromThePlaylistLastPlayed)
{
  CURL disc{"bluray://"};
  disc.SetHostName("removable://FILM_1234");
  disc.SetFileName("BDMV/index.bdmv");
  CURL playlist{disc};
  playlist.SetFileName("BDMV/PLAYLIST/00800.mpls");
  CBookmark resumePoint;
  resumePoint.timeInSeconds = 600.0;
  resumePoint.totalTimeInSeconds = 3600.0;
  ASSERT_TRUE(m_db.AddBookMarkToFile(playlist.Get(), resumePoint, CBookmark::RESUME));

  CBookmark found;
  ASSERT_TRUE(m_db.GetDiscResumeBookMark(disc.Get(), found));
  EXPECT_EQ(600.0, found.timeInSeconds);
  EXPECT_FALSE(m_db.GetDiscResumeBookMark("removable://OTHER_5678", found));
}

TEST_F(TestVideoDatabase, RemovingAVersionOnADiscPlaylistRemovesItsFileToo)
{
  const MovieWithVersion movie{AddMovieWithVersion(KODI::ART::Artwork{}, KODI::ART::Artwork{})};
  ASSERT_GT(movie.versionFileId, 0);
  EXPECT_FALSE(m_db.RemoveVideoAsset(movie.movieFileId));
  EXPECT_TRUE(m_db.RemoveVideoAsset(movie.versionFileId));
  CVideoInfoTag file;
  EXPECT_TRUE(m_db.GetFileInfo("", file, movie.versionFileId));

  const int idSource{
      AddMovie(URIUtils::GetBlurayPlaylistPath("/movies/Movie (2010)/BDMV/index.bdmv", 801))};
  ASSERT_GT(idSource, 0);
  const int idFile{m_db.GetFileIdByMovie(idSource)};
  ASSERT_TRUE(m_db.ConvertVideoToVersion(VideoDbContentType::MOVIES, idSource, movie.movieId, -1,
                                         VideoAssetType::VERSION));
  EXPECT_TRUE(m_db.RemoveVideoAsset(idFile));
  EXPECT_FALSE(m_db.GetFileInfo("", file, idFile));
}

TEST_F(TestVideoDatabase, AVersionConvertedToAVideoIsAMovieOfItsOwn)
{
  const MovieWithVersion movie{
      AddMovieWithVersion({{"poster", "/art/movie.jpg"}}, {{"poster", "/art/version.jpg"}})};
  ASSERT_GT(movie.versionFileId, 0);
  CVideoInfoTag details;
  ASSERT_TRUE(m_db.GetMovieInfo("", details, movie.movieId));
  // As Manage versions lists it
  CVideoInfoTag version{Tag("/movies/Movie (2010)/Movie (2010) Standard Edition.mkv")};
  version.m_iDbId = movie.versionFileId;
  version.m_iFileId = movie.versionFileId;

  const int idMovie{m_db.ConvertVersionToVideo(details, version)};
  ASSERT_GT(idMovie, 0);
  EXPECT_NE(movie.movieId, idMovie);
  CVideoInfoTag converted;
  ASSERT_TRUE(m_db.GetMovieInfo("", converted, idMovie));
  EXPECT_EQ(movie.versionFileId, converted.m_iFileId);
  CFileItemList versions;
  m_db.GetVideoVersions(VideoDbContentType::MOVIES, movie.movieId, versions,
                        VideoAssetType::VERSION);
  EXPECT_EQ(1, versions.Size());
}

TEST_F(TestVideoDatabase, AnExtrasFileLoadsAsTheExtrasListShowsIt)
{
  CVideoInfoTag movie{Tag("/movies/Film (2010)/Film (2010).mkv")};
  movie.m_strPictureURL.ParseFromData("<thumb>/art/poster.jpg</thumb>");
  const int idMovie{m_db.SetDetailsForMovie(movie, KODI::ART::Artwork{})};
  ASSERT_GT(idMovie, 0);
  const std::string trailer{"/movies/Film (2010)/Trailer.mkv"};
  ASSERT_GT(AddAsset(idMovie, trailer, VideoAssetType::EXTRA), 0);
  CFileItemList extras;
  m_db.GetVideoVersions(VideoDbContentType::MOVIES, idMovie, extras, VideoAssetType::EXTRA);
  ASSERT_EQ(1, extras.Size());
  const CVideoInfoTag& listed{*extras[0]->GetVideoInfoTag()};

  CVideoInfoTag loaded;
  ASSERT_TRUE(m_db.LoadVideoInfo(trailer, loaded));

  EXPECT_EQ(KODI::MEDIA::TYPE::VIDEO_VERSION, loaded.GetMediaType());
  EXPECT_EQ(listed.m_iDbId, loaded.m_iDbId);
  EXPECT_EQ(listed.m_iFileId, loaded.m_iFileId);
  EXPECT_EQ(VideoAssetType::EXTRA, loaded.GetAssetInfo().GetType());
  EXPECT_EQ(listed.GetAssetInfo().GetId(), loaded.GetAssetInfo().GetId());
  EXPECT_EQ("Trailer", loaded.m_strTitle);
  EXPECT_EQ(listed.m_strPictureURL.GetData(), loaded.m_strPictureURL.GetData());
  EXPECT_NE("", loaded.m_strPictureURL.GetData());
}

TEST_F(TestVideoDatabase, AddingAnotherMoviesFileAsAVersionBringsThatMovie)
{
  const int idTarget{AddMovie("/movies/Target.mkv")};
  const int idOther{AddMovie("/movies/Other.mkv")};
  ASSERT_GT(idTarget, 0);
  ASSERT_GT(idOther, 0);
  ASSERT_TRUE(m_db.CanAddVideoAsset(m_db.GetFileIdByMovie(idOther)));

  ASSERT_GT(AddAsset(idTarget, "/movies/Other.mkv", VideoAssetType::VERSION, "Other cut"), 0);
  CVideoInfoTag gone;
  EXPECT_FALSE(m_db.GetMovieInfo("", gone, idOther));
  EXPECT_EQ(idTarget, m_db.GetVideoVersionInfo("/movies/Other.mkv").m_idMedia);

  // A movie with versions would be left without a file of its own
  const MovieWithVersion withVersions{
      AddMovieWithVersion(KODI::ART::Artwork{}, KODI::ART::Artwork{})};
  EXPECT_FALSE(m_db.CanAddVideoAsset(withVersions.movieFileId));
}

// Choosing a disc playlist adds the asset inside a transaction of the dialog's own
TEST_F(TestVideoDatabase, AddingAnotherMoviesFileAsAVersionKeepsTheCallersTransaction)
{
  const int idTarget{AddMovie("/movies/Target.mkv")};
  ASSERT_GT(idTarget, 0);
  ASSERT_GT(AddMovie("/movies/Other.mkv"), 0);

  m_db.BeginTransaction();
  CFileItem other{"/movies/Other.mkv", false};
  const bool added{m_db.AddVideoAsset(VideoDbContentType::MOVIES, idTarget,
                                      VIDEO_VERSION_ID_DEFAULT, VideoAssetType::VERSION, other)};
  const bool stillOpen{m_db.InTransaction()};
  m_db.CommitTransaction();

  ASSERT_TRUE(added);
  EXPECT_TRUE(stillOpen);
  EXPECT_EQ(idTarget, m_db.GetVideoVersionInfo("/movies/Other.mkv").m_idMedia);
}

TEST_F(TestVideoDatabase, RevertingAPlaylistMovesItsMovieToTheDisc)
{
  const std::string disc{"/movies/Film/BDMV/index.bdmv"};
  const int idMovie{AddMovie(URIUtils::GetBlurayPlaylistPath(disc, 800))};
  ASSERT_GT(idMovie, 0);
  const int idPlaylistFile{m_db.GetFileIdByMovie(idMovie)};
  const std::vector<KODI::VIDEO::DiscPlaylist> playlists{{.playlist = 800,
                                                          .idFile = idPlaylistFile,
                                                          .mediaType = VideoDbContentType::MOVIES,
                                                          .idMedia = idMovie,
                                                          .itemType = VideoAssetType::VERSION}};

  std::vector<CVideoDatabase::PlaylistRevert> reverted;
  ASSERT_EQ(CVideoDatabase::PlaylistRevertResult::DONE,
            m_db.RevertPlaylistsToDisc(disc, playlists, reverted));
  ASSERT_EQ(1u, reverted.size());
  EXPECT_EQ(idMovie, reverted[0].details.m_iDbId);
  EXPECT_EQ(idPlaylistFile, reverted[0].oldIdFile);
  EXPECT_EQ(disc, reverted[0].details.m_strFileNameAndPath);
}

TEST_F(TestVideoDatabase, APlayedItemMovesToTheFileItWasPlayedFromOnlyWhenThatChanged)
{
  const int idMovie{AddMovie("/movies/Film.mkv")};
  ASSERT_GT(idMovie, 0);
  CVideoInfoTag tag;
  ASSERT_TRUE(m_db.GetMovieInfo("", tag, idMovie));
  CFileItem item{tag};
  EXPECT_FALSE(m_db.MoveFileOfPlayedItem(item, "/movies/Film.mkv"));

  item.SetProperty(KODI::ITEM::PROPERTY::NEW_STACK_PATH, true);
  const auto move{
      m_db.MoveFileOfPlayedItem(item, "stack:///movies/Film-cd1.mkv , /movies/Film-cd2.mkv")};
  ASSERT_TRUE(move);
  EXPECT_EQ(tag.m_iFileId, move->replacedIdFile);
  EXPECT_EQ(idMovie, move->idMedia);
  CVideoInfoTag moved;
  ASSERT_TRUE(m_db.GetMovieInfo("", moved, idMovie));
  EXPECT_EQ(move->idFile, moved.m_iFileId);
}

TEST_F(TestVideoDatabase, ATagIsFoundOnlyForTheTypeItIsGivenTo)
{
  const int idMovie{AddMovie("/movies/film.mkv")};
  ASSERT_GT(idMovie, 0);
  m_db.AddTagToItem(idMovie, m_db.AddTag("Favourites"), KODI::MEDIA::TYPE::MOVIE);

  EXPECT_TRUE(m_db.HasTag("Favourites", KODI::MEDIA::TYPE::MOVIE));
  EXPECT_FALSE(m_db.HasTag("Favourites", KODI::MEDIA::TYPE::TV_SHOW));
  EXPECT_FALSE(m_db.HasTag("Classics", KODI::MEDIA::TYPE::MOVIE));
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

// A refresh removes a disc's versions to add them again, which leaves its folder as it was
TEST_F(TestVideoDatabase, DeleteVideoAssetCanKeepTheHashOfItsFolder)
{
  const std::string folder{"/movies/Film (2010)/"};
  const int idMovie{AddMovie(folder + "Film.mkv")};
  ASSERT_GT(idMovie, 0);
  CFileItem version{folder + "Film Extended.mkv", false};
  ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, VIDEO_VERSION_ID_DEFAULT,
                                 VideoAssetType::VERSION, version));
  const int idFile{
      m_db.GetSingleValueInt("SELECT idFile FROM files WHERE strFilename = 'Film Extended.mkv'")};
  ASSERT_GT(idFile, 0);
  ASSERT_TRUE(m_db.SetPathHash(folder, "hash"));

  ASSERT_TRUE(m_db.UnlinkVideoAsset(idFile, DeleteMovieHashAction::HASH_PRESERVE));

  std::string hash;
  ASSERT_TRUE(m_db.GetPathHash(folder, hash));
  EXPECT_EQ("hash", hash);
}

// An actor, genre, studio, country or tag is the one of that name, whatever its case, and _ and %
// in a name are no wildcards
TEST_F(TestVideoDatabase, ANamedDetailIsFoundByItsNameExactly)
{
  struct Detail
  {
    void (*set)(CVideoInfoTag& movie, const std::string& name);
    std::string table;
    std::string linkTable;
  };
  const Detail details[]{
      {[](CVideoInfoTag& movie, const std::string& name)
       {
         SActorInfo actor;
         actor.strName = name;
         movie.m_cast.emplace_back(actor);
       },
       "actor", "actor_link"},
      {[](CVideoInfoTag& movie, const std::string& name) { movie.m_genre = {name}; }, "genre",
       "genre_link"},
      {[](CVideoInfoTag& movie, const std::string& name) { movie.m_studio = {name}; }, "studio",
       "studio_link"},
      {[](CVideoInfoTag& movie, const std::string& name) { movie.m_country = {name}; }, "country",
       "country_link"},
      {[](CVideoInfoTag& movie, const std::string& name) { movie.m_tags = {name}; }, "tag",
       "tag_link"},
  };
  int movies{0};
  for (const auto& [set, table, linkTable] : details)
  {
    for (const std::string name : {"Ann_Lee", "AnnxLee", "ann_lee", "Ann%"})
    {
      CVideoInfoTag movie{Tag("/movies/" + std::to_string(++movies) + ".mkv")};
      set(movie, name);
      ASSERT_GT(m_db.SetDetailsForMovie(movie, KODI::ART::Artwork{}), 0) << table << ' ' << name;
    }

    EXPECT_EQ(3, m_db.GetSingleValueInt("SELECT COUNT(*) FROM " + table)) << table;
    EXPECT_EQ(2, m_db.GetSingleValueInt(StringUtils::Format(
                     "SELECT COUNT(*) FROM {1} JOIN {0} ON {0}.{0}_id = {1}.{0}_id "
                     "WHERE {0}.name = 'Ann_Lee'",
                     table, linkTable)))
        << table;
  }
}

namespace
{
//! Sets how the date a video is added with is chosen, as advancedsettings' <dateadded> does, until it
//! goes, however a test ends
class CDateAddedPolicy
{
public:
  explicit CDateAddedPolicy(int policy) : m_previous{Setting()} { Setting() = policy; }
  ~CDateAddedPolicy() { Setting() = m_previous; }
  CDateAddedPolicy(const CDateAddedPolicy&) = delete;
  CDateAddedPolicy& operator=(const CDateAddedPolicy&) = delete;

private:
  static int& Setting()
  {
    return CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iVideoLibraryDateAdded;
  }

  const int m_previous;
};
} // namespace

// The date a file is added with is read from the file before it is added, inside a transaction
// that would otherwise wait on the file system
TEST_F(TestVideoDatabase, AFilesDateAddedIsReadBeforeItIsAdded)
{
  const CDateAddedPolicy dateAdded{1}; // the file's modified time
  const std::string path{File(Folder(m_root, "Films"), "Film.mkv")};
  MakeTwoDaysOld(path);
  const CDateTime modified{CFileUtils::GetModificationDate(path, false)};
  ASSERT_TRUE(modified.IsValid());
  ASSERT_LT(modified, CDateTime::GetCurrentDateTime() - CDateTimeSpan(1, 0, 0, 0));

  m_db.PrepareDateAdded(path);
  // Gone once the date is known, so that it can't be read again
  ASSERT_TRUE(XFILE::CFile::Delete(path));
  const int idFile{m_db.AddFile(path)};
  m_db.ForgetPreparedDatesAdded();

  ASSERT_GT(idFile, 0);
  EXPECT_EQ(modified.GetAsDBDateTime(),
            m_db.GetSingleValue(
                m_db.PrepareSQL("SELECT dateAdded FROM files WHERE idFile = %i", idFile)));
}

// A new show's folder is dated before the show is added too
TEST_F(TestVideoDatabase, AShowsFolderDateAddedIsReadBeforeItIsAdded)
{
  const CDateAddedPolicy dateAdded{1}; // the folder's modified time
  const std::string folder{Folder(m_root, "Show")};
  MakeTwoDaysOld(folder);
  const CDateTime modified{CFileUtils::GetModificationDate(folder, false)};
  ASSERT_TRUE(modified.IsValid());
  ASSERT_LT(modified, CDateTime::GetCurrentDateTime() - CDateTimeSpan(1, 0, 0, 0));

  m_db.PrepareDateAdded(folder);
  // Gone once the date is known, so that it can't be read again
  ASSERT_TRUE(XFILE::CDirectory::RemoveRecursive(m_root));
  const int idShow{AddTvShow("Show", {folder})};
  m_db.ForgetPreparedDatesAdded();

  ASSERT_GT(idShow, 0);
  EXPECT_EQ(modified.GetAsDBDateTime(),
            m_db.GetSingleValue(m_db.PrepareSQL("SELECT dateAdded FROM path WHERE idPath = %i",
                                                m_db.GetPathId(folder))));
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
  const CScopedLanguageStrings strings;
  ASSERT_TRUE(strings.IsLoaded());
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
}

// A file can be only one asset, so each extra on a disc is a playlist of its own
TEST_F(TestVideoDatabase, ExtrasOnADiscAreItsPlaylists)
{
  const std::string disc{"/movies/Nope (2022)/NOPE.iso"};
  const int idMovie{AddMovie(URIUtils::GetBlurayPlaylistPath(disc, 800))};
  ASSERT_GT(idMovie, 0);

  for (const auto& [playlist, name] : {std::pair{1244, "Gag Reel"}, std::pair{1245, "Making Of"}})
    ASSERT_GT(AddAsset(idMovie, URIUtils::GetBlurayPlaylistPath(disc, playlist),
                       VideoAssetType::EXTRA, name, VideoAssetTypeOwner::AUTO),
              0);

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
  const CScopedLanguageStrings strings;
  ASSERT_TRUE(strings.IsLoaded());
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

  ASSERT_TRUE(m_db.UnlinkVideoAsset(first));
  EXPECT_EQ("Gag Reel", m_db.GetVideoVersionById(made));
  ASSERT_TRUE(m_db.UnlinkVideoAsset(second));
  EXPECT_EQ("", m_db.GetVideoVersionById(made));

  m_db.SetVideoVersion(third, VIDEO_EXTRA_ID_BEGIN + 1);
  EXPECT_EQ("", m_db.GetVideoVersionById(typed));
  ASSERT_TRUE(m_db.UnlinkVideoAsset(third));
  EXPECT_EQ(kind, m_db.GetVideoVersionById(VIDEO_EXTRA_ID_BEGIN + 1));
  EXPECT_EQ("My Cut", m_db.GetVideoVersionById(version));
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
  ASSERT_GT(AddAsset(idMovie, URIUtils::GetBlurayPlaylistPath(disc, 12), VideoAssetType::EXTRA,
                     "Gag Reel", VideoAssetTypeOwner::AUTO),
            0);

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
  ASSERT_GT(AddAsset(idMovie, disc, VideoAssetType::EXTRA, "Disc 1", VideoAssetTypeOwner::AUTO), 0);
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

// The search's path for an extra ends in its file id, so the extra is played from its file
TEST_F(TestVideoDatabase, AnExtraFoundBySearchPlaysItsFile)
{
  const std::string disc{"/movies/Movie (2000)/BDMV/index.bdmv"};
  const int idMovie{AddMovie(URIUtils::GetBlurayPlaylistPath(disc, 800))};
  ASSERT_GT(idMovie, 0);
  const std::string playlist{URIUtils::GetBlurayPlaylistPath(disc, 802)};
  ASSERT_GT(
      AddAsset(idMovie, playlist, VideoAssetType::EXTRA, "Gag Reel", VideoAssetTypeOwner::AUTO), 0);

  CFileItemList items;
  m_db.GetMovieExtrasByName("Gag", items);
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(items[0]->GetDynPath(), playlist);
}

namespace
{
//! A movie source, /movies/, scanned recursively, in a database of the test's own
class TestLookupByFolders : public VideoDatabaseTestBase
{
protected:
  TestLookupByFolders() : VideoDatabaseTestBase("TestLookupByFolders", "/movies/") {}

  void SetUp() override
  {
    VideoDatabaseTestBase::SetUp();
    UseFolderNames(true);
  }

  void UseFolderNames(bool useFolderNames)
  {
    ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
        "UPDATE path SET strContent='movies', strScraper='metadata.local', useFolderNames=%i, "
        "scanRecursive=2147483647 WHERE idPath=%i",
        useFolderNames ? 1 : 0, m_db.AddPath("/movies/"))));
  }
};
} // namespace

// A movie in the source's root shares it, whether a file, an ISO, or a playlist of an ISO
TEST_F(TestLookupByFolders, AMovieInTheSourcesRootHasNoFolderOfItsOwn)
{
  for (const std::string& video :
       {std::string{"/movies/Film.mkv"}, std::string{"/movies/M3GAN.iso"},
        URIUtils::GetBlurayPlaylistPath("/movies/M3GAN.iso", 800)})
    EXPECT_FALSE(m_db.LookupByFolders(video)) << video;
}

TEST_F(TestLookupByFolders, AMovieInAFolderBelowTheSourcesRootHasOne)
{
  for (const std::string& video :
       {std::string{"/movies/Film/Film.mkv"}, std::string{"/movies/M3GAN/M3GAN.iso"},
        URIUtils::GetBlurayPlaylistPath("/movies/M3GAN/M3GAN.iso", 800),
        URIUtils::GetBlurayPlaylistPath("/movies/M3GAN/BDMV/index.bdmv", 800),
        URIUtils::GetBlurayPlaylistPath("/movies/M3GAN/disc 1/BDMV/index.bdmv", 800),
        URIUtils::GetBlurayPlaylistPath("/movies/M3GAN/disc 1/M3GAN.iso", 800),
        std::string{"/movies/M3GAN/VIDEO_TS/VIDEO_TS.IFO"}})
    EXPECT_TRUE(m_db.LookupByFolders(video)) << video;
}

TEST_F(TestLookupByFolders, AMovieOfASourceWithoutFolderNamesHasNoFolderOfItsOwn)
{
  UseFolderNames(false);
  for (const std::string& video : {std::string{"/movies/Film/Film.mkv"},
                                   URIUtils::GetBlurayPlaylistPath("/movies/M3GAN/M3GAN.iso", 800)})
    EXPECT_FALSE(m_db.LookupByFolders(video)) << video;
}

TEST_F(TestVideoDatabase, ClearingAllBookmarksOfAFileClearsEachType)
{
  const std::string film{"/videos/film.mkv"};
  CBookmark bookmark;
  bookmark.timeInSeconds = 60.0;
  bookmark.totalTimeInSeconds = 3600.0;
  ASSERT_TRUE(m_db.AddBookMarkToFile(film, bookmark, CBookmark::STANDARD));
  ASSERT_TRUE(m_db.AddBookMarkToFile(film, bookmark, CBookmark::RESUME));

  EXPECT_TRUE(m_db.ClearAllBookMarksOfFile(film));

  VECBOOKMARKS bookmarks;
  m_db.GetBookMarksForFile(film, bookmarks, CBookmark::STANDARD);
  EXPECT_TRUE(bookmarks.empty());
  CBookmark resumePoint;
  EXPECT_FALSE(m_db.GetResumeBookMark(film, resumePoint));
}

TEST_F(TestVideoDatabase, AnItemIsInTheLibraryByItsFileOrItsShowsFolder)
{
  ASSERT_GT(AddMovie("/movies/Film.mkv"), 0);
  const int idShow{AddTvShow("/shows/Show/")};
  ASSERT_GT(idShow, 0);
  ASSERT_GT(AddEpisode(idShow, "/shows/Show/Show S01E01.mkv", 1), 0);

  EXPECT_TRUE(m_db.HasVideoInfo(CFileItem{"/movies/Film.mkv", false}));
  EXPECT_TRUE(m_db.HasVideoInfo(CFileItem{"/shows/Show/Show S01E01.mkv", false}));
  EXPECT_TRUE(m_db.HasVideoInfo(CFileItem{"/shows/Show/", true}));
  EXPECT_FALSE(m_db.HasVideoInfo(CFileItem{"/movies/Other.mkv", false}));
}

TEST_F(TestVideoDatabase, SettingAMoviesTrailerSetsThatMoviesTrailer)
{
  const int idMovie{AddMovie("/movies/Film.mkv")};
  ASSERT_GT(idMovie, 0);

  m_db.SetTrailerForMovie(idMovie, "/movies/Film-trailer.mkv");

  CVideoInfoTag tag;
  ASSERT_TRUE(m_db.GetMovieInfo("", tag, idMovie));
  EXPECT_EQ("/movies/Film-trailer.mkv", tag.m_strTrailer);
}

TEST_F(TestVideoDatabase, TaggedItemsAreListedByWhetherTheyHaveTheTag)
{
  const int idTagged{AddMovie("/movies/Tagged.mkv")};
  const int idOther{AddMovie("/movies/Other.mkv")};
  ASSERT_GT(idTagged, 0);
  ASSERT_GT(idOther, 0);
  const int idTag{m_db.AddTag("Favourites")};
  ASSERT_GT(idTag, 0);
  const auto listed{[this, idTag](bool tagged)
                    {
                      CFileItemList items;
                      m_db.GetVideosForTag(KODI::MEDIA::TYPE::MOVIE, idTag, tagged, items);
                      std::set<int> ids;
                      for (const auto& item : items)
                        ids.insert(item->GetVideoInfoTag()->m_iDbId);
                      return ids;
                    }};

  ASSERT_TRUE(m_db.AddTagToItems(idTag, {idTagged}, KODI::MEDIA::TYPE::MOVIE));
  EXPECT_EQ(std::set<int>{idTagged}, listed(true));
  EXPECT_EQ(std::set<int>{idOther}, listed(false));

  ASSERT_TRUE(m_db.RemoveTagFromItems(idTag, {idTagged}, KODI::MEDIA::TYPE::MOVIE));
  EXPECT_TRUE(listed(true).empty());
}

TEST_F(TestVideoDatabase, ASetMadeForAMovieStartsWithItsArtOnly)
{
  CVideoInfoTag film{Tag("/movies/Film.mkv")};
  CVideoInfoTag other{Tag("/movies/Other.mkv")};
  const int idMovie{m_db.SetDetailsForMovie(film, {{"poster", "/art/film.jpg"}})};
  const int idOther{m_db.SetDetailsForMovie(other, {{"poster", "/art/other.jpg"}})};
  ASSERT_GT(idMovie, 0);
  ASSERT_GT(idOther, 0);

  const int idSet{m_db.AddSetForMovie("Saga", idMovie)};
  ASSERT_GT(idSet, 0);
  EXPECT_EQ(idSet, m_db.AddSetForMovie("Saga", idOther));

  KODI::ART::Artwork art;
  ASSERT_TRUE(m_db.GetArtForItem(idSet, KODI::MEDIA::TYPE::VIDEO_COLLECTION, art));
  EXPECT_EQ("/art/film.jpg", art["poster"]);
}

TEST_F(TestVideoDatabase, RenamingASeasonNamesItAndKeepsItsOtherDetails)
{
  const int idShow{AddTvShow("/shows/Show/")};
  ASSERT_GT(idShow, 0);
  ASSERT_GT(AddEpisode(idShow, "/shows/Show/Show S01E01.mkv", 1), 0);
  CVideoInfoTag season;
  season.m_iSeason = 1;
  season.m_strPlot = "Plot";
  const int idSeason{m_db.SetDetailsForSeason(season, {{"poster", "/art/season.jpg"}}, idShow,
                                              m_db.GetSeasonId(idShow, 1))};
  ASSERT_GT(idSeason, 0);
  CVideoInfoTag item;
  item.SetMediaType(KODI::MEDIA::TYPE::SEASON);
  item.m_iDbId = idSeason;

  ASSERT_TRUE(m_db.SetItemTitle(item, "The Beginning"));

  CVideoInfoTag details;
  ASSERT_TRUE(m_db.GetDetailsOfItem(item, details));
  EXPECT_EQ("The Beginning", details.m_strTitle);
  EXPECT_EQ("Plot", details.m_strPlot);
  KODI::ART::Artwork art;
  ASSERT_TRUE(m_db.GetArtForItem(idSeason, KODI::MEDIA::TYPE::SEASON, art));
  EXPECT_EQ("/art/season.jpg", art["poster"]);
}

TEST_F(TestVideoDatabase, AnItemsDetailsAreOfTheVersionItNames)
{
  const MovieWithVersion movie{AddMovieWithVersion(KODI::ART::Artwork{}, KODI::ART::Artwork{})};
  ASSERT_GT(movie.versionFileId, 0);
  CVideoInfoTag item;
  item.SetMediaType(KODI::MEDIA::TYPE::MOVIE);
  item.m_iDbId = movie.movieId;
  item.m_iFileId = movie.versionFileId;

  CVideoInfoTag details;
  ASSERT_TRUE(m_db.GetDetailsOfItem(item, details));
  EXPECT_EQ(movie.versionFileId, details.m_iFileId);
}

namespace
{
class TestLibraryAnnouncements : public TestPlayStateAnnouncements
{
protected:
  long Updated(int idMovie) const
  {
    return m_announcements->CountUpdated(KODI::MEDIA::TYPE::MOVIE, idMovie);
  }
};
} // namespace

TEST_F(TestLibraryAnnouncements, RemovingAnExtraAnnouncesItsMovie)
{
  const int idMovie{AddMovie("/movies/Film.mkv")};
  ASSERT_GT(idMovie, 0);
  const int idExtra{AddAsset(idMovie, "/movies/Trailer.mkv", VideoAssetType::EXTRA)};
  ASSERT_GT(idExtra, 0);
  const long before{Updated(idMovie)};

  ASSERT_TRUE(m_db.RemoveVideoAsset(idExtra));

  EXPECT_EQ(before + 1, Updated(idMovie));
}

TEST_F(TestLibraryAnnouncements, ChangingTheDefaultVersionAnnouncesTheMovieOnce)
{
  const MovieWithVersion movie{AddMovieWithVersion(KODI::ART::Artwork{}, KODI::ART::Artwork{})};
  ASSERT_GT(movie.versionFileId, 0);
  const long before{Updated(movie.movieId)};

  ASSERT_TRUE(
      m_db.SetDefaultVideoVersion(VideoDbContentType::MOVIES, movie.movieId, movie.versionFileId));
  ASSERT_TRUE(
      m_db.SetDefaultVideoVersion(VideoDbContentType::MOVIES, movie.movieId, movie.versionFileId));

  EXPECT_EQ(before + 1, Updated(movie.movieId));
}

namespace
{
//! The play state rules, which need no database
class PlayStateRules : public CVideoDatabase
{
public:
  using CVideoDatabase::AddFilePlayState;
  using CVideoDatabase::EpisodePlayState;
};

//! \brief A tag as CVideoDatabase::GetFileInfo fills it: bookkeeping, not what the item is
CVideoInfoTag FileRow()
{
  CVideoInfoTag fileDetails;
  fileDetails.m_iFileId = 42;
  fileDetails.m_strPath = "plugin://plugin.video.example/";
  fileDetails.m_strFileNameAndPath = "plugin://plugin.video.example/?play=1";
  fileDetails.SetPlayCount(3);
  fileDetails.m_lastPlayed.SetFromDBDateTime("2026-08-10 21:49:28");
  fileDetails.m_dateAdded.SetFromDBDateTime("2026-08-01 10:00:00");
  fileDetails.SetResumePoint(120.0, 1800.0, "");
  return fileDetails;
}

/*! \brief A tag as an add-on's ListItem supplies it: a description, with no playback history. */
CVideoInfoTag PluginDescription()
{
  CVideoInfoTag details;
  details.SetMediaType(KODI::MEDIA::TYPE::EPISODE);
  details.m_strTitle = "Extraterrestrial Girl";
  details.m_strShowTitle = "Planetes";
  details.m_iSeason = 1;
  details.m_iEpisode = 7;
  details.m_strFileNameAndPath = "http://stream.example/real-stream.mkv";
  return details;
}

CVideoInfoTag Episode(int playCount, const char* lastPlayed)
{
  CVideoInfoTag episode;
  episode.SetMediaType(KODI::MEDIA::TYPE::EPISODE);
  episode.SetPlayCount(playCount);
  if (lastPlayed)
    episode.m_lastPlayed.SetFromDBDateTime(lastPlayed);
  return episode;
}

CDateTime DateTime(const char* value)
{
  CDateTime dateTime;
  dateTime.SetFromDBDateTime(value);
  return dateTime;
}

CVideoDatabase::PlayStateEdit PlayStateEdit(std::optional<int> playCount,
                                            std::optional<CDateTime> lastPlayed = std::nullopt)
{
  CVideoDatabase::PlayStateEdit edit;
  edit.playCount = playCount;
  edit.lastPlayed = lastPlayed;
  return edit;
}
} // namespace

TEST(TestVideoDatabasePlayState, KeepsTheDescriptionItIsAppliedTo)
{
  CVideoInfoTag details = PluginDescription();
  PlayStateRules::AddFilePlayState(FileRow(), details);

  EXPECT_EQ(KODI::MEDIA::TYPE::EPISODE, details.GetMediaType());
  EXPECT_EQ("Extraterrestrial Girl", details.m_strTitle);
  EXPECT_EQ("Planetes", details.m_strShowTitle);
  EXPECT_EQ(1, details.m_iSeason);
  EXPECT_EQ(7, details.m_iEpisode);
}

TEST(TestVideoDatabasePlayState, AddsTheStateTheFileRowCarries)
{
  CVideoInfoTag details = PluginDescription();
  PlayStateRules::AddFilePlayState(FileRow(), details);

  EXPECT_EQ(42, details.m_iFileId);
  EXPECT_EQ(3, details.GetPlayCount());
  EXPECT_EQ("2026-08-10 21:49:28", details.m_lastPlayed.GetAsDBDateTime());
  EXPECT_EQ("2026-08-01 10:00:00", details.m_dateAdded.GetAsDBDateTime());
  EXPECT_TRUE(details.GetResumePoint().IsSet());
  EXPECT_EQ(120.0, details.GetResumePoint().timeInSeconds);
}

TEST(TestVideoDatabasePlayState, LeavesThePathsAlone)
{
  // The row's paths are the plugin URL it is keyed on, not the file that plays.
  CVideoInfoTag details = PluginDescription();
  PlayStateRules::AddFilePlayState(FileRow(), details);

  EXPECT_EQ("http://stream.example/real-stream.mkv", details.m_strFileNameAndPath);
  EXPECT_TRUE(details.m_strPath.empty());
}

TEST(TestVideoDatabasePlayState, DoesNotDowngradeStateTheItemAlreadyHas)
{
  CVideoInfoTag details = PluginDescription();
  details.SetPlayCount(9);
  details.m_lastPlayed.SetFromDBDateTime("2026-08-10 23:00:00");
  details.SetResumePoint(600.0, 1800.0, "");

  PlayStateRules::AddFilePlayState(FileRow(), details);

  EXPECT_EQ(9, details.GetPlayCount());
  EXPECT_EQ("2026-08-10 23:00:00", details.m_lastPlayed.GetAsDBDateTime());
  EXPECT_EQ(600.0, details.GetResumePoint().timeInSeconds);
}

TEST(TestVideoDatabasePlayState, TakesALaterLastPlayedThanTheItemHas)
{
  CVideoInfoTag details = PluginDescription();
  details.m_lastPlayed.SetFromDBDateTime("2026-08-01 09:00:00");

  PlayStateRules::AddFilePlayState(FileRow(), details);

  EXPECT_EQ("2026-08-10 21:49:28", details.m_lastPlayed.GetAsDBDateTime());
}

TEST(TestVideoDatabasePlayState, FillsAnItemThatSaysNothingAboutItself)
{
  // Files.GetFileDetails on a file no add-on described: the row is all there is, and it must
  // still come through.
  CVideoInfoTag details;
  PlayStateRules::AddFilePlayState(FileRow(), details);

  EXPECT_EQ(42, details.m_iFileId);
  EXPECT_EQ(3, details.GetPlayCount());
  EXPECT_TRUE(details.GetResumePoint().IsSet());
}

TEST(TestVideoDatabasePlayState, APlaycountOnlyEditKeepsTheEpisodesLastPlayed)
{
  const auto state{
      PlayStateRules::EpisodePlayState(PlayStateEdit(1), Episode(0, "2026-08-10 21:49:28"))};

  ASSERT_TRUE(state);
  EXPECT_EQ(1, state->playCount);
  EXPECT_EQ("2026-08-10 21:49:28", state->lastPlayed.GetAsDBDateTime());
}

TEST(TestVideoDatabasePlayState, ANeverPlayedEpisodeHasNoTimeToKeep)
{
  const auto state{PlayStateRules::EpisodePlayState(PlayStateEdit(1), Episode(0, nullptr))};

  ASSERT_TRUE(state);
  EXPECT_FALSE(state->lastPlayed.IsValid());
}

TEST(TestVideoDatabasePlayState, AnEpisodeAlreadyAtThePlaycountIsLeftAlone)
{
  EXPECT_FALSE(
      PlayStateRules::EpisodePlayState(PlayStateEdit(2), Episode(2, "2026-08-10 21:49:28")));
}

TEST(TestVideoDatabasePlayState, ALastPlayedEditAppliesTheShowsTime)
{
  const auto state{
      PlayStateRules::EpisodePlayState(PlayStateEdit(std::nullopt, DateTime("2026-09-02 20:00:00")),
                                       Episode(2, "2026-08-10 21:49:28"))};

  ASSERT_TRUE(state);
  EXPECT_EQ(2, state->playCount);
  EXPECT_EQ("2026-09-02 20:00:00", state->lastPlayed.GetAsDBDateTime());
}

TEST_F(TestVideoDatabase, AFolderBeneathAShowIsNotDescribedAsTheShow)
{
  const std::string showFolder{"/tv/Planetes/"};
  const std::string seasonFolder{"/tv/Planetes/Season 1/"};
  const int idShowPath{m_db.AddPath(showFolder)};
  ASSERT_GE(idShowPath, 0);
  ASSERT_GE(m_db.AddPath(seasonFolder), 0);
  ASSERT_TRUE(m_db.ExecuteQuery("INSERT INTO tvshow (idShow, c00) VALUES (1, 'Planetes')"));
  ASSERT_TRUE(m_db.ExecuteQuery(StringUtils::Format(
      "INSERT INTO tvshowlinkpath (idShow, idPath) VALUES (1, {})", idShowPath)));

  CFileItem show{showFolder, true};
  EXPECT_TRUE(m_db.DescribePath(showFolder, show));
  EXPECT_EQ("Planetes", show.GetVideoInfoTag()->m_strTitle);

  CFileItem season{seasonFolder, true};
  EXPECT_FALSE(m_db.DescribePath(seasonFolder, season));
  EXPECT_TRUE(season.GetVideoInfoTag()->m_strTitle.empty());
}

TEST_F(TestVideoDatabase, AShowInTwoFoldersIsDescribedAtTheFolderAskedAbout)
{
  const std::string firstFolder{"/tv1/Planetes/"};
  const std::string secondFolder{"/tv2/Planetes/"};
  const int idFirstPath{m_db.AddPath(firstFolder)};
  const int idSecondPath{m_db.AddPath(secondFolder)};
  ASSERT_GE(idFirstPath, 0);
  ASSERT_GT(idSecondPath, idFirstPath);
  ASSERT_TRUE(m_db.ExecuteQuery("INSERT INTO tvshow (idShow, c00) VALUES (1, 'Planetes')"));
  ASSERT_TRUE(m_db.ExecuteQuery(StringUtils::Format(
      "INSERT INTO tvshowlinkpath (idShow, idPath) VALUES (1, {})", idFirstPath)));
  ASSERT_TRUE(m_db.ExecuteQuery(StringUtils::Format(
      "INSERT INTO tvshowlinkpath (idShow, idPath) VALUES (1, {})", idSecondPath)));

  CFileItem show{secondFolder, true};
  EXPECT_TRUE(m_db.DescribePath(secondFolder, show));
  EXPECT_EQ("Planetes", show.GetVideoInfoTag()->m_strTitle);
  EXPECT_EQ(secondFolder, show.GetVideoInfoTag()->GetPath());
}

// Player.GetItem relies on this for a library item, loading nothing more itself
TEST_F(TestVideoDatabase, AMovieVersionIsFilledWithItsDetailsFromItsOwnFile)
{
  const MovieWithVersion movie{AddMovieWithVersion(KODI::ART::Artwork{}, KODI::ART::Artwork{})};
  ASSERT_GT(movie.versionFileId, 0);
  const int idSet{m_db.AddSet("Saga")};
  ASSERT_GT(idSet, 0);
  m_db.SetMovieSet(movie.movieId, idSet);
  const std::string version{"/movies/Movie (2010)/Movie (2010) Standard Edition.mkv"};

  CFileItem item{version, false};
  ASSERT_TRUE(m_db.DescribePath(version, item));
  EXPECT_EQ(movie.movieId, item.GetVideoInfoTag()->m_iDbId);
  EXPECT_EQ(movie.versionFileId, item.GetVideoInfoTag()->m_iFileId);
  EXPECT_EQ(idSet, item.GetVideoInfoTag()->m_set.GetID());
}

TEST_F(TestVideoDatabase, AnEditMayNotClearTheLastPlayedDateOfAWatchedItem)
{
  const std::string film{"/movies/Film.mkv"};
  const std::string unwatched{"/movies/Unwatched.mkv"};
  const std::string episode{"/shows/Show/Show S01E01.mkv"};
  const std::string clip{"/musicvideos/Clip.mkv"};
  const int idMovie{AddMovie(film)};
  const int idUnwatched{AddMovie(unwatched)};
  const int idShow{AddTvShow("/shows/Show/")};
  const int idEpisode{AddEpisode(idShow, episode, 1)};
  CVideoInfoTag clipDetails{Tag(clip)};
  const int idClip{m_db.SetDetailsForMusicVideo(clipDetails, KODI::ART::Artwork{})};
  ASSERT_GT(idMovie, 0);
  ASSERT_GT(idUnwatched, 0);
  ASSERT_GT(idEpisode, 0);
  ASSERT_GT(idClip, 0);
  CDateTime lastPlayed;
  lastPlayed.SetFromDBDateTime("2026-09-02 20:00:00");
  for (const std::string& file : {film, episode, clip})
    ASSERT_TRUE(m_db.SetPlayCount(CFileItem(file, false), 2, lastPlayed).IsValid());

  const auto played{[this](const std::string& file)
                    {
                      const CDateTime date{m_db.GetLastPlayed(file)};
                      return std::pair{m_db.GetPlayCount(file),
                                       date.IsValid() ? date.GetAsDBDateTime() : ""};
                    }};
  struct Edit
  {
    KODI::MEDIA::TYPE type;
    int id;
    //! The file whose play state the edit would change
    std::string file;
    std::optional<int> playCount;
  };
  const Edit edits[]{
      {KODI::MEDIA::TYPE::MOVIE, idMovie, film, std::nullopt},
      {KODI::MEDIA::TYPE::EPISODE, idEpisode, episode, std::nullopt},
      {KODI::MEDIA::TYPE::MUSIC_VIDEO, idClip, clip, std::nullopt},
      // A show is watched while any of its episodes is
      {KODI::MEDIA::TYPE::TV_SHOW, idShow, episode, std::nullopt},
      // Marked watched with no date it was
      {KODI::MEDIA::TYPE::MOVIE, idUnwatched, unwatched, 1},
  };
  for (const auto& [type, id, file, playCount] : edits)
  {
    SCOPED_TRACE(KODI::MEDIA::NameOf(type) + " " + file);
    CVideoInfoTag item;
    item.SetMediaType(type);
    item.m_iDbId = id;
    CVideoInfoTag details;
    ASSERT_TRUE(m_db.GetDetailsOfItem(item, details));
    const std::string title{details.m_strTitle};
    const auto before{played(file)};
    details.m_strTitle = "Changed";

    EXPECT_EQ(CVideoDatabase::EditResult::REFUSED,
              m_db.EditItem(type, id, details, {}, {}, {}, PlayStateEdit(playCount, CDateTime{})));

    EXPECT_EQ(before, played(file));
    ASSERT_TRUE(m_db.GetDetailsOfItem(item, details));
    EXPECT_EQ(title, details.m_strTitle);
  }

  CVideoInfoTag details;
  ASSERT_TRUE(m_db.GetMovieInfo("", details, idMovie));
  EXPECT_EQ(CVideoDatabase::EditResult::DONE,
            m_db.EditItem(KODI::MEDIA::TYPE::MOVIE, idMovie, details, {}, {}, {},
                          PlayStateEdit(0, CDateTime{})));
  EXPECT_GE(0, m_db.GetPlayCount(film));
  EXPECT_FALSE(m_db.GetLastPlayed(film).IsValid());
}

TEST_F(TestVideoDatabase, AMusicVideoEditStaysInTheCallersTransaction)
{
  CVideoInfoTag tag{Tag("/musicvideos/Artist - Song.mkv")};
  const int idMVideo{m_db.SetDetailsForMusicVideo(tag, KODI::ART::Artwork{})};
  ASSERT_GT(idMVideo, 0);
  CVideoInfoTag details;
  ASSERT_TRUE(m_db.GetMusicVideoInfo("", details, idMVideo));

  details.m_strTitle = "Renamed";
  m_db.BeginTransaction();
  EXPECT_EQ(CVideoDatabase::EditResult::DONE,
            m_db.EditItem(KODI::MEDIA::TYPE::MUSIC_VIDEO, idMVideo, details, {}, {}, {}, {}));
  m_db.RollbackTransaction();

  ASSERT_TRUE(m_db.GetMusicVideoInfo("", details, idMVideo));
  EXPECT_EQ("Artist - Song.mkv", details.m_strTitle);
}

TEST_F(TestVideoDatabase, AnEditWhosePlayCountIsNotStoredIsUndone)
{
  const int idMovie{AddMovie("/movies/Film.mkv")};
  ASSERT_GT(idMovie, 0);
  CVideoInfoTag details;
  ASSERT_TRUE(m_db.GetMovieInfo("", details, idMovie));
  ASSERT_TRUE(m_db.ExecuteQuery("CREATE TRIGGER refusePlayCount BEFORE UPDATE OF playCount ON "
                                "files BEGIN SELECT RAISE(ABORT, 'refused'); END"));

  details.m_strTitle = "Renamed";
  EXPECT_EQ(
      CVideoDatabase::EditResult::FAILED,
      m_db.EditItem(KODI::MEDIA::TYPE::MOVIE, idMovie, details, {}, {}, {}, PlayStateEdit(2)));

  ASSERT_TRUE(m_db.GetMovieInfo("", details, idMovie));
  EXPECT_EQ("Film.mkv", details.m_strTitle);
}

TEST_F(TestVideoDatabase, EditingAShowsPlayCountGivesItToEachEpisode)
{
  const int idShow{AddTvShow("/shows/Show/")};
  ASSERT_GT(idShow, 0);
  const std::string first{"/shows/Show/Show S01E01.mkv"};
  const std::string second{"/shows/Show/Show S01E02.mkv"};
  ASSERT_GT(AddEpisode(idShow, first, 1), 0);
  ASSERT_GT(AddEpisode(idShow, second, 2), 0);
  CVideoInfoTag details;
  ASSERT_TRUE(m_db.GetTvShowInfo("", details, idShow));

  ASSERT_EQ(CVideoDatabase::EditResult::DONE, m_db.EditItem(KODI::MEDIA::TYPE::TV_SHOW, idShow,
                                                            details, {}, {}, {}, PlayStateEdit(1)));

  EXPECT_EQ(1, m_db.GetPlayCount(first));
  EXPECT_EQ(1, m_db.GetPlayCount(second));

  CDateTime lastPlayed;
  lastPlayed.SetFromDBDateTime("2026-09-02 20:00:00");
  ASSERT_TRUE(m_db.SetPlayCount(CFileItem(first, false), 2, lastPlayed).IsValid());
  ASSERT_TRUE(m_db.SetPlayCount(CFileItem(second, false), 1, lastPlayed).IsValid());
  ASSERT_EQ(CVideoDatabase::EditResult::DONE,
            m_db.EditItem(KODI::MEDIA::TYPE::TV_SHOW, idShow, details, {}, {}, {},
                          PlayStateEdit(0, CDateTime{})));

  for (const std::string& file : {first, second})
  {
    EXPECT_EQ(0, m_db.GetPlayCount(file)) << file;
    EXPECT_FALSE(m_db.GetLastPlayed(file).IsValid()) << file;
  }
}

TEST_F(TestVideoDatabase, AnItemIsAddedOnlyWhereTheLibraryHasNone)
{
  const std::string film{"/movies/Film.mkv"};
  ASSERT_GT(AddMovie(film), 0);
  int id{-1};
  CVideoInfoTag again;
  EXPECT_EQ(CVideoDatabase::AddResult::IN_LIBRARY,
            m_db.AddItem(KODI::MEDIA::TYPE::MOVIE, film, again, {}, -1, {}, id));

  CVideoInfoTag added;
  ASSERT_EQ(CVideoDatabase::AddResult::ADDED,
            m_db.AddItem(KODI::MEDIA::TYPE::MOVIE, "/movies/Other Film.mkv", added, {}, -1,
                         PlayStateEdit(2), id));
  CVideoInfoTag details;
  ASSERT_TRUE(m_db.GetMovieInfo("", details, id));
  EXPECT_EQ("Other Film.mkv", details.m_strTitle);
  EXPECT_EQ(2, m_db.GetPlayCount("/movies/Other Film.mkv"));

  CVideoInfoTag episode;
  EXPECT_EQ(CVideoDatabase::AddResult::NO_SUCH_SHOW,
            m_db.AddItem(KODI::MEDIA::TYPE::EPISODE, "/shows/Show/Show S01E01.mkv", episode, {},
                         9999, {}, id));
}

TEST_F(TestVideoDatabase, AFilesPlayStateEditIsRefusedOrStoredWhole)
{
  const std::string film{"/videos/film.mkv"};
  MarkPlayed(film, 1);
  CVideoInfoTag details;

  EXPECT_EQ(CVideoDatabase::EditResult::REFUSED,
            m_db.EditFilePlayState(film, PlayStateEdit(std::nullopt, CDateTime{}), details));

  CVideoDatabase::PlayStateEdit watchedAgain{PlayStateEdit(3)};
  watchedAgain.resume = CVideoDatabase::PlayStateEdit::Resume{600.0, 3600.0};
  ASSERT_EQ(CVideoDatabase::EditResult::DONE, m_db.EditFilePlayState(film, watchedAgain, details));
  EXPECT_EQ(3, details.GetPlayCount());
  EXPECT_EQ(600.0, details.GetResumePoint().timeInSeconds);
}
