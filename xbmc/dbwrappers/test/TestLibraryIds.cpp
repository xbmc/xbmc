/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "dbwrappers/test/DatabaseTestUtils.h"
#include "filesystem/File.h"
#include "music/MusicDatabase.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabase.h"
#if defined(HAS_MYSQL) || defined(HAS_MARIADB)
#include "dbwrappers/mysqldataset.h"
#endif

#include <array>
#include <cstdlib>
#include <functional>
#include <string>

#include <gtest/gtest.h>

namespace
{

struct IdTable
{
  const char* name;
  const char* key;
};

//! The tables whose ids JSON-RPC hands out as library item or category ids.
constexpr std::array VIDEO_TABLES{
    IdTable{"movie", "idMovie"},       IdTable{"tvshow", "idShow"},
    IdTable{"seasons", "idSeason"},    IdTable{"episode", "idEpisode"},
    IdTable{"musicvideo", "idMVideo"}, IdTable{"sets", "idSet"},
    IdTable{"genre", "genre_id"},      IdTable{"tag", "tag_id"},
};

constexpr std::array MUSIC_TABLES{
    IdTable{"artist", "idArtist"}, IdTable{"album", "idAlbum"}, IdTable{"song", "idSong"},
    IdTable{"genre", "idGenre"},   IdTable{"role", "idRole"},   IdTable{"source", "idSource"},
};

int InsertRow(CDatabase& db, const IdTable& table)
{
  EXPECT_TRUE(db.ExecuteQuery(
      StringUtils::Format("INSERT INTO `{}` ({}) VALUES (NULL)", table.name, table.key)));
  return db.GetSingleValueInt(
      StringUtils::Format("SELECT max({}) FROM `{}`", table.key, table.name));
}

//! Deletes the newest row, the one SQLite hands out again when a table lacks AUTOINCREMENT.
void ExpectADeletedIdIsNeverHandedOutAgain(CDatabase& db, const IdTable& table)
{
  const int deleted{InsertRow(db, table)};
  ASSERT_TRUE(db.ExecuteQuery(
      StringUtils::Format("DELETE FROM `{}` WHERE {}={}", table.name, table.key, deleted)));

  EXPECT_GT(InsertRow(db, table), deleted) << table.name << " reused a deleted id";
}

//! Restates a table as it was before AUTOINCREMENT, keeping every column the schema has now.
void DeclareReusableIds(CDatabase& db, const IdTable& table)
{
  std::string sql{db.GetSingleValue(StringUtils::Format(
      "SELECT sql FROM sqlite_master WHERE type='table' AND name='{}'", table.name))};
  ASSERT_NE(std::string::npos, sql.find(" AUTOINCREMENT")) << table.name;
  StringUtils::Replace(sql, " AUTOINCREMENT", "");

  ASSERT_TRUE(
      db.ExecuteQuery(StringUtils::Format("ALTER TABLE `{0}` RENAME TO `{0}_old`", table.name)));
  ASSERT_TRUE(db.ExecuteQuery(sql));
  ASSERT_TRUE(db.ExecuteQuery(
      StringUtils::Format("INSERT INTO `{0}` SELECT * FROM `{0}_old`", table.name)));
  ASSERT_TRUE(db.ExecuteQuery(StringUtils::Format("DROP TABLE `{}_old`", table.name)));
}

template<typename DB>
int CurrentSchemaVersion(const DatabaseSettings& settings)
{
  constexpr const char* PROBE{"TestLibraryIdsSchemaProbe"};

  DB probe;
  EXPECT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, probe.Connect(PROBE, settings, true));
  const int version{probe.GetSingleValueInt("SELECT idVersion FROM version")};
  probe.Close();
  XFILE::CFile::Delete(settings.host + PROBE + ".db");
  return version;
}

/*!
 * \brief Builds a database at an earlier schema version with ids that can be reused, runs the
 * real upgrade through CDatabaseManager, and checks the tables afterwards.
 */
template<typename DB, std::size_t N>
void ExpectTheUpgradeStopsIdReuse(DatabaseSettings CAdvancedSettings::* slot,
                                  const std::string& baseName,
                                  int previous,
                                  const std::array<IdTable, N>& tables,
                                  const std::function<void(CDatabase&)>& restatePrevious = {})
{
  const DatabaseSettings settings{TestDatabaseSettings()};
  const std::string& folder{settings.host};

  const int current{CurrentSchemaVersion<DB>(settings)};
  const std::string oldName{StringUtils::Format("{}{}", baseName, previous)};
  const std::string newName{StringUtils::Format("{}{}", baseName, current)};
  XFILE::CFile::Delete(folder + oldName + ".db");
  XFILE::CFile::Delete(folder + newName + ".db");

  std::array<int, N> kept{};
  {
    DB old;
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, old.Connect(oldName, settings, true));
    old.DropAnalytics();
    if (restatePrevious)
      restatePrevious(old);
    for (std::size_t i = 0; i < N; ++i)
    {
      DeclareReusableIds(old, tables[i]);
      kept[i] = InsertRow(old, tables[i]);
    }
    ASSERT_TRUE(old.ExecuteQuery(StringUtils::Format("UPDATE version SET idVersion={}", previous)));
    old.Close();
  }

  ASSERT_TRUE(UpgradeThroughManager(slot, baseName))
      << "the database manager could not upgrade " << baseName;

  DB migrated;
  ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED,
            migrated.Connect(newName, settings, false));
  EXPECT_EQ(current, migrated.GetSingleValueInt("SELECT idVersion FROM version"));

  for (std::size_t i = 0; i < N; ++i)
  {
    EXPECT_EQ(
        1, migrated.GetSingleValueInt(StringUtils::Format("SELECT count(*) FROM `{}` WHERE {}={}",
                                                          tables[i].name, tables[i].key, kept[i])))
        << tables[i].name << " lost its rows";
    ExpectADeletedIdIsNeverHandedOutAgain(migrated, tables[i]);
  }

  migrated.Close();
}

template<typename DB>
class TestLibraryIds : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_name = StringUtils::Format("TestLibraryIds{}",
                                 ::testing::UnitTest::GetInstance()->current_test_info()->name());
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, m_db.Connect(m_name, m_settings, true));
  }

  void TearDown() override
  {
    m_db.Close();
    XFILE::CFile::Delete(m_settings.host + m_name + ".db");
  }

  const DatabaseSettings m_settings{TestDatabaseSettings()};
  std::string m_name;
  DB m_db;
};

using TestVideoLibraryIds = TestLibraryIds<CVideoDatabase>;
using TestMusicLibraryIds = TestLibraryIds<CMusicDatabase>;

} // unnamed namespace

TEST_F(TestVideoLibraryIds, ANewDatabaseNeverReusesAnId)
{
  for (const auto& table : VIDEO_TABLES)
    ExpectADeletedIdIsNeverHandedOutAgain(m_db, table);
}

TEST_F(TestMusicLibraryIds, ANewDatabaseNeverReusesAnId)
{
  for (const auto& table : MUSIC_TABLES)
    ExpectADeletedIdIsNeverHandedOutAgain(m_db, table);
}

TEST(TestLibraryIdsMigration, TheVideoUpgradeKeepsRowsAndStopsIdReuse)
{
  ExpectTheUpgradeStopsIdReuse<CVideoDatabase>(&CAdvancedSettings::m_databaseVideo,
                                               "MyVideosLibraryIds", 150, VIDEO_TABLES);
}

TEST(TestLibraryIdsMigration, TheMusicUpgradeKeepsRowsAndStopsIdReuse)
{
  ExpectTheUpgradeStopsIdReuse<CMusicDatabase>(&CAdvancedSettings::m_databaseMusic,
                                               "MyMusicLibraryIds", 84, MUSIC_TABLES);
}

#if defined(HAS_MYSQL) || defined(HAS_MARIADB)
TEST(TestLibraryIdsMySql, AutoIncrementTakesThePlaceOfTheSQLiteKeyword)
{
  EXPECT_EQ("CREATE TABLE tag (tag_id integer primary key auto_increment , name TEXT)",
            dbiplus::MysqlDataset::EnforceAutoIncrement(
                "CREATE TABLE tag (tag_id integer primary key AUTOINCREMENT, name TEXT)"));
}

TEST(TestLibraryIdsMySql, AKeyWithoutTheSQLiteKeywordStillGetsAutoIncrement)
{
  EXPECT_EQ("CREATE TABLE path (idPath INTEGER PRIMARY KEY auto_increment , strPath TEXT)",
            dbiplus::MysqlDataset::EnforceAutoIncrement(
                "CREATE TABLE path (idPath INTEGER PRIMARY KEY, strPath TEXT)"));
}

template<typename DB, std::size_t N>
void ExpectAMySqlDatabaseNeverReusesAnId(const std::string& name,
                                         const std::array<IdTable, N>& tables)
{
  const char* const host{std::getenv("KODI_TEST_MYSQL_HOST")};
  const char* const user{std::getenv("KODI_TEST_MYSQL_USER")};
  const char* const pass{std::getenv("KODI_TEST_MYSQL_PASS")};

  if (!host || !user || !pass)
  {
    GTEST_SKIP() << "set KODI_TEST_MYSQL_HOST, KODI_TEST_MYSQL_USER and KODI_TEST_MYSQL_PASS "
                    "to run the MySQL half of these tests";
  }

  DatabaseSettings settings;
  settings.type = "mysql";
  settings.host = host;
  settings.user = user;
  settings.pass = pass;
  if (const char* const port{std::getenv("KODI_TEST_MYSQL_PORT")})
    settings.port = port;

  DB db;
  ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, db.Connect(name, settings, true))
      << "could not create the test database on " << host;

  for (const auto& table : tables)
    ExpectADeletedIdIsNeverHandedOutAgain(db, table);

  db.Close();
}

/*!
 * The library tables as MySQL creates them from the SQLite definitions. Skipped unless pointed at
 * a server:
 *
 *   KODI_TEST_MYSQL_HOST=127.0.0.1 KODI_TEST_MYSQL_USER=kodi KODI_TEST_MYSQL_PASS=kodi
 */
TEST(TestLibraryIdsMySql, ANewVideoDatabaseNeverReusesAnId)
{
  ExpectAMySqlDatabaseNeverReusesAnId<CVideoDatabase>("kodi_test_libraryids_video", VIDEO_TABLES);
}

TEST(TestLibraryIdsMySql, ANewMusicDatabaseNeverReusesAnId)
{
  ExpectAMySqlDatabaseNeverReusesAnId<CMusicDatabase>("kodi_test_libraryids_music", MUSIC_TABLES);
}
#endif
