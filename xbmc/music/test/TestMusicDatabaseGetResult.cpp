/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/SpecialProtocol.h"
#include "music/Album.h"
#include "music/Artist.h"
#include "music/MusicDatabase.h"
#include "music/Song.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"

#include <gtest/gtest.h>

namespace
{

using GetResult = CDatabase::GetResult;

// An id that no test ever inserts, so every lookup of it misses.
constexpr int MISSING_ID = 4242;

// The album the songless-album test adds.
constexpr int ALBUM_ID = 4243;

DatabaseSettings SqliteSettings()
{
  DatabaseSettings settings;
  settings.type = "sqlite3";
  settings.host = CSpecialProtocol::TranslatePath("special://temp/");
  return settings;
}

} // namespace

class MusicDatabaseGetResultTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(m_db.Connect("TestMusicDatabaseGetResult", SqliteSettings(), true),
              CDatabase::ConnectionState::STATE_CONNECTED);
  }

  void TearDown() override { m_db.Close(); }

  CMusicDatabase m_db;
};

TEST(MusicDatabaseGetResultUnconnectedTest, EveryGetterReportsError)
{
  CMusicDatabase db;
  CSong song;
  CAlbum album;
  CArtist artist;

  EXPECT_EQ(db.TryGetSong(MISSING_ID, song), GetResult::Error);
  EXPECT_EQ(db.TryGetAlbum(MISSING_ID, album), GetResult::Error);
  EXPECT_EQ(db.TryGetArtist(MISSING_ID, artist), GetResult::Error);
  EXPECT_EQ(db.TryGetArtistExists(MISSING_ID), GetResult::Error);
}

TEST_F(MusicDatabaseGetResultTest, MissingRowsAreNotFound)
{
  CSong song;
  CAlbum album;
  CArtist artist;

  EXPECT_EQ(m_db.TryGetSong(MISSING_ID, song), GetResult::NotFound);
  EXPECT_EQ(m_db.TryGetAlbum(MISSING_ID, album), GetResult::NotFound);
  EXPECT_EQ(m_db.TryGetArtist(MISSING_ID, artist), GetResult::NotFound);
  EXPECT_EQ(m_db.TryGetArtistExists(MISSING_ID), GetResult::NotFound);
}

TEST_F(MusicDatabaseGetResultTest, BoolOverloadsStillReportFalseForMissingRows)
{
  CSong song;
  CAlbum album;
  CArtist artist;

  EXPECT_FALSE(m_db.GetSong(MISSING_ID, song));
  EXPECT_FALSE(m_db.GetAlbum(MISSING_ID, album));
  EXPECT_FALSE(m_db.GetArtist(MISSING_ID, artist));
  EXPECT_FALSE(m_db.GetArtistExists(MISSING_ID));
}

// A negative id is rejected before any query runs, which is still an answer
// about the record.
TEST_F(MusicDatabaseGetResultTest, NegativeIdIsNotFound)
{
  CAlbum album;
  CArtist artist;

  EXPECT_EQ(m_db.TryGetAlbum(-1, album), GetResult::NotFound);
  EXPECT_EQ(m_db.TryGetArtist(-1, artist), GetResult::NotFound);
}

// An album the database holds is retrieved even when it carries no songs, because an empty
// list is a result rather than a failure. The bool overload keeps reporting it as a failure.
TEST(MusicDatabaseGetResultSonglessAlbumTest, AlbumWithNoSongsIsRetrievedWithAnEmptySongList)
{
  CMusicDatabase db;
  // A database of this test's own, because it adds rows.
  ASSERT_EQ(db.Connect("TestMusicDatabaseSonglessAlbum", SqliteSettings(), true),
            CDatabase::ConnectionState::STATE_CONNECTED);

  // An album credited to the artist the schema seeds, and deliberately no songs.
  ASSERT_TRUE(db.ExecuteQuery(
      StringUtils::Format("INSERT INTO album (idAlbum, strAlbum) VALUES ({}, 'Album')", ALBUM_ID)));
  ASSERT_TRUE(db.ExecuteQuery(
      StringUtils::Format("INSERT INTO album_artist (idArtist, idAlbum, iOrder, strArtist) "
                          "VALUES ({}, {}, 0, 'Artist')",
                          BLANKARTIST_ID, ALBUM_ID)));

  CAlbum album;
  EXPECT_EQ(db.TryGetAlbum(ALBUM_ID, album, false), GetResult::Ok);
  EXPECT_EQ(album.strAlbum, "Album");

  CAlbum withSongs;
  EXPECT_EQ(db.TryGetAlbum(ALBUM_ID, withSongs, true), GetResult::Ok);
  EXPECT_TRUE(withSongs.songs.empty());

  // Unchanged from before the distinction existed.
  EXPECT_TRUE(db.GetAlbum(ALBUM_ID, album, false));
  EXPECT_FALSE(db.GetAlbum(ALBUM_ID, withSongs, true));

  db.Close();
}

// Refreshing the library info is a side effect of a commit, not a condition of it.
TEST(MusicDatabaseTransactionTest, CommitSucceedsWithoutAGui)
{
  CMusicDatabase db;
  ASSERT_EQ(db.Connect("TestMusicDatabaseTransaction", SqliteSettings(), true),
            CDatabase::ConnectionState::STATE_CONNECTED);

  db.BeginTransaction();
  EXPECT_TRUE(db.CommitTransaction());

  db.Close();
}

TEST(MusicDatabaseGetResultQueryFailureTest, FailedQueryReportsError)
{
  CMusicDatabase db;
  // A database of this test's own, because it damages the schema.
  ASSERT_EQ(db.Connect("TestMusicDatabaseGetResultQueryFailure", SqliteSettings(), true),
            CDatabase::ConnectionState::STATE_CONNECTED);

  // Remove what each getter selects from, so the query itself fails.
  ASSERT_TRUE(db.ExecuteQuery("DROP VIEW songview"));
  ASSERT_TRUE(db.ExecuteQuery("DROP VIEW albumview"));
  ASSERT_TRUE(db.ExecuteQuery("DROP VIEW artistview"));
  ASSERT_TRUE(db.ExecuteQuery("DROP TABLE artist"));

  CSong song;
  CAlbum album;
  CArtist artist;

  EXPECT_EQ(db.TryGetSong(MISSING_ID, song), GetResult::Error);
  EXPECT_EQ(db.TryGetAlbum(MISSING_ID, album), GetResult::Error);
  EXPECT_EQ(db.TryGetArtist(MISSING_ID, artist), GetResult::Error);
  EXPECT_EQ(db.TryGetArtistExists(MISSING_ID), GetResult::Error);

  // The bool overloads return the same false here as they do for a missing row.
  EXPECT_FALSE(db.GetSong(MISSING_ID, song));
  EXPECT_FALSE(db.GetAlbum(MISSING_ID, album));
  EXPECT_FALSE(db.GetArtist(MISSING_ID, artist));
  EXPECT_FALSE(db.GetArtistExists(MISSING_ID));

  db.Close();
}
