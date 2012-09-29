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

#include "utils/log.h"
#include "utils/RegExp.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"

#include "test/TestUtils.h"

#include "gtest/gtest.h"

class Testlog : public testing::Test
{
protected:
  Testlog(){}
  ~Testlog()
  {
    /* Reset globals used by CLog after each test. */
    g_log_globalsRef->m_file = NULL;
    g_log_globalsRef->m_repeatCount = 0;
    g_log_globalsRef->m_repeatLogLevel = -1;
    g_log_globalsRef->m_logLevel = LOG_LEVEL_DEBUG;
  }
};

TEST_F(Testlog, Log)
{
  CStdString logfile, logstring;
  char buf[100];
  unsigned int bytesread;
  XFILE::CFile file;
  CRegExp regex;

  logfile = CSpecialProtocol::TranslatePath("special://temp/") + "xbmc.log";
  EXPECT_TRUE(CLog::Init(CSpecialProtocol::TranslatePath("special://temp/")));
  EXPECT_TRUE(XFILE::CFile::Exists(logfile));

  CLog::Log(LOGDEBUG, "debug log message");
  CLog::Log(LOGINFO, "info log message");
  CLog::Log(LOGNOTICE, "notice log message");
  CLog::Log(LOGWARNING, "warning log message");
  CLog::Log(LOGERROR, "error log message");
  CLog::Log(LOGSEVERE, "severe log message");
  CLog::Log(LOGFATAL, "fatal log message");
  CLog::Log(LOGNONE, "none type log message");
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

  EXPECT_TRUE(regex.RegComp(".*DEBUG: debug log message.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);
  EXPECT_TRUE(regex.RegComp(".*INFO: info log message.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);
  EXPECT_TRUE(regex.RegComp(".*NOTICE: notice log message.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);
  EXPECT_TRUE(regex.RegComp(".*WARNING: warning log message.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);
  EXPECT_TRUE(regex.RegComp(".*ERROR: error log message.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);
  EXPECT_TRUE(regex.RegComp(".*SEVERE: severe log message.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);
  EXPECT_TRUE(regex.RegComp(".*FATAL: fatal log message.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);
  EXPECT_TRUE(regex.RegComp(".*NONE: none type log message.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);

  EXPECT_TRUE(XFILE::CFile::Delete(logfile));
}

TEST_F(Testlog, MemDump)
{
  CStdString logfile, logstring;
  char buf[100];
  unsigned int bytesread;
  XFILE::CFile file;
  CRegExp regex;
  char refdata[] = "0123456789abcdefghijklmnopqrstuvwxyz";

  logfile = CSpecialProtocol::TranslatePath("special://temp/") + "xbmc.log";
  EXPECT_TRUE(CLog::Init(CSpecialProtocol::TranslatePath("special://temp/")));
  EXPECT_TRUE(XFILE::CFile::Exists(logfile));

  CLog::MemDump(refdata, sizeof(refdata));
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

  EXPECT_TRUE(regex.RegComp(".*DEBUG: MEM_DUMP: Dumping from.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);
  EXPECT_TRUE(regex.RegComp(".*DEBUG: MEM_DUMP: 0000  30 31 32 33.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);
  EXPECT_TRUE(regex.RegComp(".*73 74 75 76  ghijklmnopqrstuv.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);

  EXPECT_TRUE(XFILE::CFile::Delete(logfile));
}

TEST_F(Testlog, SetLogLevel)
{
  CStdString logfile;

  logfile = CSpecialProtocol::TranslatePath("special://temp/") + "xbmc.log";
  EXPECT_TRUE(CLog::Init(CSpecialProtocol::TranslatePath("special://temp/")));
  EXPECT_TRUE(XFILE::CFile::Exists(logfile));

  EXPECT_EQ(LOG_LEVEL_DEBUG, CLog::GetLogLevel());
  CLog::SetLogLevel(LOG_LEVEL_MAX);
  EXPECT_EQ(LOG_LEVEL_MAX, CLog::GetLogLevel());

  CLog::Close();
  EXPECT_TRUE(XFILE::CFile::Delete(logfile));
}
