/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "dbwrappers/sqlitedataset.h"
#if defined(HAS_MYSQL) || defined(HAS_MARIADB)
#include "dbwrappers/mysqldataset.h"
#endif

#include <array>
#include <string>

#include <gtest/gtest.h>

namespace
{
template<std::derived_from<dbiplus::Database> T>
std::string PrepareSQL(std::string_view sqlFormat, ...)
{
  std::string strResult;
  T db;

  va_list args;
  va_start(args, sqlFormat);
  strResult = db.vprepare(sqlFormat, args);
  va_end(args);

  return strResult;
}
} // namespace

struct VPrepareNoParamTest
{
  std::string format;
  std::string expectedSqlite;
  std::string expectedMySql;
};

const auto VPrepareNoParamTests = std::array{
    VPrepareNoParamTest{"foo", "foo", "foo"},
    // %% interpreted as single %, not as a format specifier combined with the next letter
    VPrepareNoParamTest{"SELECT %%s", "SELECT %s", "SELECT %s"},
    VPrepareNoParamTest{"SELECT %%", "SELECT %", "SELECT %"},
    VPrepareNoParamTest{"SELECT %%foo", "SELECT %foo", "SELECT %foo"},
    // strftime("%s", xxx) translation
    VPrepareNoParamTest{"strftime(\"%%s\",c01)", "strftime(\"%s\",c01)", "strftime(\"%s\",c01)"},
    VPrepareNoParamTest{"CAST(strftime(\"%%s\",c01) AS INTEGER)",
                        "CAST(strftime(\"%s\",c01) AS INTEGER)",
                        "CAST(UNIX_TIMESTAMP(c01) AS SIGNED INTEGER)"},
    VPrepareNoParamTest{"CAST(strftime(\"%%s\",c01) AS REAL)", "CAST(strftime(\"%s\",c01) AS REAL)",
                        "CAST(strftime(\"%s\",c01) AS REAL)"},
    // RANDOM function
    VPrepareNoParamTest{"SELECT RANDOM(), foo", "SELECT RANDOM(), foo", "SELECT RAND(), foo"},
    // CAST translation
    VPrepareNoParamTest{"SELECT CAST(foo AS TEXT), bar", "SELECT CAST(foo AS TEXT), bar",
                        "SELECT CAST(foo AS CHAR), bar"},
    VPrepareNoParamTest{"SELECT CAST(foo AS INTEGER), bar", "SELECT CAST(foo AS INTEGER), bar",
                        "SELECT CAST(foo AS SIGNED INTEGER), bar"},
    VPrepareNoParamTest{"SELECT CAST(foo AS REAL), bar", "SELECT CAST(foo AS REAL), bar",
                        "SELECT CAST(foo AS REAL), bar"},
    // COLLATE translation
    VPrepareNoParamTest{"SELECT foo COLLATE NOCASE, bar", "SELECT foo COLLATE NOCASE, bar",
                        "SELECT foo, bar"},
    VPrepareNoParamTest{"SELECT foo COLLATE ALPHANUM, bar", "SELECT foo COLLATE ALPHANUM, bar",
                        "SELECT foo, bar"},
    VPrepareNoParamTest{"SELECT foo COLLATE BINARY, bar", "SELECT foo COLLATE BINARY, bar",
                        "SELECT foo COLLATE BINARY, bar"},
};

class VPrepareNoParamTester : public testing::WithParamInterface<VPrepareNoParamTest>,
                              public testing::Test
{
};

TEST_P(VPrepareNoParamTester, Sqlite)
{
  const auto params = GetParam();
  EXPECT_EQ(params.expectedSqlite, PrepareSQL<dbiplus::SqliteDatabase>(params.format));
}

#if defined(HAS_MYSQL) || defined(HAS_MARIADB)
TEST_P(VPrepareNoParamTester, MySql)
{
  const auto params = GetParam();
  EXPECT_EQ(params.expectedMySql, PrepareSQL<dbiplus::MysqlDatabase>(params.format));
}
#endif

INSTANTIATE_TEST_SUITE_P(TestDbWrappers,
                         VPrepareNoParamTester,
                         testing::ValuesIn(VPrepareNoParamTests));
