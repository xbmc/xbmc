/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if defined(HAS_MYSQL) || defined(HAS_MARIADB)

#include "dbwrappers/dataset.h"
#include "settings/AdvancedSettings.h"
#include "video/VideoDatabase.h"

#include <cstdlib>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace
{
class CQueryableDatabase : public CVideoDatabase
{
public:
  std::unique_ptr<dbiplus::Dataset> Query(const std::string& sql)
  {
    std::unique_ptr<dbiplus::Dataset> ds{m_pDB->CreateDataset()};
    ds->query(sql);
    return ds;
  }
};
} // namespace

/*!
 * Skipped unless pointed at a server:
 *
 *   KODI_TEST_MYSQL_HOST=127.0.0.1 KODI_TEST_MYSQL_USER=kodi KODI_TEST_MYSQL_PASS=kodi
 */
TEST(TestMysqlDataset, MarksANullFieldAsNull)
{
  const char* const host{std::getenv("KODI_TEST_MYSQL_HOST")};
  const char* const user{std::getenv("KODI_TEST_MYSQL_USER")};
  const char* const pass{std::getenv("KODI_TEST_MYSQL_PASS")};
  if (!host || !user || !pass)
  {
    GTEST_SKIP() << "set KODI_TEST_MYSQL_HOST, KODI_TEST_MYSQL_USER and KODI_TEST_MYSQL_PASS "
                    "to run this test";
  }

  DatabaseSettings settings;
  settings.type = "mysql";
  settings.host = host;
  settings.user = user;
  settings.pass = pass;
  if (const char* const port{std::getenv("KODI_TEST_MYSQL_PORT")})
    settings.port = port;

  CQueryableDatabase db;
  ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED,
            db.Connect("kodi_test_mysqldataset", settings, true))
      << "could not create the test database on " << host;

  // A text and an integer column, each NULL in one row and set in the other
  ASSERT_TRUE(db.ExecuteQuery("DELETE FROM path WHERE strPath IN ('/a/', '/b/')"));
  ASSERT_TRUE(db.ExecuteQuery(
      "INSERT INTO path (strPath, strContent, idParentPath) VALUES ('/a/', NULL, 7)"));
  ASSERT_TRUE(db.ExecuteQuery(
      "INSERT INTO path (strPath, strContent, idParentPath) VALUES ('/b/', 'movies', NULL)"));

  const auto ds{db.Query("SELECT strContent, idParentPath FROM path "
                         "WHERE strPath IN ('/a/', '/b/') ORDER BY strPath")};
  ASSERT_EQ(2, ds->num_rows());

  EXPECT_TRUE(ds->fv(0).get_isNull());
  EXPECT_EQ("", ds->fv(0).get_asString());
  EXPECT_FALSE(ds->fv(1).get_isNull());
  EXPECT_EQ(7, ds->fv(1).get_asInt());

  ds->next();
  EXPECT_FALSE(ds->fv(0).get_isNull());
  EXPECT_EQ("movies", ds->fv(0).get_asString());
  EXPECT_TRUE(ds->fv(1).get_isNull());
  EXPECT_EQ(0, ds->fv(1).get_asInt());

  ds->close();
  db.Close();
}

#endif
