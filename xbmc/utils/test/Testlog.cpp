/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <stdlib.h>
#include "utils/log.h"
#include "utils/RegExp.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "CompileInfo.h"

#include "test/TestUtils.h"

#include "gtest/gtest.h"

class Testlog : public testing::Test
{
protected:
  Testlog() = default;
  ~Testlog() override
  {
    CLog::Close();
  }
};

TEST_F(Testlog, Log)
{
  std::string logfile, logstring;
  char buf[100];
  unsigned int bytesread;
  XFILE::CFile file;
  CRegExp regex;

  std::string appName = CCompileInfo::GetAppName();
  StringUtils::ToLower(appName);
  logfile = CSpecialProtocol::TranslatePath("special://temp/") + appName + ".log";
  EXPECT_TRUE(CLog::Init(CSpecialProtocol::TranslatePath("special://temp/").c_str()));
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
  std::string logfile, logstring;
  char buf[100];
  unsigned int bytesread;
  XFILE::CFile file;
  CRegExp regex;
  char refdata[] = "0123456789abcdefghijklmnopqrstuvwxyz";

  std::string appName = CCompileInfo::GetAppName();
  StringUtils::ToLower(appName);
  logfile = CSpecialProtocol::TranslatePath("special://temp/") + appName + ".log";
  EXPECT_TRUE(CLog::Init(CSpecialProtocol::TranslatePath("special://temp/").c_str()));
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
  std::string logfile;

  std::string appName = CCompileInfo::GetAppName();
  StringUtils::ToLower(appName);
  logfile = CSpecialProtocol::TranslatePath("special://temp/") + appName + ".log";
  EXPECT_TRUE(CLog::Init(CSpecialProtocol::TranslatePath("special://temp/").c_str()));
  EXPECT_TRUE(XFILE::CFile::Exists(logfile));

  EXPECT_EQ(LOG_LEVEL_DEBUG, CLog::GetLogLevel());
  CLog::SetLogLevel(LOG_LEVEL_MAX);
  EXPECT_EQ(LOG_LEVEL_MAX, CLog::GetLogLevel());

  CLog::Close();
  EXPECT_TRUE(XFILE::CFile::Delete(logfile));
}
