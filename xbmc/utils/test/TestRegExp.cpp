/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

/* TODO: gtest/gtest.h needs to come in before utils/RegExp.h.
 * Investigate why.
 */
#include "gtest/gtest.h"

#include "utils/RegExp.h"
#include "utils/log.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"

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
  EXPECT_STREQ("string", regex.GetReplaceString("\\2"));
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
  TestRegExpLog(){}
  ~TestRegExpLog()
  {
    /* Reset globals used by CLog after each test. */
    g_log_globalsRef->m_file = NULL;
    g_log_globalsRef->m_repeatCount = 0;
    g_log_globalsRef->m_repeatLogLevel = -1;
    g_log_globalsRef->m_logLevel = LOG_LEVEL_DEBUG;
  }
};

TEST_F(TestRegExpLog, DumpOvector)
{
  CRegExp regex;
  CStdString logfile, logstring;
  char buf[100];
  unsigned int bytesread;
  XFILE::CFile file;

  logfile = CSpecialProtocol::TranslatePath("special://temp/") + "xbmc.log";
  EXPECT_TRUE(CLog::Init(CSpecialProtocol::TranslatePath("special://temp/")));
  EXPECT_TRUE(XFILE::CFile::Exists(logfile));

  EXPECT_TRUE(regex.RegComp("^(?<first>Test)\\s*(?<second>.*)\\."));
  EXPECT_EQ(0, regex.RegFind("Test string."));
  regex.DumpOvector(LOGDEBUG);
  CLog::Close();

  EXPECT_TRUE(file.Open(logfile));
  while ((bytesread = file.Read(buf, sizeof(buf) - 1)) > 0)
  {
    buf[bytesread] = '\0';
    logstring.append(buf);
  }
  file.Close();
  EXPECT_FALSE(logstring.empty());

  EXPECT_STREQ("\xEF\xBB\xBF", logstring.substr(0, 3).c_str());

  EXPECT_TRUE(regex.RegComp(".*DEBUG: regexp ovector=\\{\\[0,12\\],\\[0,4\\],"
                            "\\[5,11\\]\\}.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);

  EXPECT_TRUE(XFILE::CFile::Delete(logfile));
}
