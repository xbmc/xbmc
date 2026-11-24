/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "dbwrappers/test/DatabaseTestUtils.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabase.h"

#include <cstdlib>
#include <string>

#include <gtest/gtest.h>

namespace
{
//! The rows of a table, or -1 where they couldn't be counted, which GetSingleValueInt() gives as 0
int Count(const CDatabase& db, const std::string& table)
{
  const std::string count{db.GetSingleValue("SELECT count(*) FROM " + table)};
  return count.empty() ? -1 : std::atoi(count.c_str());
}

class TestDatabaseTransactions : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_name = StringUtils::Format("TestDatabaseTransactions{}",
                                 ::testing::UnitTest::GetInstance()->current_test_info()->name());
    XFILE::CFile::Delete(m_settings.host + m_name + ".db");
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, m_db.Connect(m_name, m_settings, true));
    ASSERT_TRUE(m_db.ExecuteQuery("CREATE TABLE kept (id INTEGER)"));
  }

  void TearDown() override
  {
    m_db.Close();
    XFILE::CFile::Delete(m_settings.host + m_name + ".db");
  }

  //! A statement run as it is, which the wrapper takes no part in
  bool Run(const std::string& sql) { return m_db.ExecuteQuery(sql); }

  bool Write(int id)
  {
    return m_db.ExecuteQuery(StringUtils::Format("INSERT INTO kept (id) VALUES ({})", id));
  }

  int Kept() const { return Count(m_db, "kept"); }

  //! A transaction of the database's own, to see that it goes on as before
  void ExpectATransactionToBeKept(int id)
  {
    const int kept{Kept()};
    ASSERT_GE(kept, 0);
    EXPECT_TRUE(m_db.BeginTransaction());
    EXPECT_TRUE(Write(id));
    EXPECT_TRUE(m_db.CommitTransaction());
    EXPECT_FALSE(m_db.InTransaction());
    EXPECT_EQ(kept + 1, Kept());
  }

  const DatabaseSettings m_settings{TestDatabaseSettings()};
  std::string m_name;
  CVideoDatabase m_db;
};
} // unnamed namespace

TEST_F(TestDatabaseTransactions, ACommittedTransactionKeepsWhatWasWrittenInIt)
{
  ASSERT_TRUE(m_db.BeginTransaction());
  EXPECT_TRUE(m_db.InTransaction());
  EXPECT_TRUE(Write(1));
  EXPECT_TRUE(Write(2));

  EXPECT_TRUE(m_db.CommitTransaction());

  EXPECT_FALSE(m_db.InTransaction());
  EXPECT_EQ(2, Kept());
}

TEST_F(TestDatabaseTransactions, ATransactionTheEngineRolledBackTakesNothingMoreAndFailsItsCommit)
{
  ASSERT_TRUE(m_db.BeginTransaction());
  EXPECT_TRUE(Write(1));
  // As SQLite does itself on some errors, such as a full disk
  ASSERT_TRUE(Run("ROLLBACK"));

  // Run alone, it would be committed, without what was written before it
  EXPECT_FALSE(Write(2));
  EXPECT_FALSE(m_db.CommitTransaction());

  EXPECT_FALSE(m_db.InTransaction());
  EXPECT_EQ(0, Kept());
  ExpectATransactionToBeKept(3);
}

TEST_F(TestDatabaseTransactions, ACommitThatFailsSaysSoAndLeavesNothingOfTheTransaction)
{
  // A reference checked as the transaction is committed, which then fails
  ASSERT_TRUE(Run("PRAGMA foreign_keys=ON"));
  ASSERT_TRUE(Run("CREATE TABLE parent (id INTEGER PRIMARY KEY)"));
  ASSERT_TRUE(Run("CREATE TABLE child (idParent INTEGER REFERENCES parent(id) DEFERRABLE INITIALLY "
                  "DEFERRED)"));

  ASSERT_TRUE(m_db.BeginTransaction());
  EXPECT_TRUE(Write(1));
  EXPECT_TRUE(Run("INSERT INTO child (idParent) VALUES (7)"));

  EXPECT_FALSE(m_db.CommitTransaction());

  EXPECT_FALSE(m_db.InTransaction());
  EXPECT_EQ(0, Kept());
  EXPECT_EQ(0, Count(m_db, "child"));
  // As some callers do where a commit fails, such as the add-on database's repository update
  m_db.RollbackTransaction();
  EXPECT_FALSE(m_db.InTransaction());
  ExpectATransactionToBeKept(2);
}

TEST_F(TestDatabaseTransactions, ATransactionThatCouldntBeBegunTakesNothingAndFailsItsCommit)
{
  // A transaction the connection is in already, of which the database knows nothing, so that its
  // own can't be begun
  ASSERT_TRUE(Run("BEGIN"));

  EXPECT_FALSE(m_db.BeginTransaction());
  // Its caller goes on to write in it, and to end it
  EXPECT_TRUE(m_db.InTransaction());
  EXPECT_FALSE(Write(1));
  EXPECT_FALSE(m_db.CommitTransaction());

  EXPECT_FALSE(m_db.InTransaction());
  ASSERT_TRUE(Run("ROLLBACK"));
  EXPECT_EQ(0, Kept());
  ExpectATransactionToBeKept(2);
}

TEST_F(TestDatabaseTransactions, ATransactionOfAClosedDatabaseIsNeitherBegunNorCommitted)
{
  m_db.Close();

  EXPECT_FALSE(m_db.BeginTransaction());
  EXPECT_FALSE(m_db.CommitTransaction());
  EXPECT_FALSE(m_db.InTransaction());
}

TEST_F(TestDatabaseTransactions, ATransactionOfADatabaseNeverOpenedIsNeitherBegunNorCommitted)
{
  // Not created, as where Connect() isn't asked to create it
  const std::string name{m_name + "Missing"};
  XFILE::CFile::Delete(m_settings.host + name + ".db");
  CVideoDatabase missing;
  ASSERT_EQ(CDatabase::ConnectionState::STATE_DATABASE_NOT_FOUND,
            missing.Connect(name, m_settings, false));

  EXPECT_FALSE(missing.BeginTransaction());
  EXPECT_FALSE(missing.CommitTransaction());
  EXPECT_FALSE(missing.InTransaction());
}

#if defined(HAS_MYSQL) || defined(HAS_MARIADB)
namespace
{
/*!
 * Skipped unless pointed at a server:
 *
 *   KODI_TEST_MYSQL_HOST=127.0.0.1 KODI_TEST_MYSQL_USER=kodi KODI_TEST_MYSQL_PASS=kodi
 *
 * The tests share a table, so CTest runs them one at a time (RESOURCE_LOCK in CMakeLists.txt). Each
 * TEST_F stays on one line, or gtest_add_tests() doesn't find it.
 */
class TestDatabaseTransactionsMysql : public ::testing::Test
{
protected:
  void SetUp() override
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

    const std::string name{"kodi_test_transactions"};
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, m_db.Connect(name, settings, true))
        << "could not create the test database on " << host;
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, m_other.Connect(name, settings, false));
    ASSERT_TRUE(m_db.ExecuteQuery("DROP TABLE IF EXISTS kept"));
    ASSERT_TRUE(m_db.ExecuteQuery(
        "CREATE TABLE kept (id INTEGER PRIMARY KEY, payload INTEGER) ENGINE=InnoDB"));
  }

  void TearDown() override
  {
    if (m_db.InTransaction())
      m_db.RollbackTransaction();
    if (m_other.InTransaction())
      m_other.RollbackTransaction();
    if (m_other.IsOpen())
      m_other.ExecuteQuery("DROP TABLE IF EXISTS kept");
  }

  bool Write(int id)
  {
    return m_db.ExecuteQuery(StringUtils::Format("INSERT INTO kept (id) VALUES ({})", id));
  }

  //! Counted on the other connection, which is never lost
  int Kept() const { return Count(m_other, "kept"); }

  //! As when the server restarts, or the network drops: the server rolls back the transaction
  bool LoseTheConnection()
  {
    const int connection{m_db.GetSingleValueInt("SELECT CONNECTION_ID()")};
    return connection > 0 && m_other.ExecuteQuery(StringUtils::Format("KILL {}", connection));
  }

  CVideoDatabase m_db;
  CVideoDatabase m_other;
};
} // unnamed namespace

TEST_F(TestDatabaseTransactionsMysql, ATransactionWhoseConnectionIsLostTakesNothingMore)
{
  ASSERT_TRUE(m_db.BeginTransaction());
  EXPECT_TRUE(Write(1));
  ASSERT_TRUE(LoseTheConnection());

  // Run again on a new connection, it would be committed, without what was written before it
  EXPECT_FALSE(Write(2));
  EXPECT_FALSE(m_db.CommitTransaction());

  EXPECT_FALSE(m_db.InTransaction());
  EXPECT_EQ(0, Kept());

  // The next transaction connects again, with no statement outside a transaction to do it first
  ASSERT_TRUE(m_db.BeginTransaction());
  EXPECT_TRUE(Write(3));
  EXPECT_TRUE(m_db.CommitTransaction());
  EXPECT_EQ(1, Kept());
}

TEST_F(TestDatabaseTransactionsMysql, ACommitWhoseConnectionIsLostFails)
{
  ASSERT_TRUE(m_db.BeginTransaction());
  EXPECT_TRUE(Write(1));
  // Lost after the last statement, so that the commit is the first to find it gone
  ASSERT_TRUE(LoseTheConnection());

  EXPECT_FALSE(m_db.CommitTransaction());

  EXPECT_FALSE(m_db.InTransaction());
  EXPECT_EQ(0, Kept());
  // As some callers do where a commit fails
  m_db.RollbackTransaction();
  EXPECT_FALSE(m_db.InTransaction());
  ASSERT_TRUE(m_db.BeginTransaction());
  EXPECT_TRUE(Write(2));
  EXPECT_TRUE(m_db.CommitTransaction());
  EXPECT_EQ(1, Kept());
}

TEST_F(TestDatabaseTransactionsMysql, ATransactionThatTimesOutOnALockTakesNothingMore)
{
  ASSERT_TRUE(Write(1));
  ASSERT_TRUE(m_db.ExecuteQuery("SET SESSION innodb_lock_wait_timeout=1"));
  // The other connection holds the row's lock until its transaction ends
  ASSERT_TRUE(m_other.BeginTransaction());
  ASSERT_TRUE(m_other.ExecuteQuery("UPDATE kept SET payload=1 WHERE id=1"));

  ASSERT_TRUE(m_db.BeginTransaction());
  EXPECT_TRUE(Write(2));
  // The server rolls back the statement, or with innodb_rollback_on_timeout the whole transaction
  EXPECT_FALSE(m_db.ExecuteQuery("UPDATE kept SET payload=2 WHERE id=1"));
  EXPECT_FALSE(Write(3));
  EXPECT_FALSE(m_db.CommitTransaction());
  m_other.RollbackTransaction();

  EXPECT_FALSE(m_db.InTransaction());
  EXPECT_EQ(1, Kept());
}

TEST_F(TestDatabaseTransactionsMysql, ATransactionEndedToBreakADeadlockTakesNothingMore)
{
  for (const int id : {1, 2, 3, 4})
    ASSERT_TRUE(Write(id));
  // Bounded, should the server not look for deadlocks
  ASSERT_TRUE(m_db.ExecuteQuery("SET SESSION innodb_lock_wait_timeout=5"));
  ASSERT_TRUE(m_other.ExecuteQuery("SET SESSION innodb_lock_wait_timeout=5"));
  // The other connection's transaction changes more rows, so the server ends this one to break the
  // deadlock
  ASSERT_TRUE(m_other.BeginTransaction());
  ASSERT_TRUE(m_other.ExecuteQuery("UPDATE kept SET payload=1 WHERE id IN (2, 3, 4)"));
  ASSERT_TRUE(m_db.BeginTransaction());
  ASSERT_TRUE(m_db.ExecuteQuery("UPDATE kept SET payload=2 WHERE id=1"));
  std::atomic<bool> written{false};
  std::thread other{[this, &written]
                    { written = m_other.ExecuteQuery("UPDATE kept SET payload=1 WHERE id=1"); }};
  // Once the other connection waits for the row this one holds
  std::this_thread::sleep_for(std::chrono::milliseconds{500});

  EXPECT_FALSE(m_db.ExecuteQuery("UPDATE kept SET payload=2 WHERE id=2"));
  EXPECT_FALSE(Write(5));
  EXPECT_FALSE(m_db.CommitTransaction());
  other.join();
  EXPECT_TRUE(written);
  EXPECT_TRUE(m_other.CommitTransaction());

  EXPECT_FALSE(m_db.InTransaction());
  EXPECT_EQ(4, Kept());
  EXPECT_EQ(0, Count(m_other, "kept WHERE payload=2"));
}
#endif
