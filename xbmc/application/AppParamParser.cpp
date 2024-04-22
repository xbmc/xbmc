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
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "application/AppParams.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"

#include <iostream>
#include <stdlib.h>
#include <string>

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

CAppParamParser::CAppParamParser() : m_params(std::make_shared<CAppParams>())
{
}

void CAppParamParser::Parse(const char* const* argv, int nArgs)
{
  std::vector<std::string> args;
  args.reserve(nArgs);

  for (int i = 0; i < nArgs; i++)
  {
    args.emplace_back(argv[i]);
    if (i > 0)
      ParseArg(argv[i]);
  }

  if (nArgs > 1)
  {
    // testmode is only valid if at least one item to play was given
    if (m_params->GetPlaylist().IsEmpty())
      m_params->SetTestMode(false);
  }

  // Record raw paramerters
  m_params->SetRawArgs(std::move(args));
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
    m_params->SetStartFullScreen(true);
  else if (arg == "-h" || arg == "--help")
  {
    DisplayHelp();
    exit(0);
  }
  else if (arg == "-v" || arg == "--version")
    DisplayVersion();
  else if (arg == "--standalone")
    m_params->SetStandAlone(true);
  else if (arg == "-p" || arg  == "--portable")
    m_params->SetPlatformDirectories(false);
  else if (arg == "--debug")
    m_params->SetLogLevel(LOG_LEVEL_DEBUG);
  else if (arg == "--test")
    m_params->SetTestMode(true);
  else if (arg.substr(0, 11) == "--settings=")
    m_params->SetSettingsFile(arg.substr(11));
  else if (arg.length() != 0 && arg[0] != '-')
  {
    const CFileItemPtr item = std::make_shared<CFileItem>(arg);
    item->SetPath(arg);
    m_params->GetPlaylist().Add(item);
  }
}
