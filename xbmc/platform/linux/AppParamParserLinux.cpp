/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AppParamParserLinux.h"

#include "AppParams.h"
#include "CompileInfo.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <vector>

namespace
{
std::vector<std::string> availableWindowSystems = CCompileInfo::GetAvailableWindowSystems();
std::array<std::string, 1> availableLogTargets = {"console"};

constexpr const char* windowingText =
    R"""(
Selected window system not available: {}
    Available window systems: {}
)""";

constexpr const char* loggingText =
    R"""(
Selected logging target not available: {}
    Available log targest: {}
)""";

constexpr const char* helpText =
    R"""(
Linux Specific Arguments:
  --windowing=<system>  Select which windowing method to use.
                          Available window systems are: {}
  --logging=<target>    Select which log target to use (log file will always be used in conjunction).
                          Available log targets are: {}
)""";


} // namespace

CAppParamParserLinux::CAppParamParserLinux() : CAppParamParser()
{
}

CAppParamParserLinux::~CAppParamParserLinux() = default;

void CAppParamParserLinux::ParseArg(const std::string& arg)
{
  CAppParamParser::ParseArg(arg);

  if (arg.substr(0, 12) == "--windowing=")
  {
    if (std::find(availableWindowSystems.begin(), availableWindowSystems.end(), arg.substr(12)) !=
        availableWindowSystems.end())
      GetAppParams()->SetWindowing(arg.substr(12));
    else
    {
      std::cout << StringUtils::Format(windowingText, arg.substr(12),
                                       StringUtils::Join(availableWindowSystems, ", "));
      exit(0);
    }
  }
  else if (arg.substr(0, 10) == "--logging=")
  {
    if (std::find(availableLogTargets.begin(), availableLogTargets.end(), arg.substr(10)) !=
        availableLogTargets.end())
    {
      GetAppParams()->SetLogTarget(arg.substr(10));
    }
    else
    {
      std::cout << StringUtils::Format(loggingText, arg.substr(10),
                                       StringUtils::Join(availableLogTargets, ", "));
      exit(0);
    }
  }
}

void CAppParamParserLinux::DisplayHelp()
{
  CAppParamParser::DisplayHelp();

  std::cout << StringUtils::Format(helpText, StringUtils::Join(availableWindowSystems, ", "),
                                   StringUtils::Join(availableLogTargets, ", "));
}
