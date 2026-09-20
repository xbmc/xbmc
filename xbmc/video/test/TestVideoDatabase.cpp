/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "cores/VideoSettings.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/AdvancedSettings.h"
#include "utils/Artwork.h"
#include "utils/URIUtils.h"
#include "video/Bookmark.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"

#include <memory>
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
  }

  void TearDown() override
  {
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
