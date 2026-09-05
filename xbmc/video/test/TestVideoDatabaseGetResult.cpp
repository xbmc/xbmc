/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/SpecialProtocol.h"
#include "settings/AdvancedSettings.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"

#include <gtest/gtest.h>

namespace
{

using GetResult = CDatabase::GetResult;

// An id that no test ever inserts, so every lookup of it misses.
constexpr int MISSING_ID = 4242;

DatabaseSettings SqliteSettings()
{
  DatabaseSettings settings;
  settings.type = "sqlite3";
  settings.host = CSpecialProtocol::TranslatePath("special://temp/");
  return settings;
}

} // namespace

class VideoDatabaseGetResultTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(m_db.Connect("TestVideoDatabaseGetResult", SqliteSettings(), true),
              CDatabase::ConnectionState::STATE_CONNECTED);
  }

  void TearDown() override { m_db.Close(); }

  CVideoDatabase m_db;
};

TEST(VideoDatabaseGetResultUnconnectedTest, EveryGetterReportsError)
{
  CVideoDatabase db;
  CVideoInfoTag details;

  EXPECT_EQ(db.TryGetMovieInfo("", details, MISSING_ID), GetResult::Error);
  EXPECT_EQ(db.TryGetTvShowInfo("", details, MISSING_ID), GetResult::Error);
  EXPECT_EQ(db.TryGetSeasonInfo(MISSING_ID, details), GetResult::Error);
  EXPECT_EQ(db.TryGetEpisodeInfo("", details, MISSING_ID), GetResult::Error);
  EXPECT_EQ(db.TryGetMusicVideoInfo("", details, MISSING_ID), GetResult::Error);
  EXPECT_EQ(db.TryGetSetInfo(MISSING_ID, details), GetResult::Error);
  EXPECT_EQ(db.TryGetFileInfo("", details, MISSING_ID), GetResult::Error);
}

TEST_F(VideoDatabaseGetResultTest, MissingRowsAreNotFound)
{
  CVideoInfoTag details;

  EXPECT_EQ(m_db.TryGetMovieInfo("", details, MISSING_ID), GetResult::NotFound);
  EXPECT_EQ(m_db.TryGetTvShowInfo("", details, MISSING_ID), GetResult::NotFound);
  EXPECT_EQ(m_db.TryGetSeasonInfo(MISSING_ID, details), GetResult::NotFound);
  EXPECT_EQ(m_db.TryGetEpisodeInfo("", details, MISSING_ID), GetResult::NotFound);
  EXPECT_EQ(m_db.TryGetMusicVideoInfo("", details, MISSING_ID), GetResult::NotFound);
  EXPECT_EQ(m_db.TryGetSetInfo(MISSING_ID, details), GetResult::NotFound);
  EXPECT_EQ(m_db.TryGetFileInfo("", details, MISSING_ID), GetResult::NotFound);
}

TEST_F(VideoDatabaseGetResultTest, BoolOverloadsStillReportFalseForMissingRows)
{
  CVideoInfoTag details;

  EXPECT_FALSE(m_db.GetMovieInfo("", details, MISSING_ID));
  EXPECT_FALSE(m_db.GetTvShowInfo("", details, MISSING_ID));
  EXPECT_FALSE(m_db.GetSeasonInfo(MISSING_ID, details));
  EXPECT_FALSE(m_db.GetEpisodeInfo("", details, MISSING_ID));
  EXPECT_FALSE(m_db.GetMusicVideoInfo("", details, MISSING_ID));
  EXPECT_FALSE(m_db.GetSetInfo(MISSING_ID, details));
  EXPECT_FALSE(m_db.GetFileInfo("", details, MISSING_ID));
}

// A negative id is rejected before any query runs, which is still an answer
// about the record.
TEST_F(VideoDatabaseGetResultTest, NegativeIdIsNotFound)
{
  CVideoInfoTag details;

  EXPECT_EQ(m_db.TryGetSeasonInfo(-1, details), GetResult::NotFound);
  EXPECT_EQ(m_db.TryGetSetInfo(-1, details), GetResult::NotFound);
}

// Refreshing the library info is a side effect of a commit, not a condition of it.
TEST(VideoDatabaseTransactionTest, CommitSucceedsWithoutAGui)
{
  CVideoDatabase db;
  ASSERT_EQ(db.Connect("TestVideoDatabaseTransaction", SqliteSettings(), true),
            CDatabase::ConnectionState::STATE_CONNECTED);

  db.BeginTransaction();
  EXPECT_TRUE(db.CommitTransaction());

  db.Close();
}

TEST(VideoDatabaseGetResultQueryFailureTest, FailedQueryReportsError)
{
  CVideoDatabase db;
  // A database of this test's own, because it damages the schema.
  ASSERT_EQ(db.Connect("TestVideoDatabaseGetResultQueryFailure", SqliteSettings(), true),
            CDatabase::ConnectionState::STATE_CONNECTED);

  // Remove what each getter selects from, so the query itself fails.
  ASSERT_TRUE(db.ExecuteQuery("DROP VIEW movie_view"));
  ASSERT_TRUE(db.ExecuteQuery("DROP VIEW tvshow_view"));
  ASSERT_TRUE(db.ExecuteQuery("DROP VIEW episode_view"));
  ASSERT_TRUE(db.ExecuteQuery("DROP VIEW musicvideo_view"));
  ASSERT_TRUE(db.ExecuteQuery("DROP TABLE seasons"));
  ASSERT_TRUE(db.ExecuteQuery("DROP TABLE files"));

  CVideoInfoTag details;

  EXPECT_EQ(db.TryGetMovieInfo("", details, MISSING_ID), GetResult::Error);
  EXPECT_EQ(db.TryGetTvShowInfo("", details, MISSING_ID), GetResult::Error);
  EXPECT_EQ(db.TryGetSeasonInfo(MISSING_ID, details), GetResult::Error);
  EXPECT_EQ(db.TryGetEpisodeInfo("", details, MISSING_ID), GetResult::Error);
  EXPECT_EQ(db.TryGetMusicVideoInfo("", details, MISSING_ID), GetResult::Error);
  EXPECT_EQ(db.TryGetSetInfo(MISSING_ID, details), GetResult::Error);
  EXPECT_EQ(db.TryGetFileInfo("", details, MISSING_ID), GetResult::Error);

  // The bool overloads return the same false here as they do for a missing row.
  EXPECT_FALSE(db.GetMovieInfo("", details, MISSING_ID));
  EXPECT_FALSE(db.GetTvShowInfo("", details, MISSING_ID));
  EXPECT_FALSE(db.GetSeasonInfo(MISSING_ID, details));
  EXPECT_FALSE(db.GetEpisodeInfo("", details, MISSING_ID));
  EXPECT_FALSE(db.GetMusicVideoInfo("", details, MISSING_ID));
  EXPECT_FALSE(db.GetSetInfo(MISSING_ID, details));
  EXPECT_FALSE(db.GetFileInfo("", details, MISSING_ID));

  db.Close();
}
