/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AppParamParser.h"
#include "PlayListPlayer.h"
#include "Application.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/SystemInfo.h"
#include "utils/StringUtils.h"
#ifdef TARGET_WINDOWS
#include "WIN32Util.h"
#endif
#ifndef TARGET_WINDOWS
#include "platform/linux/XTimeUtils.h"
#endif
#include <stdlib.h>

using namespace KODI::MESSAGING;

CAppParamParser::CAppParamParser()
: m_logLevel(LOG_LEVEL_NORMAL)
{
}

void CAppParamParser::Parse(const char* const* argv, int nArgs)
{
  if (nArgs > 1)
  {
    for (int i = 1; i < nArgs; i++)
      ParseArg(argv[i]);
  }
}

void CAppParamParser::DisplayVersion()
{
  printf("%s Media Center %s\n", CSysInfo::GetVersion().c_str(), CSysInfo::GetAppName().c_str());
  printf("Copyright (C) 2005-2013 Team %s - http://kodi.tv\n", CSysInfo::GetAppName().c_str());
  exit(0);
}

void CAppParamParser::DisplayHelp()
{
  std::string lcAppName = CSysInfo::GetAppName();
  StringUtils::ToLower(lcAppName);
  printf("Usage: %s [OPTION]... [FILE]...\n\n", lcAppName.c_str());
  printf("Arguments:\n");
  printf("  -fs\t\t\tRuns %s in full screen\n", CSysInfo::GetAppName().c_str());
  printf("  --standalone\t\t%s runs in a stand alone environment without a window \n", CSysInfo::GetAppName().c_str());
  printf("\t\t\tmanager and supporting applications. For example, that\n");
  printf("\t\t\tenables network settings.\n");
  printf("  -p or --portable\t%s will look for configurations in install folder instead of ~/.%s\n", CSysInfo::GetAppName().c_str(), lcAppName.c_str());
  printf("  --debug\t\tEnable debug logging\n");
  printf("  --version\t\tPrint version information\n");
  printf("  --test\t\tEnable test mode. [FILE] required.\n");
  printf("  --settings=<filename>\t\tLoads specified file after advancedsettings.xml replacing any settings specified\n");
  printf("  \t\t\t\tspecified file must exist in special://xbmc/system/\n");
  exit(0);
}

void CAppParamParser::ParseArg(const std::string &arg)
{
  if (arg == "-fs" || arg == "--fullscreen")
    m_startFullScreen = true;
  else if (arg == "-h" || arg == "--help")
    DisplayHelp();
  else if (arg == "-v" || arg == "--version")
    DisplayVersion();
  else if (arg == "--standalone")
  {
    m_standAlone = true;
    g_application.SetStandAlone(true);
  }
  else if (arg == "-p" || arg  == "--portable")
    g_application.EnablePlatformDirectories(false);
  else if (arg == "--debug")
    m_logLevel = LOG_LEVEL_DEBUG;
  else if (arg == "--test")
    m_testmode = true;
  else if (arg.substr(0, 11) == "--settings=")
    m_settingsFile = arg.substr(11);
  else if (arg.length() != 0 && arg[0] != '-')
  {
    if (m_testmode)
      g_application.SetEnableTestMode(true);

    const CFileItemPtr item = std::make_shared<CFileItem>(arg);
    item->SetPath(arg);

    m_playlist.Add(item);
  }
}

void CAppParamParser::SetAdvancedSettings(CAdvancedSettings& advancedSettings) const
{
  if (m_logLevel == LOG_LEVEL_DEBUG)
  {
    advancedSettings.m_logLevel = LOG_LEVEL_DEBUG;
    advancedSettings.m_logLevelHint = LOG_LEVEL_DEBUG;
    CLog::SetLogLevel(LOG_LEVEL_DEBUG);
  }

  if (!m_settingsFile.empty())
    advancedSettings.AddSettingsFile(m_settingsFile);

  if (m_startFullScreen)
    advancedSettings.m_startFullScreen = true;

  if (m_standAlone)
    advancedSettings.m_handleMounting = true;
}
