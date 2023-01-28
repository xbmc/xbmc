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
#include "test/TestUtils.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <stdlib.h>

#include <gtest/gtest.h>

class Testlog : public testing::Test
{
protected:
  Testlog() = default;
  ~Testlog() override { CServiceBroker::GetLogging().Deinitialize(); }
};

TEST_F(Testlog, Log)
{
  std::string logfile, logstring;
  char buf[100];
  ssize_t bytesread;
  XFILE::CFile file;
  CRegExp regex;

  std::string appName = CCompileInfo::GetAppName();
  StringUtils::ToLower(appName);
  logfile = CSpecialProtocol::TranslatePath("special://temp/") + appName + ".log";
  CServiceBroker::GetLogging().Initialize(CSpecialProtocol::TranslatePath("special://temp/"));
  EXPECT_TRUE(XFILE::CFile::Exists(logfile));

  CLog::Log(LOGDEBUG, "debug log message");
  CLog::Log(LOGINFO, "info log message");
  CLog::Log(LOGWARNING, "warning log message");
  CLog::Log(LOGERROR, "error log message");
  CLog::Log(LOGFATAL, "fatal log message");
  CLog::Log(LOGNONE, "none type log message");
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

  EXPECT_TRUE(regex.RegComp(".*(debug|DEBUG) <general>: debug log message.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);
  EXPECT_TRUE(regex.RegComp(".*(info|INFO) <general>: info log message.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);
  EXPECT_TRUE(regex.RegComp(".*(warning|WARNING) <general>: warning log message.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);
  EXPECT_TRUE(regex.RegComp(".*(error|ERROR) <general>: error log message.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);
  EXPECT_TRUE(regex.RegComp(".*(critical|CRITICAL|fatal|FATAL) <general>: fatal log message.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);
  EXPECT_TRUE(regex.RegComp(".*(off|OFF) <general>: none type log message.*"));
  EXPECT_GE(regex.RegFind(logstring), 0);

  EXPECT_TRUE(XFILE::CFile::Delete(logfile));
}

TEST_F(Testlog, SetLogLevel)
{
  std::string logfile;

  std::string appName = CCompileInfo::GetAppName();
  StringUtils::ToLower(appName);
  logfile = CSpecialProtocol::TranslatePath("special://temp/") + appName + ".log";
  CServiceBroker::GetLogging().Initialize(CSpecialProtocol::TranslatePath("special://temp/"));
  EXPECT_TRUE(XFILE::CFile::Exists(logfile));

  EXPECT_EQ(LOG_LEVEL_DEBUG, CServiceBroker::GetLogging().GetLogLevel());
  CServiceBroker::GetLogging().SetLogLevel(LOG_LEVEL_MAX);
  EXPECT_EQ(LOG_LEVEL_MAX, CServiceBroker::GetLogging().GetLogLevel());

  CServiceBroker::GetLogging().Deinitialize();
  EXPECT_TRUE(XFILE::CFile::Delete(logfile));
}
