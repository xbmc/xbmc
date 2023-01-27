/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/** @todo gtest/gtest.h needs to come in before utils/RegExp.h.
 * Investigate why.
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

TEST(TestRegExp, GetCaptureTotal)
{
  CRegExp regex;

  EXPECT_TRUE(regex.RegComp("^(Test)\\s*(.*)\\."));
  EXPECT_EQ(0, regex.RegFind("Test string."));
  EXPECT_EQ(2, regex.GetCaptureTotal());
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

TEST(TestRegExp, GetNamedSubPattern)
{
  CRegExp regex;
  std::string match;

  EXPECT_TRUE(regex.RegComp("^(?<first>Test)\\s*(?<second>.*)\\."));
  EXPECT_EQ(0, regex.RegFind("Test string."));
  EXPECT_TRUE(regex.GetNamedSubPattern("first", match));
  EXPECT_STREQ("Test", match.c_str());
  EXPECT_TRUE(regex.GetNamedSubPattern("second", match));
  EXPECT_STREQ("string", match.c_str());
}

TEST(TestRegExp, operatorEqual)
{
  CRegExp regex, regexcopy;
  std::string match;

  EXPECT_TRUE(regex.RegComp("^(?<first>Test)\\s*(?<second>.*)\\."));
  regexcopy = regex;
  EXPECT_EQ(0, regexcopy.RegFind("Test string."));
  EXPECT_TRUE(regexcopy.GetNamedSubPattern("first", match));
  EXPECT_STREQ("Test", match.c_str());
  EXPECT_TRUE(regexcopy.GetNamedSubPattern("second", match));
  EXPECT_STREQ("string", match.c_str());
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
