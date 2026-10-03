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
#include <cstdint>
#include <string>

#include <gtest/gtest.h>

namespace
{
template<std::derived_from<dbiplus::Database> T>
std::string TestPrepareSQL(std::string_view sqlFormat, ...)
{
  std::string strResult;
  T db;

  va_list args;
  va_start(args, sqlFormat);
  strResult = db.vprepare(sqlFormat, args);
  va_end(args);

  return strResult;
}

//! \brief Reaches the protected conversion pass. Abstract and never instantiated, so the pure
//! virtuals of Database can stay unimplemented.
class TestDatabase : public dbiplus::Database
{
public:
  using dbiplus::Database::EscapeStringConversions;
};

std::string TestEscapeStringConversions(std::string_view format)
{
  std::string result{format};
  TestDatabase::EscapeStringConversions(result);
  return result;
}
} // namespace

struct EscapeConversionsTest
{
  std::string format;
  std::string expected;
};

const auto EscapeConversionsTests = std::array{
    // nothing to rewrite
    EscapeConversionsTest{"", ""},
    EscapeConversionsTest{"SELECT foo", "SELECT foo"},
    // %s is the escaping conversion and becomes %q
    EscapeConversionsTest{"%s", "%q"},
    EscapeConversionsTest{"WHERE name = '%s'", "WHERE name = '%q'"},
    EscapeConversionsTest{"%s%s", "%q%q"},
    // %% is a literal percent, so the %s that %%s hides is the literal "%s" and must survive
    EscapeConversionsTest{"%%s", "%%s"},
    EscapeConversionsTest{"strftime(\"%%s\",c01)", "strftime(\"%%s\",c01)"},
    // ... while the LIKE wildcard idiom is a literal percent followed by a conversion
    EscapeConversionsTest{"'%%%s%%'", "'%%%q%%'"},
    EscapeConversionsTest{"'%%%s'", "'%%%q'"},
    EscapeConversionsTest{"'%s%%'", "'%q%%'"},
    // a literal %s does not consume the conversion that follows it
    EscapeConversionsTest{"%%s '%s'", "%%s '%q'"},
    // any other conversion is stepped over whole, keeping the scan in step with the percents that
    // follow it. VIDEODB_ID_* queries look exactly like this.
    EscapeConversionsTest{"%i %s", "%i %q"},
    EscapeConversionsTest{"WHERE c%02d LIKE '%%%s%%'", "WHERE c%02d LIKE '%%%q%%'"},
    // a trailing percent has no second half to read
    EscapeConversionsTest{"100%", "100%"},
};

class EscapeConversionsTester : public testing::WithParamInterface<EscapeConversionsTest>,
                                public testing::Test
{
};

TEST_P(EscapeConversionsTester, EscapeStringConversions)
{
  const auto params = GetParam();
  EXPECT_EQ(params.expected, TestEscapeStringConversions(params.format));
}

INSTANTIATE_TEST_SUITE_P(TestDbWrappers,
                         EscapeConversionsTester,
                         testing::ValuesIn(EscapeConversionsTests));

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
  EXPECT_EQ(params.expectedSqlite, TestPrepareSQL<dbiplus::SqliteDatabase>(params.format));
}

#if defined(HAS_MYSQL) || defined(HAS_MARIADB)
TEST_P(VPrepareNoParamTester, MySql)
{
  const auto params = GetParam();
  EXPECT_EQ(params.expectedMySql, TestPrepareSQL<dbiplus::MysqlDatabase>(params.format));
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
            TestPrepareSQL<dbiplus::SqliteDatabase>(params.format, params.param.c_str()));
}

INSTANTIATE_TEST_SUITE_P(TestDbWrappers,
                         VPrepareStringParamTester,
                         testing::ValuesIn(VPrepareStringParamTests));

/*!
 * Both backends, because a column holding a count of seconds is written on both, and a conversion
 * narrower than the value moves the row it names to another date without saying so.
 */
struct VPrepareInt64ParamTest
{
  std::string format;
  int64_t param;
  std::string expected;
};

const auto VPrepareInt64ParamTests = std::array{
    VPrepareInt64ParamTest{"WHERE iEndTime > %lld", 0, "WHERE iEndTime > 0"},
    // past the signed 32-bit ceiling of 2038-01-19
    VPrepareInt64ParamTest{"WHERE iEndTime > %lld", 4102444740, "WHERE iEndTime > 4102444740"},
    // past the unsigned one of 2106-02-07
    VPrepareInt64ParamTest{"WHERE iEndTime > %lld", 7258118400, "WHERE iEndTime > 7258118400"},
    // and below the epoch, which a conversion read as unsigned would wrap
    VPrepareInt64ParamTest{"WHERE iEndTime > %lld", -2208988800, "WHERE iEndTime > -2208988800"},
};

class VPrepareInt64ParamTester : public testing::WithParamInterface<VPrepareInt64ParamTest>,
                                 public testing::Test
{
};

TEST_P(VPrepareInt64ParamTester, Sqlite)
{
  const auto params = GetParam();
  EXPECT_EQ(params.expected, TestPrepareSQL<dbiplus::SqliteDatabase>(params.format, params.param));
}

#if defined(HAS_MYSQL) || defined(HAS_MARIADB)
TEST_P(VPrepareInt64ParamTester, MySql)
{
  const auto params = GetParam();
  EXPECT_EQ(params.expected, TestPrepareSQL<dbiplus::MysqlDatabase>(params.format, params.param));
}
#endif

INSTANTIATE_TEST_SUITE_P(TestDbWrappers,
                         VPrepareInt64ParamTester,
                         testing::ValuesIn(VPrepareInt64ParamTests));

/*!
 * A literal percent next to two 64-bit conversions: the escaping pass steps over a conversion two
 * characters at a time, so this is where it would lose its place and swap the arguments.
 */
TEST(VPrepareInt64Param, ALiteralPercentBetweenConversionsSqlite)
{
  EXPECT_EQ("(iStartTime % 86400) >= 7258118400",
            TestPrepareSQL<dbiplus::SqliteDatabase>("(iStartTime %% %lld) >= %lld", int64_t{86400},
                                                    int64_t{7258118400}));
}

#if defined(HAS_MYSQL) || defined(HAS_MARIADB)
TEST(VPrepareInt64Param, ALiteralPercentBetweenConversionsMySql)
{
  EXPECT_EQ("(iStartTime % 86400) >= 7258118400",
            TestPrepareSQL<dbiplus::MysqlDatabase>("(iStartTime %% %lld) >= %lld", int64_t{86400},
                                                   int64_t{7258118400}));
}
#endif
