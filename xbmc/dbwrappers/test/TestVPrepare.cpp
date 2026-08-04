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

/*!
 * Sqlite only: escaping a value is what these cover, and MySQL escapes through
 * mysql_real_escape_string(), which needs the live connection a unit test has no way to hand it.
 * Which conversions become %q is backend independent - the pass is shared - so the format strings
 * below still stand in for both.
 */
struct VPrepareStringParamTest
{
  std::string format;
  std::string param;
  std::string expected;
};

const auto VPrepareStringParamTests = std::array{
    // %s is the escaping conversion: a quote in the value has to come out doubled
    VPrepareStringParamTest{"WHERE name = '%s'", "O'Brien", "WHERE name = 'O''Brien'"},
    // ... including behind the LIKE wildcards, where the %% before it is a literal percent and
    // does not turn the %s into the literal "%s" that %%s asks for
    VPrepareStringParamTest{"WHERE name LIKE '%%%s%%'", "O'Brien", "WHERE name LIKE '%O''Brien%'"},
    VPrepareStringParamTest{"WHERE name LIKE '%%%s'", "O'Brien", "WHERE name LIKE '%O''Brien'"},
    VPrepareStringParamTest{"WHERE name LIKE '%s%%'", "O'Brien", "WHERE name LIKE 'O''Brien%'"},
    // a literal %s still survives, and does not consume the conversion that follows it
    VPrepareStringParamTest{"CAST(strftime(\"%%s\",c01) AS REAL) = '%s'", "O'Brien",
                            "CAST(strftime(\"%s\",c01) AS REAL) = 'O''Brien'"},
};

class VPrepareStringParamTester : public testing::WithParamInterface<VPrepareStringParamTest>,
                                  public testing::Test
{
};

TEST_P(VPrepareStringParamTester, Sqlite)
{
  const auto params = GetParam();
  EXPECT_EQ(params.expected,
            PrepareSQL<dbiplus::SqliteDatabase>(params.format, params.param.c_str()));
}

INSTANTIATE_TEST_SUITE_P(TestDbWrappers,
                         VPrepareStringParamTester,
                         testing::ValuesIn(VPrepareStringParamTests));

// A conversion that is neither %% nor %s must not put the left to right scan out of step, so the
// LIKE wildcards after one are still recognised as literal percents. VIDEODB_ID_* queries look
// exactly like this.
TEST(TestDbWrappers, VPrepareMixedConversionsSqlite)
{
  EXPECT_EQ("WHERE c07 LIKE '%O''Brien%'",
            PrepareSQL<dbiplus::SqliteDatabase>("WHERE c%02d LIKE '%%%s%%'", 7, "O'Brien"));
}
