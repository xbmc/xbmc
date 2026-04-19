/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CompileInfo.h"
#include "ServiceBroker.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <gtest/gtest.h>

TEST(TestRegExp, RegFind)
{
  CRegExp regex;

  EXPECT_TRUE(regex.RegComp("^Test.*"));
  EXPECT_EQ(0, regex.RegFind("Test string."));

  EXPECT_TRUE(regex.RegComp("^string.*"));
  EXPECT_EQ(-1, regex.RegFind("Test string."));
}

TEST(TestRegExp, InvalidPattern)
{
  CRegExp regex;

  EXPECT_FALSE(regex.RegComp("+"));
}

TEST(TestRegExp, Unicode)
{
  CRegExp regex;

  EXPECT_TRUE(regex.RegComp("Бог!$"));
  EXPECT_EQ(12, regex.RegFind("С нами Бог!"));
}

TEST(TestRegExp, JIT)
{
  CRegExp regex;

  EXPECT_TRUE(regex.RegComp(".JIT.", CRegExp::StudyWithJitComp));
  EXPECT_EQ(12, regex.RegFind("Test string, JIT-matched."));
}

TEST(TestRegExp, GetReplaceString)
{
  CRegExp regex;

  EXPECT_TRUE(regex.RegComp("^(Test)\\s*(.*)\\."));
  EXPECT_EQ(0, regex.RegFind("Test string."));
  EXPECT_STREQ("string", regex.GetReplaceString("\\2").c_str());
}

TEST(TestRegExp, GetFindLen)
{
  CRegExp regex;

  EXPECT_TRUE(regex.RegComp("^(Test)\\s*(.*)\\."));
  EXPECT_EQ(0, regex.RegFind("Test string."));
  EXPECT_EQ(12, regex.GetFindLen());
}

TEST(TestRegExp, GetSubCount)
{
  CRegExp regex;

  EXPECT_TRUE(regex.RegComp("^(Test)\\s*(.*)\\."));
  EXPECT_EQ(0, regex.RegFind("Test string."));
  EXPECT_EQ(2, regex.GetSubCount());
}

TEST(TestRegExp, GetSubStart)
{
  CRegExp regex;

  EXPECT_TRUE(regex.RegComp("^(Test)\\s*(.*)\\."));
  EXPECT_EQ(0, regex.RegFind("Test string."));
  EXPECT_EQ(0, regex.GetSubStart(0));
  EXPECT_EQ(0, regex.GetSubStart(1));
  EXPECT_EQ(5, regex.GetSubStart(2));
}

TEST(TestRegExp, GetSubLength)
{
  CRegExp regex;

  EXPECT_TRUE(regex.RegComp("^(Test)\\s*(.*)\\."));
  EXPECT_EQ(0, regex.RegFind("Test string."));
  EXPECT_EQ(12, regex.GetSubLength(0));
  EXPECT_EQ(4, regex.GetSubLength(1));
  EXPECT_EQ(6, regex.GetSubLength(2));
}

TEST(TestRegExp, GetCaptureTotal)
{
  CRegExp regex;

  // Zero capturing groups
  EXPECT_TRUE(regex.RegComp("abc"));
  EXPECT_EQ(0, regex.GetCaptureTotal());

  // 1 capturing group
  EXPECT_TRUE(regex.RegComp("(abc)"));
  EXPECT_EQ(1, regex.GetCaptureTotal());

  // 2 capturing groups
  EXPECT_TRUE(regex.RegComp("^(Test)\\s*(.*)\\."));
  EXPECT_EQ(2, regex.GetCaptureTotal());

  // Mixed: 2 capturing, 2 non-capturing
  EXPECT_TRUE(regex.RegComp("(a)(?:b)(c)(?:d)"));
  EXPECT_EQ(2, regex.GetCaptureTotal());

  // Named groups still count as capturing
  EXPECT_TRUE(regex.RegComp("(?<first>a)(?<second>b)"));
  EXPECT_EQ(2, regex.GetCaptureTotal());

  // Nested groups
  EXPECT_TRUE(regex.RegComp("((a)(b))"));
  EXPECT_EQ(3, regex.GetCaptureTotal());

  // Before compilation
  CRegExp uncompiled;
  EXPECT_EQ(-1, uncompiled.GetCaptureTotal());
}

TEST(TestRegExp, GetMatch)
{
  CRegExp regex;

  EXPECT_TRUE(regex.RegComp("^(Test)\\s*(.*)\\."));
  EXPECT_EQ(0, regex.RegFind("Test string."));
  EXPECT_STREQ("Test string.", regex.GetMatch(0).c_str());
  EXPECT_STREQ("Test", regex.GetMatch(1).c_str());
  EXPECT_STREQ("string", regex.GetMatch(2).c_str());
}

TEST(TestRegExp, GetPattern)
{
  CRegExp regex;

  EXPECT_TRUE(regex.RegComp("^(Test)\\s*(.*)\\."));
  EXPECT_STREQ("^(Test)\\s*(.*)\\.", regex.GetPattern().c_str());
}

TEST(TestRegExp, operatorEqual)
{
  CRegExp regex, regexcopy;
  std::string match;

  EXPECT_TRUE(regex.RegComp("^(?<first>Test)\\s*(?<second>.*)\\."));
  regexcopy = regex;
  EXPECT_EQ(0, regexcopy.RegFind("Test string."));
}

TEST(TestRegExp, BoundsChecks)
{
  CRegExp regex;

  EXPECT_TRUE(regex.RegComp("^(Test)\\s*(.*)\\."));
  EXPECT_EQ(0, regex.RegFind("Test string."));

  // One past last capturing group (the off-by-one slot)
  EXPECT_TRUE(regex.GetMatch(3).empty());
  EXPECT_EQ(-1, regex.GetSubStart(3));
  EXPECT_EQ(-1, regex.GetSubLength(3));

  // Further beyond
  EXPECT_TRUE(regex.GetMatch(4).empty());
  EXPECT_EQ(-1, regex.GetSubStart(4));
  EXPECT_EQ(-1, regex.GetSubLength(4));

  // At and beyond m_MaxNumOfBackrefrences (20)
  EXPECT_TRUE(regex.GetMatch(CRegExp::m_MaxNumOfBackrefrences).empty());
  EXPECT_EQ(-1, regex.GetSubStart(CRegExp::m_MaxNumOfBackrefrences));
  EXPECT_EQ(-1, regex.GetSubLength(CRegExp::m_MaxNumOfBackrefrences));
  EXPECT_TRUE(regex.GetMatch(CRegExp::m_MaxNumOfBackrefrences + 1).empty());
  EXPECT_EQ(-1, regex.GetSubStart(CRegExp::m_MaxNumOfBackrefrences + 1));
  EXPECT_EQ(-1, regex.GetSubLength(CRegExp::m_MaxNumOfBackrefrences + 1));

  // Negative index
  EXPECT_TRUE(regex.GetMatch(-1).empty());
  EXPECT_EQ(-1, regex.GetSubStart(-1));
  EXPECT_EQ(-1, regex.GetSubLength(-1));
}

// Non-capturing groups (?:...) must not inflate GetCaptureTotal
TEST(TestRegExp, NonCapturingGroupsNotCounted)
{
  CRegExp regex;

  // (?:...) is invisible to capture numbering
  EXPECT_TRUE(regex.RegComp("S(\\d+)E(\\d+)(?:[^\\\\/]*)$"));
  EXPECT_EQ(2, regex.GetCaptureTotal());

  EXPECT_GE(regex.RegFind("S01E02_remainder"), 0);
  EXPECT_STREQ("01", regex.GetMatch(1).c_str());
  EXPECT_STREQ("02", regex.GetMatch(2).c_str());

  // Group 3 doesn't exist — non-capturing group is not numbered
  EXPECT_TRUE(regex.GetMatch(3).empty());
  EXPECT_EQ(-1, regex.GetSubStart(3));
}

// Optional groups within pattern: unmatched groups should return empty/−1,
// not stale data from a previous match
TEST(TestRegExp, OptionalGroupUnmatched)
{
  CRegExp regex;

  EXPECT_TRUE(regex.RegComp("(a)(b)?(c)?"));
  EXPECT_EQ(3, regex.GetCaptureTotal());

  // First match: all 3 groups participate
  EXPECT_GE(regex.RegFind("abc"), 0);
  EXPECT_STREQ("a", regex.GetMatch(1).c_str());
  EXPECT_STREQ("b", regex.GetMatch(2).c_str());
  EXPECT_STREQ("c", regex.GetMatch(3).c_str());

  // Second match (same compiled pattern, reuses m_matchData): only group 1
  EXPECT_GE(regex.RegFind("a"), 0);
  EXPECT_STREQ("a", regex.GetMatch(1).c_str());

  // Groups 2 and 3 did not participate — must NOT return stale "b"/"c"
  EXPECT_TRUE(regex.GetMatch(2).empty());
  EXPECT_TRUE(regex.GetMatch(3).empty());

  // Beyond pattern — off-by-one slot
  EXPECT_TRUE(regex.GetMatch(4).empty());
  EXPECT_EQ(-1, regex.GetSubStart(4));
  EXPECT_EQ(-1, regex.GetSubLength(4));
}

// Verify behavior before any match and after a failed match
TEST(TestRegExp, GetMatchNoMatch)
{
  CRegExp regex;

  EXPECT_TRUE(regex.RegComp("(a)(b)(c)"));

  // Before any RegFind: m_bMatched is false, m_iMatchCount is 0
  EXPECT_TRUE(regex.GetMatch(0).empty());
  EXPECT_TRUE(regex.GetMatch(1).empty());
  EXPECT_EQ(-1, regex.GetSubStart(0));

  // After failed RegFind
  EXPECT_EQ(-1, regex.RegFind("xyz"));
  EXPECT_TRUE(regex.GetMatch(0).empty());
  EXPECT_TRUE(regex.GetMatch(1).empty());
  EXPECT_EQ(-1, regex.GetSubStart(0));
  EXPECT_EQ(-1, regex.GetSubLength(0));
}

class TestRegExpLog : public testing::Test
{
protected:
  TestRegExpLog() = default;
  ~TestRegExpLog() override { CServiceBroker::GetLogging().Deinitialize(); }
};

TEST_F(TestRegExpLog, DumpOvector)
{
  CRegExp regex;
  std::string logfile, logstring;
  char buf[100];
  ssize_t bytesread;
  XFILE::CFile file;

  std::string appName = CCompileInfo::GetAppName();
  StringUtils::ToLower(appName);
  logfile = CSpecialProtocol::TranslatePath("special://temp/") + appName + ".log";
  CServiceBroker::GetLogging().Initialize(CSpecialProtocol::TranslatePath("special://temp/"));
  EXPECT_TRUE(XFILE::CFile::Exists(logfile));

  EXPECT_TRUE(regex.RegComp("^(?<first>Test)\\s*(?<second>.*)\\."));
  EXPECT_EQ(0, regex.RegFind("Test string."));
  regex.DumpOvector(LOGDEBUG);
  CServiceBroker::GetLogging().Deinitialize();

  EXPECT_TRUE(file.Open(logfile));
  while ((bytesread = file.Read(buf, sizeof(buf) - 1)) > 0)
  {
    buf[bytesread] = '\0';
    logstring.append(buf);
  }
  file.Close();
  EXPECT_FALSE(logstring.empty());

  EXPECT_STREQ("\xEF\xBB\xBF", logstring.substr(0, 3).c_str());

  EXPECT_TRUE(regex.RegComp(".*(debug|DEBUG) <general>: regexp ovector=\\{\\[0,12\\],\\[0,4\\],"
                            "\\[5,11\\]\\}.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);

  EXPECT_TRUE(XFILE::CFile::Delete(logfile));
}
