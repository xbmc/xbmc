/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/AdvancedSettings.h"
#include "video/VideoDatabase.h"

#include <string>

#include <gtest/gtest.h>

namespace
{
constexpr const char* DB_NAME{"TestVideoDatabaseMigration.db"};
} // namespace

/*!
 * \brief Migration of the name columns of actor / genre / country / studio / tag to the NOCASE
 * collation (schema version 149).
 *
 * The starting point is a current database whose five tables are put back to an uncollated
 * definition, standing in for one created before v149. That covers the migration code, not the
 * exact shape of every schema it can be handed - a v148 database differing from the definitions
 * below would not be caught here.
 */
class TestVideoDatabaseMigration : public testing::Test
{
protected:
  CVideoDatabase m_db;

  static std::string DbPath()
  {
    return CSpecialProtocol::TranslatePath(std::string("special://temp/") + DB_NAME);
  }

  void SetUp() override
  {
    XFILE::CFile::Delete(DbPath());

    DatabaseSettings settings;
    settings.type = "sqlite3";
    settings.name = "test";
    settings.host = CSpecialProtocol::TranslatePath("special://temp/");
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, m_db.Connect(DB_NAME, settings, true));

    // Analytics have to go before the tables they cover can be replaced. UpdateTables() runs
    // without them in a real update too.
    m_db.DropAnalytics();

    Uncollate("actor", "actor_id INTEGER PRIMARY KEY, name TEXT, art_urls TEXT");
    Uncollate("genre", "genre_id integer primary key, name TEXT");
    Uncollate("country", "country_id integer primary key, name TEXT");
    Uncollate("studio", "studio_id integer primary key, name TEXT");
    Uncollate("tag", "tag_id integer primary key, name TEXT");
  }

  void TearDown() override
  {
    m_db.Close();
    XFILE::CFile::Delete(DbPath());
  }

  //! \brief Replaces a freshly created (and therefore empty) table with its pre-v149 definition
  void Uncollate(const std::string& table, const std::string& definition)
  {
    ASSERT_TRUE(m_db.ExecuteQuery("DROP TABLE " + table));
    ASSERT_TRUE(m_db.ExecuteQuery("CREATE TABLE " + table + " (" + definition + ")"));
  }

  //! \brief Runs the update the way CDatabaseManager::UpdateVersion() does. Every block below 149
  //! is guarded by a lower version, so only the v149 one applies.
  void Migrate()
  {
    m_db.DropAnalytics();
    m_db.BeginTransaction();
    m_db.UpdateTables(148);
    m_db.CreateAnalytics();
    ASSERT_TRUE(m_db.CommitTransaction());
  }

  void Exec(const std::string& query) { ASSERT_TRUE(m_db.ExecuteQuery(query)); }

  int Count(const std::string& query) const { return m_db.GetSingleValueInt(query); }

  std::string Value(const std::string& query) const { return m_db.GetSingleValue(query); }
};

TEST_F(TestVideoDatabaseMigration, MergesNamesDifferingOnlyInCase)
{
  Exec("INSERT INTO actor (actor_id, name, art_urls) VALUES "
       "(1, 'Tom Hanks', ''), (2, 'tom hanks', ''), (3, 'TOM HANKS', ''), (4, 'Meg Ryan', '')");

  Migrate();

  EXPECT_EQ(2, Count("SELECT COUNT(*) FROM actor"));
  EXPECT_EQ("Tom Hanks", Value("SELECT name FROM actor WHERE actor_id = 1"));
  EXPECT_EQ(0, Count("SELECT COUNT(*) FROM actor WHERE actor_id IN (2, 3)"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM actor WHERE actor_id = 4"));
}

TEST_F(TestVideoDatabaseMigration, AppliesTheCollationToEveryNameTable)
{
  Exec("INSERT INTO actor (actor_id, name, art_urls) VALUES (1, 'Tom Hanks', '')");
  Exec("INSERT INTO genre (genre_id, name) VALUES (1, 'Science Fiction')");
  Exec("INSERT INTO country (country_id, name) VALUES (1, 'New Zealand')");
  Exec("INSERT INTO studio (studio_id, name) VALUES (1, 'Studio Ghibli')");
  Exec("INSERT INTO tag (tag_id, name) VALUES (1, 'Christmas')");

  Migrate();

  // the lookups of AddActor() and AddToTable(), which is what the collation is there for
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM actor WHERE name = 'TOM HANKS'"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM genre WHERE name = 'science fiction'"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM country WHERE name = 'NEW ZEALAND'"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM studio WHERE name = 'studio ghibli'"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM tag WHERE name = 'CHRISTMAS'"));

  // CreateAnalytics() only logs a statement it cannot run, so the index has to be asserted on
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM sqlite_master WHERE type = 'index' AND "
                     "name = 'ix_actor_1'"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM sqlite_master WHERE type = 'index' AND "
                     "name = 'ix_tag_1'"));

  // and it is now case-insensitively unique
  EXPECT_FALSE(m_db.ExecuteQuery("INSERT INTO actor (actor_id, name, art_urls) VALUES "
                                 "(99, 'tom hanks', '')"));
}

TEST_F(TestVideoDatabaseMigration, KeepsNamesThatOnlyLookAlikeToLike)
{
  // a merge matching with LIKE rather than '=' would take the % and the _ for wildcards and
  // collapse these into one row each
  Exec("INSERT INTO actor (actor_id, name, art_urls) VALUES "
       "(1, '50% Off', ''), (2, '500 Off', ''), (3, 'Bob_', ''), (4, 'Bobs', ''), "
       "(5, 'O''Neal', ''), (6, 'o''neal', '')");

  Migrate();

  EXPECT_EQ(5, Count("SELECT COUNT(*) FROM actor"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM actor WHERE actor_id = 1"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM actor WHERE actor_id = 2"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM actor WHERE actor_id = 3"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM actor WHERE actor_id = 4"));
  // the quoted name is the one pair that does differ only in case
  EXPECT_EQ("O'Neal", Value("SELECT name FROM actor WHERE actor_id = 5"));
  EXPECT_EQ(0, Count("SELECT COUNT(*) FROM actor WHERE actor_id = 6"));
}

TEST_F(TestVideoDatabaseMigration, KeepsEveryNullName)
{
  // a unique index holds as many null names as it likes, so they are not duplicates of each other.
  // Merging them is impossible anyway - no '=' comparison can pick a row out of that group again -
  // and treating them as duplicates would leave the group behind and fail the whole update.
  Exec("INSERT INTO actor (actor_id, name, art_urls) VALUES "
       "(1, NULL, ''), (2, NULL, ''), (3, NULL, ''), "
       "(4, 'Tom Hanks', ''), (5, 'tom hanks', '')");

  Migrate();

  EXPECT_EQ(3, Count("SELECT COUNT(*) FROM actor WHERE name IS NULL"));
  // the real duplicate is still merged
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM actor WHERE name = 'TOM HANKS'"));
  EXPECT_EQ(4, Count("SELECT COUNT(*) FROM actor"));
}

TEST_F(TestVideoDatabaseMigration, DoesNotTakeAnEmptyNameForANullOne)
{
  // a null name reaches the merge as an empty string, so a group of nulls would otherwise pick up
  // the rows that are genuinely named ''
  Exec("INSERT INTO actor (actor_id, name, art_urls) VALUES "
       "(1, NULL, ''), (2, NULL, ''), (3, '', 'first'), (4, '', 'second')");
  Exec("INSERT INTO actor_link (actor_id, media_id, media_type, role, cast_order) VALUES "
       "(3, 10, 'movie', 'Self', 0), (4, 11, 'movie', 'Self', 0)");

  Migrate();

  EXPECT_EQ(2, Count("SELECT COUNT(*) FROM actor WHERE name IS NULL"));
  // the two '' rows are duplicates of each other, and of neither null row
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM actor WHERE name = ''"));
  EXPECT_EQ(3, Count("SELECT COUNT(*) FROM actor"));
  EXPECT_EQ(2, Count("SELECT COUNT(*) FROM actor_link WHERE actor_id = 3"));
  EXPECT_EQ(0, Count("SELECT COUNT(*) FROM actor_link WHERE actor_id IN (1, 2, 4)"));
}

TEST_F(TestVideoDatabaseMigration, RepointsAndDedupesLinks)
{
  Exec("INSERT INTO actor (actor_id, name, art_urls) VALUES (1, 'Tom Hanks', ''), "
       "(2, 'tom hanks', '')");
  // the same media and role through both rows, so one of the two has to go
  Exec("INSERT INTO actor_link (actor_id, media_id, media_type, role, cast_order) VALUES "
       "(1, 10, 'movie', 'Woody', 0), (2, 10, 'movie', 'Woody', 1), "
       "(2, 10, 'movie', 'Sheriff', 2), (2, 11, 'movie', 'Woody', 0)");
  // director_link and writer_link have no role in their unique index
  Exec("INSERT INTO director_link (actor_id, media_id, media_type) VALUES "
       "(1, 10, 'movie'), (2, 10, 'movie'), (2, 12, 'movie')");
  Exec("INSERT INTO writer_link (actor_id, media_id, media_type) VALUES "
       "(1, 10, 'movie'), (2, 10, 'movie')");

  Migrate();

  EXPECT_EQ(0, Count("SELECT COUNT(*) FROM actor_link WHERE actor_id = 2"));
  EXPECT_EQ(3, Count("SELECT COUNT(*) FROM actor_link"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM actor_link WHERE media_id = 10 AND role = 'Woody'"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM actor_link WHERE media_id = 10 AND role = 'Sheriff'"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM actor_link WHERE media_id = 11"));

  EXPECT_EQ(2, Count("SELECT COUNT(*) FROM director_link"));
  EXPECT_EQ(0, Count("SELECT COUNT(*) FROM director_link WHERE actor_id = 2"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM writer_link"));
}

TEST_F(TestVideoDatabaseMigration, RepointsAndDedupesLinksOfTheOtherTables)
{
  Exec("INSERT INTO tag (tag_id, name) VALUES (1, 'Christmas'), (2, 'christmas')");
  Exec("INSERT INTO tag_link (tag_id, media_id, media_type) VALUES "
       "(1, 10, 'movie'), (2, 10, 'movie'), (2, 11, 'movie')");

  Migrate();

  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM tag"));
  EXPECT_EQ(2, Count("SELECT COUNT(*) FROM tag_link"));
  EXPECT_EQ(0, Count("SELECT COUNT(*) FROM tag_link WHERE tag_id = 2"));
}

TEST_F(TestVideoDatabaseMigration, MovesArtOfATypeTheSurvivorHasNot)
{
  Exec("INSERT INTO actor (actor_id, name, art_urls) VALUES (1, 'Tom Hanks', ''), "
       "(2, 'tom hanks', '')");
  Exec("INSERT INTO art (art_id, media_id, media_type, type, url) VALUES "
       "(1, 1, 'actor', 'thumb', 'kept'), (2, 2, 'actor', 'thumb', 'dropped'), "
       "(3, 2, 'actor', 'fanart', 'moved'), (4, 2, 'director', 'thumb', 'moved too')");

  Migrate();

  EXPECT_EQ(3, Count("SELECT COUNT(*) FROM art"));
  EXPECT_EQ(0, Count("SELECT COUNT(*) FROM art WHERE media_id = 2"));
  // the survivor keeps its own url where both hold the same media type and type
  EXPECT_EQ("kept", Value("SELECT url FROM art WHERE media_id = 1 AND media_type = 'actor' AND "
                          "type = 'thumb'"));
  EXPECT_EQ("moved", Value("SELECT url FROM art WHERE media_id = 1 AND media_type = 'actor' AND "
                           "type = 'fanart'"));
  EXPECT_EQ("moved too", Value("SELECT url FROM art WHERE media_id = 1 AND "
                               "media_type = 'director' AND type = 'thumb'"));
}

TEST_F(TestVideoDatabaseMigration, GivesTheSurvivorTheArtUrlsItHasNot)
{
  Exec("INSERT INTO actor (actor_id, name, art_urls) VALUES "
       "(1, 'Tom Hanks', ''), (2, 'tom hanks', '<thumb>second</thumb>'), "
       "(3, 'TOM HANKS', '<thumb>third</thumb>')");

  Migrate();

  EXPECT_EQ("<thumb>second</thumb>", Value("SELECT art_urls FROM actor WHERE actor_id = 1"));
}

TEST_F(TestVideoDatabaseMigration, LeavesTheArtUrlsOfTheSurvivorAlone)
{
  Exec("INSERT INTO actor (actor_id, name, art_urls) VALUES "
       "(1, 'Tom Hanks', '<thumb>first</thumb>'), (2, 'tom hanks', '<thumb>second</thumb>')");

  Migrate();

  EXPECT_EQ("<thumb>first</thumb>", Value("SELECT art_urls FROM actor WHERE actor_id = 1"));
}

TEST_F(TestVideoDatabaseMigration, CopesWithArtUrlsHeldAsNull)
{
  Exec("INSERT INTO actor (actor_id, name, art_urls) VALUES "
       "(1, 'Tom Hanks', NULL), (2, 'tom hanks', NULL)");

  Migrate();

  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM actor"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM actor WHERE art_urls IS NULL"));
}

TEST_F(TestVideoDatabaseMigration, RunsAgainWithoutLoss)
{
  Exec("INSERT INTO actor (actor_id, name, art_urls) VALUES "
       "(1, 'Tom Hanks', '<thumb>first</thumb>'), (2, 'tom hanks', ''), (3, 'Meg Ryan', '')");
  Exec("INSERT INTO actor_link (actor_id, media_id, media_type, role, cast_order) VALUES "
       "(1, 10, 'movie', 'Woody', 0)");

  Migrate();
  Migrate();

  EXPECT_EQ(2, Count("SELECT COUNT(*) FROM actor"));
  EXPECT_EQ("<thumb>first</thumb>", Value("SELECT art_urls FROM actor WHERE actor_id = 1"));
  EXPECT_EQ("Meg Ryan", Value("SELECT name FROM actor WHERE actor_id = 3"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM actor_link"));
  EXPECT_EQ(1, Count("SELECT COUNT(*) FROM actor WHERE name = 'tom HANKS'"));
}
