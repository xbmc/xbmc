/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AppParamParser.h"

#include "CompileInfo.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"

#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

namespace
{

constexpr const char* versionText =
    R"""({0} Media Center {1}
Copyright (C) {2} Team {0} - http://kodi.tv
)""";

constexpr const char* helpText =
    R"""(Usage: {0} [OPTION]... [FILE]...

Arguments:
  -fs                   Runs {1} in full screen
  --standalone          {1} runs in a stand alone environment without a window
                        manager and supporting applications. For example, that
                        enables network settings.
  -p or --portable      {1} will look for configurations in install folder instead of ~/.{0}
  --debug               Enable debug logging
  --version             Print version information
  --test                Enable test mode. [FILE] required.
  --settings=<filename> Loads specified file after advancedsettings.xml replacing any settings specified
                        specified file must exist in special://xbmc/system/
)""";

} // namespace

CAppParamParser::CAppParamParser() : m_playlist(std::make_unique<CFileItemList>())
{
}

CAppParamParser::~CAppParamParser() = default;

void CAppParamParser::Parse(const char* const* argv, int nArgs)
{
  if (nArgs > 1)
  {
    for (int i = 1; i < nArgs; i++)
      ParseArg(argv[i]);

    // testmode is only valid if at least one item to play was given
    if (m_playlist->IsEmpty())
      m_testmode = false;
  }
}

void CAppParamParser::DisplayVersion()
{
  std::cout << StringUtils::Format(versionText, CSysInfo::GetAppName(), CSysInfo::GetVersion(),
                                   CCompileInfo::GetCopyrightYears());
  exit(0);
}

void CAppParamParser::DisplayHelp()
{
  std::string lcAppName = CSysInfo::GetAppName();
  StringUtils::ToLower(lcAppName);

  std::cout << StringUtils::Format(helpText, lcAppName, CSysInfo::GetAppName());
}

void CAppParamParser::ParseArg(const std::string &arg)
{
  if (arg == "-fs" || arg == "--fullscreen")
    m_startFullScreen = true;
  else if (arg == "-h" || arg == "--help")
  {
    DisplayHelp();
    exit(0);
  }
  else if (arg == "-v" || arg == "--version")
    DisplayVersion();
  else if (arg == "--standalone")
    m_standAlone = true;
  else if (arg == "-p" || arg  == "--portable")
    m_platformDirectories = false;
  else if (arg == "--debug")
    m_logLevel = LOG_LEVEL_DEBUG;
  else if (arg == "--test")
    m_testmode = true;
  else if (arg.substr(0, 11) == "--settings=")
    m_settingsFile = arg.substr(11);
  else if (arg.length() != 0 && arg[0] != '-')
  {
    const CFileItemPtr item = std::make_shared<CFileItem>(arg);
    item->SetPath(arg);
    m_playlist->Add(item);
  }
}

void CAppParamParser::SetAdvancedSettings(CAdvancedSettings& advancedSettings) const
{
  if (m_logLevel == LOG_LEVEL_DEBUG)
  {
    advancedSettings.m_logLevel = LOG_LEVEL_DEBUG;
    advancedSettings.m_logLevelHint = LOG_LEVEL_DEBUG;
    CServiceBroker::GetLogging().SetLogLevel(LOG_LEVEL_DEBUG);
  }

  if (!m_settingsFile.empty())
    advancedSettings.AddSettingsFile(m_settingsFile);

  if (m_startFullScreen)
    advancedSettings.m_startFullScreen = true;

  if (m_standAlone)
    advancedSettings.m_handleMounting = true;
}

const CFileItemList& CAppParamParser::GetPlaylist() const
{
  return *m_playlist;
}
