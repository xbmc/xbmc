/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItemList.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "music/MusicDatabase.h"
#include "settings/AdvancedSettings.h"

#include <gtest/gtest.h>

namespace
{
constexpr const char* DB_NAME = "TestMusicDatabase";

class TestMusicDatabase : public testing::Test
{
protected:
  void SetUp() override
  {
    m_settings.type = "sqlite3";
    m_settings.host = CSpecialProtocol::TranslatePath("special://temp/");
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, m_db.Connect(DB_NAME, m_settings, true));
  }

  void TearDown() override
  {
    m_db.Close();
    XFILE::CFile::Delete(m_settings.host + DB_NAME + ".db");
  }

  void ExpectNoScratchTables()
  {
    EXPECT_EQ("0", m_db.GetSingleValue("SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' "
                                       "AND name IN ('tempDisco', 'tempAlbum')"));
  }

  DatabaseSettings m_settings;
  CMusicDatabase m_db;
};
} // namespace

TEST_F(TestMusicDatabase, DiscographyCleansUpAfterSuccess)
{
  ASSERT_TRUE(m_db.ExecuteQuery("INSERT INTO discography VALUES (2, 'Album', '2026', 'release')"));
  CFileItemList items;
  EXPECT_TRUE(m_db.GetArtistDiscography(2, items));
  EXPECT_EQ(1, items.Size());
  ExpectNoScratchTables();
}

TEST_F(TestMusicDatabase, DiscographyCleansUpAfterEmptyResult)
{
  CFileItemList items;
  EXPECT_TRUE(m_db.GetArtistDiscography(2, items));
  EXPECT_EQ(0, items.Size());
  ExpectNoScratchTables();
}

TEST_F(TestMusicDatabase, DiscographyCleansUpAfterQueryError)
{
  ASSERT_TRUE(m_db.ExecuteQuery("DROP TABLE discography"));
  CFileItemList items;
  EXPECT_FALSE(m_db.GetArtistDiscography(2, items));
  ExpectNoScratchTables();
}

TEST_F(TestMusicDatabase, DiscographyRecoversLeftoverTables)
{
  ASSERT_TRUE(m_db.ExecuteQuery("CREATE TABLE tempDisco (strAlbum TEXT)"));
  ASSERT_TRUE(m_db.ExecuteQuery("CREATE TABLE tempAlbum (strAlbum TEXT)"));
  CFileItemList items;
  EXPECT_TRUE(m_db.GetArtistDiscography(2, items));
  EXPECT_EQ(0, items.Size());
  ExpectNoScratchTables();
}

TEST_F(TestMusicDatabase, CommitTransactionSucceedsWithoutAGUI)
{
  // The test process has no GUI, so there are no library info booleans to refresh
  m_db.BeginTransaction();
  ASSERT_TRUE(m_db.ExecuteQuery("INSERT INTO discography VALUES (2, 'Album', '2026', 'release')"));
  EXPECT_TRUE(m_db.CommitTransaction());
  EXPECT_EQ("1", m_db.GetSingleValue("SELECT COUNT(*) FROM discography"));
}
